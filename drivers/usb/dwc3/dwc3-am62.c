// SPDX-License-Identifier: GPL-2.0
/*
 * dwc3-am62.c - TI specific Glue layer for AM62 DWC3 USB Controller
 *
 * Copyright (C) 2021 Texas Instruments Incorporated - https://www.ti.com
 */

#include <common.h>
#include <asm-generic/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <syscon.h>
#include <malloc.h>
#include <usb.h>
#include <usb/xhci.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <regmap.h>
#include <linux/usb/otg.h>
#include <dm/lists.h>

/* USB WRAPPER register offsets */
#define USBSS_PID			0x0
#define USBSS_OVERCURRENT_CTRL		0x4
#define USBSS_PHY_CONFIG		0x8
#define USBSS_PHY_TEST			0xc
#define USBSS_CORE_STAT			0x14
#define USBSS_HOST_VBUS_CTRL		0x18
#define USBSS_MODE_CONTROL		0x1c
#define USBSS_WAKEUP_CONFIG		0x30
#define USBSS_WAKEUP_STAT		0x34
#define USBSS_OVERRIDE_CONFIG		0x38
#define USBSS_IRQ_MISC_STATUS_RAW	0x430
#define USBSS_IRQ_MISC_STATUS		0x434
#define USBSS_IRQ_MISC_ENABLE_SET	0x438
#define USBSS_IRQ_MISC_ENABLE_CLR	0x43c
#define USBSS_IRQ_MISC_EOI		0x440
#define USBSS_INTR_TEST			0x490
#define USBSS_VBUS_FILTER		0x614
#define USBSS_VBUS_STAT			0x618
#define USBSS_DEBUG_CFG			0x708
#define USBSS_DEBUG_DATA		0x70c
#define USBSS_HOST_HUB_CTRL		0x714

/* PHY CONFIG register bits */
#define USBSS_PHY_VBUS_SEL_MASK		GENMASK(2, 1)
#define USBSS_PHY_VBUS_SEL_SHIFT	1
#define USBSS_PHY_LANE_REVERSE		BIT(0)

/* MODE CONTROL register bits */
#define USBSS_MODE_VALID	BIT(0)

/* IRQ_MISC_STATUS_RAW register bits */
#define USBSS_IRQ_MISC_RAW_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_RAW_SESSVALID	BIT(20)

/* IRQ_MISC_STATUS register bits */
#define USBSS_IRQ_MISC_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_SESSVALID	BIT(20)

/* IRQ_MISC_ENABLE_SET register bits */
#define USBSS_IRQ_MISC_ENABLE_SET_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_ENABLE_SET_SESSVALID	BIT(20)

/* IRQ_MISC_ENABLE_CLR register bits */
#define USBSS_IRQ_MISC_ENABLE_CLR_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_ENABLE_CLR_SESSVALID	BIT(20)

/* VBUS_STAT register bits */
#define USBSS_VBUS_STAT_SESSVALID	BIT(2)
#define USBSS_VBUS_STAT_VBUSVALID	BIT(0)

/* Mask for PHY PLL REFCLK */
#define PHY_PLL_REFCLK_MASK	GENMASK(3, 0)

struct dwc3_data {
	struct udevice *dev;
	void __iomem *usbss;
	struct regmap *syscon;
	unsigned int offset;
	unsigned int vbus_divider;
};

static inline u32 dwc3_ti_readl(struct dwc3_data *data, u32 offset)
{
	return readl(data->usbss + offset);
}

static inline void dwc3_ti_writel(struct dwc3_data *data, u32 offset, u32 value)
{
	writel(value, data->usbss + offset);
}

static const int dwc3_ti_rate_table[] = {	/* in KHZ */
	9600,
	10000,
	12000,
	19200,
	20000,
	24000,
	25000,
	26000,
	38400,
	40000,
	58000,
	50000,
	52000,
};

static int phy_syscon_pll_refclk(struct dwc3_data *data, int rate_code)
{
	struct udevice *dev = data->dev;
	struct ofnode_phandle_args args;
	ofnode node = dev_ofnode(dev);
	struct regmap *syscon;
	int ret;

	syscon = syscon_regmap_lookup_by_phandle(dev, "ti,syscon-phy-pll-refclk");
	if (IS_ERR(syscon)) {
		dev_err(dev, "unable to get ti,syscon-phy-pll-refclk regmap\n");
		return PTR_ERR(syscon);
	}

	data->syscon = syscon;

	ret = ofnode_parse_phandle_with_args(node, "ti,syscon-phy-pll-refclk", NULL, 1,
					     0, &args);
	if (ret)
		return ret;

	data->offset = args.args[0];

	/* Program PHY PLL refclk by reading syscon property */
	ret = regmap_update_bits(data->syscon, data->offset, PHY_PLL_REFCLK_MASK, rate_code);
	if (ret) {
		dev_err(dev, "failed to set phy pll reference clock rate\n");
		return ret;
	}

	return 0;
}

