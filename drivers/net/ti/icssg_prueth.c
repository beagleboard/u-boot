// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments K3 AM65 PRU Ethernet Driver
 *
 * Copyright (C) 2018-2024, Texas Instruments, Incorporated
 *
 */

#include <asm/io.h>
#include <asm/processor.h>
#include <clk.h>
#include <dm/lists.h>
#include <dm/device.h>
#include <dma-uclass.h>
#include <dm/of_access.h>
#include <dm/pinctrl.h>
#include <fs_loader.h>
#include <miiphy.h>
#include <net.h>
#include <phy.h>
#include <power-domain.h>
#include <linux/soc/ti/ti-udma.h>
#include <regmap.h>
#include <remoteproc.h>
#include <syscon.h>
#include <soc.h>
#include <linux/pruss_driver.h>
#include <dm/device_compat.h>

#include "cpsw_mdio.h"
#include "icssg_prueth.h"
#include "icss_mii_rt.h"

#define ICSS_SLICE0     0
#define ICSS_SLICE1     1

#ifdef PKTSIZE_ALIGN
#define UDMA_RX_BUF_SIZE PKTSIZE_ALIGN
#else
#define UDMA_RX_BUF_SIZE ALIGN(1522, ARCH_DMA_MINALIGN)
#endif

#ifdef PKTBUFSRX
#define UDMA_RX_DESC_NUM PKTBUFSRX
#else
#define UDMA_RX_DESC_NUM 4
#endif

/* Config region lies in shared RAM */
#define ICSS_CONFIG_OFFSET_SLICE0	0
#define ICSS_CONFIG_OFFSET_SLICE1	0x8000

/* Firmware flags */
#define ICSS_SET_RUN_FLAG_VLAN_ENABLE		BIT(0)	/* switch only */
#define ICSS_SET_RUN_FLAG_FLOOD_UNICAST		BIT(1)	/* switch only */
#define ICSS_SET_RUN_FLAG_PROMISC		BIT(2)	/* MAC only */
#define ICSS_SET_RUN_FLAG_MULTICAST_PROMISC	BIT(3)	/* MAC only */

/* CTRLMMR_ICSSG_RGMII_CTRL register bits */
#define ICSSG_CTRL_RGMII_ID_MODE		BIT(24)

/* Management packet type */
#define PRUETH_PKT_TYPE_CMD		0x10

/* Number of PRU Cores per Slice */
#define ICSSG_NUM_PRU_CORES		3

static int icssg_phy_init(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);
	struct phy_device *phydev;
	u32 supported = PHY_GBIT_FEATURES;
	int ret;

	phydev = phy_connect(priv->bus,
			     priv->phy_addr,
			     priv->dev,
			     priv->phy_interface);

	if (!phydev) {
		dev_err(dev, "phy_connect() failed\n");
		return -ENODEV;
	}

	/* disable unsupported features */
	supported &= ~(PHY_10BT_FEATURES |
			SUPPORTED_100baseT_Half |
			SUPPORTED_1000baseT_Half |
			SUPPORTED_Pause |
			SUPPORTED_Asym_Pause);

	phydev->supported &= supported;
	phydev->advertising = phydev->supported;

	if (IS_ENABLED(CONFIG_DM_ETH))
		if (ofnode_valid(priv->phy_node))
			phydev->node = priv->phy_node;

	priv->phydev = phydev;
	ret = phy_config(phydev);
	if (ret < 0)
		pr_err("phy_config() failed: %d", ret);

	return ret;
}

static ofnode prueth_find_mdio(ofnode parent)
{
	ofnode node;

	ofnode_for_each_subnode(node, parent)
		if (ofnode_device_is_compatible(node, "ti,davinci_mdio"))
			return node;

	return ofnode_null();
}

static int prueth_mdio_setup(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);
	struct udevice *mdio_dev;
	ofnode mdio;
	int ret;

	mdio = prueth_find_mdio(dev_ofnode(priv->pruss));
	if (!ofnode_valid(mdio))
		return 0;

	/*
	 * The MDIO controller is represented in the DT binding by a
	 * subnode of the MAC controller.
	 *
	 * We don't have a DM driver for the MDIO device yet, and thus any
	 * pinctrl setting on its node will be ignored.
	 *
	 * However, we do need to make sure the pins states tied to the
	 * MDIO node are configured properly. Fortunately, the core DM
	 * does that for use when we get a device, so we can work around
	 * that whole issue by just requesting a dummy MDIO driver to
	 * probe, and our pins will get muxed.
	 */
	ret = uclass_get_device_by_ofnode(UCLASS_MDIO, mdio, &mdio_dev);
	if (ret)
		return ret;

	return 0;
}

