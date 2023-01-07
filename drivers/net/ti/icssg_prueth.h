/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Texas Instruments K3 AM65 Ethernet Switch SubSystem Driver
 *
 * Copyright (C) 2019, Texas Instruments, Incorporated
 *
 */

#ifndef __NET_TI_ICSSG_PRUETH_H
#define __NET_TI_ICSSG_PRUETH_H

#include <asm/io.h>
#include <clk.h>
#include <dm/lists.h>
#include <dm/ofnode.h>
#include <dm/device.h>
#include <dma-uclass.h>
#include <regmap.h>
#include <linux/pruss_driver.h>
#include "icssg_config.h"

void icssg_class_set_mac_addr(struct regmap *miig_rt, int slice, u8 *mac);
void icssg_class_disable(struct regmap *miig_rt, int slice, bool is_sr1);
void icssg_class_default(struct regmap *miig_rt, int slice, bool allmulti,
			 bool is_sr1);
void icssg_ft1_set_mac_addr(struct regmap *miig_rt, int slice, u8 *mac_addr);

enum prueth_mac {
	PRUETH_MAC0 = 0,
	PRUETH_MAC1,
	PRUETH_NUM_MACS,
};

enum prueth_port {
	PRUETH_PORT_HOST = 0,	/* host side port */
	PRUETH_PORT_MII0,	/* physical port MII 0 */
	PRUETH_PORT_MII1,	/* physical port MII 1 */
};

struct prueth {
	struct udevice		*dev;
	struct udevice		*pruss;
	struct regmap		*miig_rt;
	struct regmap		*mii_rt;
	fdt_addr_t		mdio_base;
	struct pruss_mem_region shram;
	struct pruss_mem_region dram;
	phys_addr_t		tmaddr;
	struct mii_dev		*bus;
	u32			port_id;
	u32			sram_pa;
	struct phy_device	*phydev;
	bool			has_phy;
	ofnode			phy_node;
	u32			phy_addr;
	ofnode			eth_node[PRUETH_NUM_MACS];
	struct icssg_config_sr1	config[PRUSS_NUM_PRUS];
	u32			mdio_freq;
	int			phy_interface;
	struct			clk mdiofck;
	struct dma		dma_tx;
	struct dma		dma_rx;
	struct dma		dma_rx_mgm;
	u32			rx_next;
	u32			rx_pend;
	int			slice;
	bool			is_sr1;
	bool			mdio_manual_mode;
};

/* config helpers */
void icssg_config_ipg(struct prueth *prueth, int speed, int mii);
int icssg_config(struct prueth *prueth);
void icssg_config_sr1(struct prueth *prueth);

#endif /* __NET_TI_ICSSG_PRUETH_H */