static int dwc3_ti_bind(struct udevice *parent)
{
	ofnode node;
	const char *name = ofnode_get_name(node);
	enum usb_dr_mode dr_mode;
	struct udevice *dev;
	const char *driver = NULL;
	int ret;

	node = ofnode_by_compatible(dev_ofnode(parent), "snps,dwc3");
	if (!ofnode_valid(node)) {
		ret = -ENODEV;
		return 0;
	}

	name = ofnode_get_name(node);
	dr_mode = usb_get_dr_mode(node);
	debug("%s: subnode name: %s\n", __func__, name);

	switch (dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
	case USB_DR_MODE_OTG:
#if CONFIG_IS_ENABLED(DM_USB_GADGET)
		debug("%s: dr_mode: OTG or Peripheral\n", __func__);
		driver = "dwc3-generic-peripheral";
#endif
		break;
#if defined(CONFIG_SPL_USB_HOST_SUPPORT) || !defined(CONFIG_SPL_BUILD)
	case USB_DR_MODE_HOST:
		debug("%s: dr_mode: HOST\n", __func__);
		driver = "dwc3-generic-host";
		break;
#endif
	default:
		debug("%s: unsupported dr_mode\n", __func__);
		return -ENODEV;
	};

	if (!driver)
		return 0;

	ret = device_bind_driver_to_node(parent, driver, name,
					 node, &dev);
	if (ret) {
		debug("%s: not able to bind usb device mode\n",
		      __func__);
		return ret;
	}

	return 0;
}

static int dwc3_ti_probe(struct udevice *dev)
{
	struct dwc3_data *data = dev_get_platdata(dev);
	struct clk usb2_refclk;
	int rate_code, i, ret;
	unsigned long rate;
	u32 reg;

	data->dev = dev;

	data->usbss = dev_remap_addr_index(dev, 0);
	if (IS_ERR(data->usbss)) {
		dev_err(dev, "can't map IOMEM resource\n");
		return PTR_ERR(data->usbss);
	}

	ret = clk_get_by_name(dev, "ref", &usb2_refclk);
	if (ret) {
		dev_err(dev, "can't get usb2_refclk\n");
		return ret;
	}

	/* Calcuate the rate code */
	rate = clk_get_rate(&usb2_refclk);
	rate /= 1000;	/* To KHz */
	for (i = 0; i < ARRAY_SIZE(dwc3_ti_rate_table); i++) {
		if (dwc3_ti_rate_table[i] == rate)
			break;
	}

	if (i == ARRAY_SIZE(dwc3_ti_rate_table)) {
		dev_err(dev, "unsupported usb2_refclk rate: %lu KHz\n", rate);
		return -EINVAL;
	}

	rate_code = i;

	/* Read the syscon property */
	ret = phy_syscon_pll_refclk(data, rate_code);
	if (ret)
		return ret;

	/* VBUS divider select */
	reg = dwc3_ti_readl(data, USBSS_PHY_CONFIG);
	data->vbus_divider = dev_read_bool(dev, "ti,vbus-divider");
	if (data->vbus_divider)
		reg |= 1 << USBSS_PHY_VBUS_SEL_SHIFT;

	dwc3_ti_writel(data, USBSS_PHY_CONFIG, reg);

	/* Set mode valid */
	reg = dwc3_ti_readl(data, USBSS_MODE_CONTROL);
	reg |= USBSS_MODE_VALID;
	dwc3_ti_writel(data, USBSS_MODE_CONTROL, reg);

	return 0;
}

/* work queue not required to cancel separately as auto cancel API was used*/
static int dwc3_ti_remove(struct udevice *dev)
{
	struct dwc3_data *data = dev_get_platdata(dev);
	u32 reg;

	/* Clear mode valid */
	reg = dwc3_ti_readl(data, USBSS_MODE_CONTROL);
	reg &= ~USBSS_MODE_VALID;
	dwc3_ti_writel(data, USBSS_MODE_CONTROL, reg);

	return 0;
}

static const struct udevice_id dwc3_ti_of_match[] = {
	{ .compatible = "ti,am62-usb", },
	{},
};

U_BOOT_DRIVER(dwc3_ti) = {
	.name = "dwc3-ti",
	.id = UCLASS_NOP,
	.of_match = dwc3_ti_of_match,
	.bind = dwc3_ti_bind,
	.probe = dwc3_ti_probe,
	.remove = dwc3_ti_remove,
	.platdata_auto_alloc_size = sizeof(struct dwc3_data),
	.flags = DM_FLAG_OS_PREPARE,
};