static int icssg_mdio_init(struct udevice *dev)
{
	struct prueth *prueth = dev_get_priv(dev);
	int ret;

	ret = prueth_mdio_setup(dev);
	if (ret)
		return ret;

	prueth->bus = cpsw_mdio_init(dev->name, prueth->mdio_base,
				     prueth->mdio_freq,
				     clk_get_rate(&prueth->mdiofck),
				     prueth->mdio_manual_mode);
	if (!prueth->bus)
		return -EFAULT;

	return 0;
}

static void icssg_config_set_speed(struct prueth *priv, int speed)
{
	u8 fw_speed;

	switch (speed) {
	case SPEED_1000:
		fw_speed = FW_LINK_SPEED_1G;
		break;
	case SPEED_100:
		fw_speed = FW_LINK_SPEED_100M;
		break;
	case SPEED_10:
		fw_speed = FW_LINK_SPEED_10M;
		break;
	default:
		/* Other links speeds not supported */
		dev_err(priv->dev, "Unsupported link speed\n");
		return;
	}

	writeb(fw_speed, priv->dram.pa + PORT_LINK_SPEED_OFFSET);
}

static int icssg_update_link(struct prueth *priv)
{
	struct phy_device *phy = priv->phydev;
	bool gig_en = false, full_duplex = false;

	if (phy->link) { /* link up */
		if (phy->speed == SPEED_1000)
			gig_en = true;
		if (phy->duplex == DUPLEX_FULL)
			full_duplex = true;
		/* Set the RGMII cfg for gig en and full duplex */
		icssg_update_rgmii_cfg(priv->miig_rt, phy->speed, full_duplex,
				       priv->slice, priv);
		/* update the Tx IPG based on 100M/1G speed */
		icssg_config_ipg(priv, phy->speed, priv->slice);

		/* Send command to firmware to update Speed setting */
		icssg_config_set_speed(priv, phy->speed);

		/* Enable PORT FORWARDING */
		emac_set_port_state(priv, ICSSG_EMAC_PORT_FORWARD);

		printf("link up on port %d, speed %d, %s duplex\n",
		       priv->slice, phy->speed,
		       (phy->duplex == DUPLEX_FULL) ? "full" : "half");
	} else {
		emac_set_port_state(priv, ICSSG_EMAC_PORT_DISABLE);
		printf("link down on port %d\n", priv->slice);
	}

	return phy->link;
}

struct icssg_firmwares {
	char *pru;
	char *rtu;
	char *txpru;
};

static struct icssg_firmwares icssg_emac_firmwares[] = {
	{
		.pru = "/lib/firmware/ti-pruss/am65x-sr2-pru0-prueth-fw.elf",
		.rtu = "/lib/firmware/ti-pruss/am65x-sr2-rtu0-prueth-fw.elf",
		.txpru = "/lib/firmware/ti-pruss/am65x-sr2-txpru0-prueth-fw.elf",
	},
	{
		.pru = "/lib/firmware/ti-pruss/am65x-sr2-pru1-prueth-fw.elf",
		.rtu = "/lib/firmware/ti-pruss/am65x-sr2-rtu1-prueth-fw.elf",
		.txpru = "/lib/firmware/ti-pruss/am65x-sr2-txpru1-prueth-fw.elf",
	}
};

