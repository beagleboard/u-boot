// SPDX-License-Identifier: GPL-2.0
//
// Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
// Author: Vignesh Raghavendra <vigneshr@ti.com>

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <regmap.h>
#include <syscon.h>
#include <dm/device_compat.h>

#define FSS_SYSC_REG	0x4

#define HYPERBUS_CALIB_COUNT 64
#define HYPERBUS_CALIB_MIN_TRIES 48
#define HYPERBUS_CALIB_SUCCESS_COUNT 4
#define HYPERBUS_CALIB_SIZE 64

#define HYPERBUS_CFG_DLL_STAT_REG 4
#define HYPERBUS_CFG_MDLL_LOCK BIT(0)
#define HYPERBUS_CFG_SDLL_LOCK BIT(1)
#define HYPERBUS_CFG_RAM_STAT_REG 8
#define HYPERBUS_CFG_RAM_FIFO_INIT_DONE BIT(0)

struct am654_hbmc_priv {
	void __iomem *mmiobase;
	void __iomem *cfgregbase;
	bool calibrated;
};

/* confirm calibration by looking for "QRY" string within the CFI space */
static int am654_hyperbus_calibrate_check(struct udevice *dev)
{
	struct am654_hbmc_priv *priv = dev_get_priv(dev);
	int count = HYPERBUS_CALIB_COUNT;
	int pass_count = 0;
	int ret = -EIO;
	u16 qry[3];

	while (count--) {
		qry[0] = readw(priv->mmiobase + 0x20);
		qry[1] = readw(priv->mmiobase + 0x22);
		qry[2] = readw(priv->mmiobase + 0x24);

		if (qry[0] == 'Q' && qry[1] == 'R' && qry[2] == 'Y')
			pass_count++;
		else
			pass_count = 0;

		if (pass_count > HYPERBUS_CALIB_SUCCESS_COUNT)
			return 0;
	}

	return ret;
}

static int am654_hyperbus_calibrate(struct udevice *dev)
{
	struct am654_hbmc_priv *priv = dev_get_priv(dev);
	char verifybuf[HYPERBUS_CALIB_SIZE];
	char rdbuf[HYPERBUS_CALIB_SIZE];
	int tries = HYPERBUS_CALIB_COUNT;
	int pass_count = 0;
	unsigned int reg;
	int ret = -EIO;

	if (priv->calibrated)
		return 0;

	if (!priv->cfgregbase || !priv->mmiobase)
		return ret;

	reg = readl(priv->cfgregbase + HYPERBUS_CFG_RAM_STAT_REG);
	if (!(reg & HYPERBUS_CFG_RAM_FIFO_INIT_DONE)) {
		dev_err(dev, "Hyperbus RAM FIFO init failed\n");
		return ret;
	}

	writew(0xF0, priv->mmiobase);
	writew(0x98, priv->mmiobase + 0xaa);

	/*
	 * Perform calibration by reading 64 bytes from CFI region and
	 * compare with previously read data, and ensure Delay Locked Loop(DLL)
	 * is stabilized.
	 */
	while (tries--) {
		memcpy(rdbuf, priv->mmiobase, HYPERBUS_CALIB_SIZE);
		if (!memcmp(rdbuf, verifybuf, HYPERBUS_CALIB_SIZE)) {
			reg = readl(priv->cfgregbase + HYPERBUS_CFG_DLL_STAT_REG);
			if ((reg & HYPERBUS_CFG_MDLL_LOCK) &&
			    (reg & HYPERBUS_CFG_SDLL_LOCK))
				pass_count++;
			else
				pass_count = 0;
		}
		memcpy(verifybuf, rdbuf, HYPERBUS_CALIB_SIZE);
		if (pass_count > HYPERBUS_CALIB_SUCCESS_COUNT &&
		    tries < HYPERBUS_CALIB_MIN_TRIES)
			break;
	}

	if (tries > 0)
		ret = am654_hyperbus_calibrate_check(dev);

	writew(0xF0, priv->mmiobase);
	writew(0xFF, priv->mmiobase);

	return ret;
}

static int am654_select_hbmc(struct udevice *dev)
{
	struct regmap *regmap = syscon_get_regmap(dev_get_parent(dev));

	return regmap_update_bits(regmap, FSS_SYSC_REG, 0x2, 0x2);
}

static int am654_hbmc_bind(struct udevice *dev)
{
	return dm_scan_fdt_dev(dev);
}

static int am654_hbmc_probe(struct udevice *dev)
{
	struct am654_hbmc_priv *priv = dev_get_priv(dev);
	int ret;

	priv->cfgregbase = devfdt_remap_addr_index(dev, 0);
	priv->mmiobase = devfdt_remap_addr_index(dev, 2);

	if (dev_read_bool(dev, "mux-controls")) {
		ret = am654_select_hbmc(dev);
		if (ret) {
			dev_err(dev, "Failed to select HBMC mux\n");
			return ret;
		}
	}

	if (!priv->calibrated) {
		ret = am654_hyperbus_calibrate(dev);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "Calibration Failed\n");
			return ret;
		}
	}
	priv->calibrated = true;

	return 0;
}

static const struct udevice_id am654_hbmc_dt_ids[] = {
	{
		.compatible = "ti,am654-hbmc",
	},
	{ /* end of table */ }
};

U_BOOT_DRIVER(hbmc_am654) = {
	.name	= "hbmc-am654",
	.id	= UCLASS_MTD,
	.of_match = am654_hbmc_dt_ids,
	.probe = am654_hbmc_probe,
	.bind = am654_hbmc_bind,
	.priv_auto	= sizeof(struct am654_hbmc_priv),
};