static int icssg_start_pru_cores(struct udevice *dev)
{
	struct prueth *prueth = dev_get_priv(dev);
	struct icssg_firmwares *firmwares;
	struct udevice *rproc_dev = NULL;
	int ret, slice;
	u32 phandle;
	u8 index;

	slice = prueth->slice;
	index = slice * ICSSG_NUM_PRU_CORES;
	firmwares = icssg_emac_firmwares;

	ofnode_read_u32_index(dev_ofnode(dev), "ti,prus", index, &phandle);
	ret = uclass_get_device_by_phandle_id(UCLASS_REMOTEPROC, phandle, &rproc_dev);
	if (ret) {
		dev_err(dev, "Unknown remote processor with phandle '0x%x' requested(%d)\n",
			phandle, ret);
		return ret;
	}

	prueth->pru_core_id = dev_seq(rproc_dev);
	ret = rproc_set_firmware(rproc_dev, firmwares[slice].pru);
	if (ret)
		return ret;

	ret = rproc_boot(rproc_dev);
	if (ret) {
		dev_err(dev, "failed to boot PRU%d: %d\n", slice, ret);
		return -EINVAL;
	}

	ofnode_read_u32_index(dev_ofnode(dev), "ti,prus", index + 1, &phandle);
	ret = uclass_get_device_by_phandle_id(UCLASS_REMOTEPROC, phandle, &rproc_dev);
	if (ret) {
		dev_err(dev, "Unknown remote processor with phandle '0x%x' requested(%d)\n",
			phandle, ret);
		goto halt_pru;
	}

	prueth->rtu_core_id = dev_seq(rproc_dev);
	ret = rproc_set_firmware(rproc_dev, firmwares[slice].rtu);
	if (ret)
		goto halt_pru;

	ret = rproc_boot(rproc_dev);
	if (ret) {
		dev_err(dev, "failed to boot RTU%d: %d\n", slice, ret);
		goto halt_pru;
	}

	ofnode_read_u32_index(dev_ofnode(dev), "ti,prus", index + 2, &phandle);
	ret = uclass_get_device_by_phandle_id(UCLASS_REMOTEPROC, phandle, &rproc_dev);
	if (ret) {
		dev_err(dev, "Unknown remote processor with phandle '0x%x' requested(%d)\n",
			phandle, ret);
		goto halt_rtu;
	}

	prueth->txpru_core_id = dev_seq(rproc_dev);
	ret = rproc_set_firmware(rproc_dev, firmwares[slice].txpru);
	if (ret)
		goto halt_rtu;

	ret = rproc_boot(rproc_dev);
	if (ret) {
		dev_err(dev, "failed to boot TXPRU%d: %d\n", slice, ret);
		goto halt_rtu;
	}

	return 0;

halt_rtu:
	rproc_stop(prueth->rtu_core_id);

halt_pru:
	rproc_stop(prueth->pru_core_id);
	return ret;
}

static int icssg_stop_pru_cores(struct udevice *dev)
{
	struct prueth *prueth = dev_get_priv(dev);

	rproc_stop(prueth->pru_core_id);
	rproc_stop(prueth->rtu_core_id);
	rproc_stop(prueth->txpru_core_id);

	return 0;
}

static int prueth_start(struct udevice *dev)
{
	struct ti_udma_drv_chan_cfg_data *dma_rx_cfg_data;
	struct eth_pdata *pdata = dev_get_plat(dev);
	struct prueth *priv = dev_get_priv(dev);
	struct icssg_flow_cfg *flow_cfg;
	u8 *hwaddr = pdata->enetaddr;
	char chn_name[16];
	void *config;
	int ret, i;

	icssg_class_set_mac_addr(priv->miig_rt, priv->slice, hwaddr);
	icssg_ft1_set_mac_addr(priv->miig_rt, priv->slice, hwaddr);
	icssg_class_default(priv->miig_rt, priv->slice, 0);

	/* Set Load time configuration */
	icssg_config(priv);

	ret = icssg_start_pru_cores(dev);
	if (ret)
		return ret;

	/* To differentiate channels for SLICE0 vs SLICE1 */
	snprintf(chn_name, sizeof(chn_name), "tx%d-0", priv->slice);

	ret = dma_get_by_name(dev, chn_name, &priv->dma_tx);
	if (ret)
		dev_err(dev, "TX dma get failed %d\n", ret);

	snprintf(chn_name, sizeof(chn_name), "rx%d", priv->slice);
	ret = dma_get_by_name(dev, chn_name, &priv->dma_rx);
	if (ret)
		dev_err(dev, "RX dma get failed %d\n", ret);

	for (i = 0; i < UDMA_RX_DESC_NUM; i++) {
		ret = dma_prepare_rcv_buf(&priv->dma_rx,
					  net_rx_packets[i],
					  UDMA_RX_BUF_SIZE);
		if (ret)
			dev_err(dev, "RX dma add buf failed %d\n", ret);
	}

	ret = dma_enable(&priv->dma_tx);
	if (ret) {
		dev_err(dev, "TX dma_enable failed %d\n", ret);
		goto tx_fail;
	}

	ret = dma_enable(&priv->dma_rx);
	if (ret) {
		dev_err(dev, "RX dma_enable failed %d\n", ret);
		goto rx_fail;
	}

	/* check if the rx_flow_id of dma_rx is as expected since
	 * driver hardcode that value in config struct to firmware
	 * in probe. Just add this sanity check to catch any change
	 * to rx channel assignment in the future.
	 */
	dma_get_cfg(&priv->dma_rx, 0, (void **)&dma_rx_cfg_data);
	config = (void *)(priv->dram.pa + ICSSG_CONFIG_OFFSET);
	flow_cfg = config + PSI_L_REGULAR_FLOW_ID_BASE_OFFSET;
	writew(dma_rx_cfg_data->flow_id_base, &flow_cfg->rx_base_flow);
	writew(0, &flow_cfg->mgm_base_flow);

	dev_info(dev, "K3 ICSSG: rflow_id_base: %u, chn_name = %s\n",
		 dma_rx_cfg_data->flow_id_base, chn_name);

	ret = emac_fdb_flow_id_updated(priv);
	if (ret) {
		dev_err(dev, "Failed to update Rx Flow ID %d", ret);
		goto phy_fail;
	}

	ret = phy_startup(priv->phydev);
	if (ret) {
		dev_err(dev, "phy_startup failed\n");
		goto phy_fail;
	}

	ret = icssg_update_link(priv);
	if (!ret) {
		ret = -ENODEV;
		goto phy_shut;
	}

	return 0;

phy_shut:
	phy_shutdown(priv->phydev);
phy_fail:
	dma_disable(&priv->dma_rx);
	dma_free(&priv->dma_rx);
rx_fail:
	dma_disable(&priv->dma_tx);
	dma_free(&priv->dma_tx);

tx_fail:
	icssg_class_disable(priv->miig_rt, priv->slice);

	return ret;
}

static int prueth_send(struct udevice *dev, void *packet, int length)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret;

	ret = dma_send(&priv->dma_tx, packet, length, NULL);

	return ret;
}

static int prueth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret;

	/* try to receive a new packet */
	ret = dma_receive(&priv->dma_rx, (void **)packetp, NULL);

	return ret;
}

static int prueth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret = 0;

	if (length > 0) {
		u32 pkt = priv->rx_next % UDMA_RX_DESC_NUM;

		dev_dbg(dev, "%s length:%d pkt:%u\n", __func__, length, pkt);

		ret = dma_prepare_rcv_buf(&priv->dma_rx,
					  net_rx_packets[pkt],
					  UDMA_RX_BUF_SIZE);
		priv->rx_next++;
	}

	return ret;
}

static void prueth_stop(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);

	phy_shutdown(priv->phydev);

	dma_disable(&priv->dma_tx);
	dma_disable(&priv->dma_rx);

	icssg_stop_pru_cores(dev);

	dma_free(&priv->dma_tx);
	dma_free(&priv->dma_rx);
}

static const struct eth_ops prueth_ops = {
	.start		= prueth_start,
	.send		= prueth_send,
	.recv		= prueth_recv,
	.free_pkt	= prueth_free_pkt,
	.stop		= prueth_stop,
};

static int icssg_ofdata_parse_phy(struct udevice *dev, ofnode port_np)
{
	struct prueth *priv = dev_get_priv(dev);
	struct ofnode_phandle_args out_args;
	int ret = 0;

	priv->phy_interface = ofnode_read_phy_mode(port_np);

	if (priv->phy_interface == PHY_INTERFACE_MODE_NA) {
		dev_err(dev, "Invalid PHY mode '%s'\n",
			phy_string_for_interface(priv->phy_interface));
		ret = -EINVAL;
		goto out;
	}

	ret = ofnode_parse_phandle_with_args(port_np, "phy-handle",
					     NULL, 0, 0, &out_args);
	if (ret) {
		dev_err(dev, "can't parse phy-handle port (%d)\n", ret);
		ret = 0;
	}

	priv->phy_node = out_args.node;
	ret = ofnode_read_u32(priv->phy_node, "reg", &priv->phy_addr);
	if (ret)
		dev_err(dev, "failed to get phy_addr port (%d)\n", ret);

out:
	return ret;
}

static const struct soc_attr k3_mdio_soc_data[] = {
	{ .family = "AM62X", .revision = "SR1.0" },
	{ .family = "AM64X", .revision = "SR1.0" },
	{ .family = "AM64X", .revision = "SR2.0" },
	{ .family = "AM65X", .revision = "SR1.0" },
	{ .family = "AM65X", .revision = "SR2.0" },
	{ .family = "J7200", .revision = "SR1.0" },
	{ .family = "J7200", .revision = "SR2.0" },
	{ .family = "J721E", .revision = "SR1.0" },
	{ .family = "J721E", .revision = "SR1.1" },
	{ .family = "J721S2", .revision = "SR1.0" },
	{ /* sentinel */ },
};

static int prueth_probe(struct udevice *dev)
{
	ofnode node, pruss_node, mdio_node, sram_node, curr_sram_node;
	ofnode eth_ports_node, eth0_node, eth1_node, eth_node;
	struct prueth *prueth = dev_get_priv(dev);
	u32 phandle, err, sp, prev_end_addr;
	struct udevice **prussdev = NULL;
	int ret = 0;

	prueth = dev_get_priv(dev);
	prueth->dev = dev;
	err = ofnode_read_u32(dev_ofnode(dev), "ti,prus", &phandle);
	if (err)
		return err;

	node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(node))
		return -EINVAL;

	pruss_node = ofnode_get_parent(node);
	ret = device_get_global_by_ofnode(pruss_node, prussdev);
	if (ret)
		dev_err(dev, "error getting the pruss dev\n");
	prueth->pruss = *prussdev;

	ret = pruss_request_mem_region(*prussdev, PRUSS_MEM_SHRD_RAM2,
				       &prueth->shram);
	if (ret)
		return ret;

	ret = pruss_request_tm_region(*prussdev, &prueth->tmaddr);
	if (ret)
		return ret;

	eth_ports_node = dev_read_subnode(dev, "ethernet-ports");
	if (!ofnode_valid(eth_ports_node))
		return -ENOENT;

	ofnode_for_each_subnode(eth_node, eth_ports_node) {
		const char *node_name;
		u32 reg;

		node_name = ofnode_get_name(eth_node);
		ret = ofnode_read_u32(eth_node, "reg", &reg);
		if (ret)
			dev_err(dev, "%s: error reading port_id (%d)\n", node_name, ret);

		if (reg >= PRUETH_NUM_MACS) {
			dev_err(dev, "%s: invalid port_id (%d)\n", node_name, reg);
			return -EINVAL;
		}

		if (reg == 0)
			eth0_node = eth_node;
		else if (reg == 1)
			eth1_node = eth_node;
		else
			dev_err(dev, "port reg should be 0 or 1\n");
	}

	/* one node must be present and available else we fail */
	if (!ofnode_is_enabled(eth0_node) && !ofnode_is_enabled(eth1_node)) {
		dev_err(dev, "neither port@0 nor port@1 node available\n");
		return -ENODEV;
	}

	/*
	 * Uboot ethernet framework does not support two interfaces in a single
	 * probe. If the eth0_node is enabled in DT, we'll only probe eth0_node.
	 * If the eth0_node is not enabled in DT, we'll check for eth1_node and
	 * probe the eth1_node if enabled.
	 */
	if (ofnode_is_enabled(eth0_node)) {
		if (ofnode_valid(eth0_node) && ofnode_is_enabled(eth0_node)) {
			prueth->slice = 0;
			icssg_ofdata_parse_phy(dev, eth0_node);
			prueth->eth_node[PRUETH_MAC0] = eth0_node;
			dev_dbg(dev, "Using port0\n");
		}
	} else if (ofnode_is_enabled(eth1_node)) {
		if (ofnode_valid(eth1_node) && ofnode_is_enabled(eth1_node)) {
			prueth->slice = 1;
			icssg_ofdata_parse_phy(dev, eth1_node);
			prueth->eth_node[PRUETH_MAC0] = eth1_node;
			dev_dbg(dev, "Using port1\n");
		}
	}

	ret = pruss_request_mem_region(*prussdev,
				       prueth->slice ? PRUSS_MEM_DRAM1 : PRUSS_MEM_DRAM0,
				       &prueth->dram);
	if (ret) {
		dev_err(dev, "could not request DRAM%d region\n", prueth->slice);
		return ret;
	}

	prueth->miig_rt = syscon_regmap_lookup_by_phandle(dev, "ti,mii-g-rt");
	if (!prueth->miig_rt) {
		dev_err(dev, "couldn't get mii-g-rt syscon regmap\n");
		return -ENODEV;
	}

	prueth->mii_rt = syscon_regmap_lookup_by_phandle(dev, "ti,mii-rt");
	if (!prueth->mii_rt) {
		dev_err(dev, "couldn't get mii-rt syscon regmap\n");
		return -ENODEV;
	}

	ret = ofnode_read_u32(dev_ofnode(dev), "sram", &sp);
	if (ret) {
		dev_err(dev, "sram node fetch failed %d\n", ret);
		return ret;
	}

	sram_node = ofnode_get_by_phandle(sp);
	if (!ofnode_valid(sram_node))
		return -EINVAL;

	prev_end_addr = ofnode_get_addr(sram_node);

	ofnode_for_each_subnode(curr_sram_node, sram_node) {
		u32 start_addr, size, end_addr, avail;
		const char *name;

		name = ofnode_get_name(curr_sram_node);
		start_addr = ofnode_get_addr(curr_sram_node);
		size = ofnode_get_size(curr_sram_node);
		end_addr = start_addr + size;
		avail = start_addr - prev_end_addr;

		if (avail > MSMC_RAM_SIZE)
			break;

		prev_end_addr = end_addr;
	}

	prueth->sram_pa = prev_end_addr;
	if (prueth->sram_pa % SZ_64K != 0) {
		/* This is constraint for SR2.0 firmware */
		dev_err(dev, "sram address needs to be 64KB aligned\n");
		return -EINVAL;
	}
	dev_dbg(dev, "sram: addr %x size %x\n", prueth->sram_pa, MSMC_RAM_SIZE);

	if (prueth->phy_interface != PHY_INTERFACE_MODE_MII &&
	    prueth->phy_interface < PHY_INTERFACE_MODE_RGMII &&
	    prueth->phy_interface > PHY_INTERFACE_MODE_RGMII_TXID) {
		dev_err(prueth->dev, "PHY mode unsupported %s\n",
			phy_string_for_interface(prueth->phy_interface));
		return -EINVAL;
	}

	/* AM65 SR2.0 has TX Internal delay always enabled by hardware
	 * and it is not possible to disable TX Internal delay. The below
	 * switch case block describes how we handle different phy modes
	 * based on hardware restriction.
	 */
	switch (prueth->phy_interface) {
	case PHY_INTERFACE_MODE_RGMII_ID:
		prueth->phy_interface = PHY_INTERFACE_MODE_RGMII_RXID;
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		prueth->phy_interface = PHY_INTERFACE_MODE_RGMII;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_RXID:
		dev_err(prueth->dev, "RGMII mode without TX delay is not supported");
		return -EINVAL;
	default:
		break;
	}

	mdio_node = ofnode_find_subnode(pruss_node, "mdio");
	prueth->mdio_base = ofnode_get_addr(mdio_node);
	ofnode_read_u32(mdio_node, "bus_freq", &prueth->mdio_freq);

	ret = clk_get_by_name_nodev(mdio_node, "fck", &prueth->mdiofck);
	if (ret) {
		dev_err(dev, "failed to get clock %d\n", ret);
		return ret;
	}

	ret = clk_enable(&prueth->mdiofck);
	if (ret) {
		dev_err(dev, "clk_enable failed %d\n", ret);
		return ret;
	}

	prueth->mdio_manual_mode = false;
	if (soc_device_match(k3_mdio_soc_data))
		prueth->mdio_manual_mode = true;

	ret = icssg_mdio_init(dev);
	if (ret)
		return ret;

	ret = icssg_phy_init(dev);
	if (ret) {
		dev_err(dev, "phy_init failed\n");
		goto out;
	}

	return 0;
out:
	cpsw_mdio_free(prueth->bus);
	clk_disable(&prueth->mdiofck);

	return ret;
}

static const struct udevice_id prueth_ids[] = {
	{ .compatible = "ti,am654-icssg-prueth" },
	{ .compatible = "ti,am642-icssg-prueth" },
	{ }
};

U_BOOT_DRIVER(prueth) = {
	.name	= "prueth",
	.id	= UCLASS_ETH,
	.of_match = prueth_ids,
	.probe	= prueth_probe,
	.ops	= &prueth_ops,
	.priv_auto = sizeof(struct prueth),
	.plat_auto = sizeof(struct eth_pdata),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};

static const struct udevice_id prueth_mdio_ids[] = {
	{ .compatible = "ti,davinci_mdio" },
	{ }
};

U_BOOT_DRIVER(prueth_mdio) = {
	.name		= "prueth_mdio",
	.id		= UCLASS_MDIO,
	.of_match	= prueth_mdio_ids,
};
