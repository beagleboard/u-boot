// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005-2007 by Texas Instruments
 * Some code has been taken from tusb6010.c
 * Copyrights for that are attributable to:
 * Copyright (C) 2006 Nokia Corporation
 * Tony Lindgren <tony@atomide.com>
 *
 * This file is part of the Inventra Controller Driver for Linux.
 */
#include <dm.h>
#include <log.h>
#include <serial.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/usb/otg.h>
#include <asm/global_data.h>
#include <asm/omap_common.h>
#include <asm/omap_musb.h>
#include <twl4030.h>
#include <twl6030.h>
#include "linux-compat.h"
#include "musb_core.h"
#include "omap2430.h"
#include "musb_uboot.h"

static inline void omap2430_low_level_exit(struct musb *musb)
{
	u32 l;

	/* in any role */
	l = musb_readl(musb->mregs, OTG_FORCESTDBY);
	l |= ENABLEFORCE;	/* enable MSTANDBY */
	musb_writel(musb->mregs, OTG_FORCESTDBY, l);
}

static inline void omap2430_low_level_init(struct musb *musb)
{
	u32 l;

	l = musb_readl(musb->mregs, OTG_FORCESTDBY);
	l &= ~ENABLEFORCE;	/* disable MSTANDBY */
	musb_writel(musb->mregs, OTG_FORCESTDBY, l);
}

static int omap2430_musb_init(struct musb *musb)
{
	u32 l;
	int status = 0;
	unsigned long int start;

	struct omap_musb_board_data *data =
		(struct omap_musb_board_data *)musb->controller;

	/* Reset the controller */
	musb_writel(musb->mregs, OTG_SYSCONFIG, SOFTRST);

	start = get_timer(0);

	while (1) {
		l = musb_readl(musb->mregs, OTG_SYSCONFIG);
		if ((l & SOFTRST) == 0)
			break;

		if (get_timer(start) > (CONFIG_SYS_HZ / 1000)) {
			dev_err(musb->controller, "MUSB reset is taking too long\n");
			return -ENODEV;
		}
	}

	l = musb_readl(musb->mregs, OTG_INTERFSEL);

	if (data->interface_type == MUSB_INTERFACE_UTMI) {
		/* OMAP4 uses Internal PHY GS70 which uses UTMI interface */
		l &= ~ULPI_12PIN;       /* Disable ULPI */
		l |= UTMI_8BIT;         /* Enable UTMI  */
	} else {
		l |= ULPI_12PIN;
	}

	musb_writel(musb->mregs, OTG_INTERFSEL, l);

	pr_debug("HS USB OTG: revision 0x%x, sysconfig 0x%02x, "
			"sysstatus 0x%x, intrfsel 0x%x, simenable  0x%x\n",
			musb_readl(musb->mregs, OTG_REVISION),
			musb_readl(musb->mregs, OTG_SYSCONFIG),
			musb_readl(musb->mregs, OTG_SYSSTATUS),
			musb_readl(musb->mregs, OTG_INTERFSEL),
			musb_readl(musb->mregs, OTG_SIMENABLE));
	return 0;

err1:
	return status;
}

static int omap2430_musb_enable(struct musb *musb)
{
#ifdef CONFIG_TWL4030_USB
	if (twl4030_usb_ulpi_init()) {
		serial_printf("ERROR: %s Could not initialize PHY\n",
				__PRETTY_FUNCTION__);
	}
#endif

#ifdef CONFIG_TWL6030_POWER
	twl6030_usb_device_settings();
#endif

#ifdef CONFIG_OMAP44XX
	u32 *usbotghs_control = (u32 *)((*ctrl)->control_usbotghs_ctrl);
	*usbotghs_control = USBOTGHS_CONTROL_AVALID |
		USBOTGHS_CONTROL_VBUSVALID | USBOTGHS_CONTROL_IDDIG;
#endif

	return 0;
}

static void omap2430_musb_disable(struct musb *musb)
{

}

static int omap2430_musb_exit(struct musb *musb)
{
	del_timer_sync(&musb_idle_timer);

	omap2430_low_level_exit(musb);

	return 0;
}

const struct musb_platform_ops omap2430_ops = {
	.init		= omap2430_musb_init,
	.exit		= omap2430_musb_exit,
	.enable		= omap2430_musb_enable,
	.disable	= omap2430_musb_disable,
};

#if CONFIG_IS_ENABLED(DM_USB)

struct omap2430_musb_plat {
	void *base;
	void *ctrl_mod_base;
	struct musb_hdrc_platform_data plat;
	struct musb_hdrc_config musb_config;
	struct omap_musb_board_data otg_board_data;
};

static int omap2430_musb_of_to_plat(struct udevice *dev)
{
	struct omap2430_musb_plat *plat = dev_get_plat(dev);
	const void *fdt = gd->fdt_blob;
	int node = dev_of_offset(dev);

	plat->base = (void *)dev_read_addr_ptr(dev);

	plat->musb_config.multipoint = fdtdec_get_int(fdt, node, "multipoint",
						      -1);
	if (plat->musb_config.multipoint < 0) {
		pr_err("MUSB multipoint DT entry missing\n");
		return -ENOENT;
	}

	plat->musb_config.dyn_fifo = 1;
	plat->musb_config.num_eps = fdtdec_get_int(fdt, node, "num-eps", -1);
	if (plat->musb_config.num_eps < 0) {
		pr_err("MUSB num-eps DT entry missing\n");
		return -ENOENT;
	}

	plat->musb_config.ram_bits = fdtdec_get_int(fdt, node, "ram-bits", -1);
	if (plat->musb_config.ram_bits < 0) {
		pr_err("MUSB ram-bits DT entry missing\n");
		return -ENOENT;
	}

	plat->plat.power = fdtdec_get_int(fdt, node, "power", -1);
	if (plat->plat.power < 0) {
		pr_err("MUSB power DT entry missing\n");
		return -ENOENT;
	}

	plat->otg_board_data.interface_type = fdtdec_get_int(fdt, node,
							     "interface-type",
							     -1);
	if (plat->otg_board_data.interface_type < 0) {
		pr_err("MUSB interface-type DT entry missing\n");
		return -ENOENT;
	}

#if 0 /* In a perfect world, mode would be set to OTG, mode 3 from DT */
	plat->plat.mode = fdtdec_get_int(fdt, node, "mode", -1);
	if (plat->plat.mode < 0) {
		pr_err("MUSB mode DT entry missing\n");
		return -ENOENT;
	}
#else /* MUSB_OTG, it doesn't work */
#ifdef CONFIG_USB_MUSB_HOST /* Host seems to be the only option that works */
	plat->plat.mode = MUSB_HOST;
#else /* For that matter, MUSB_PERIPHERAL doesn't either */
	plat->plat.mode = MUSB_PERIPHERAL;
#endif
#endif
	plat->otg_board_data.dev = dev;
	plat->plat.config = &plat->musb_config;
	plat->plat.platform_ops = &omap2430_ops;
	plat->plat.board_data = &plat->otg_board_data;
	return 0;
}

static int omap2430_musb_probe(struct udevice *dev)
{
	struct omap2430_musb_plat *plat = dev_get_plat(dev);
	struct omap_musb_board_data *otg_board_data;
	int ret = 0;
	void *base = dev_read_addr_ptr(dev);
	struct musb *musbp;

	otg_board_data = &plat->otg_board_data;

	if (IS_ENABLED(CONFIG_USB_MUSB_HOST)) {
		struct musb_host_data *host = dev_get_priv(dev);
		struct usb_bus_priv *priv = dev_get_uclass_priv(dev);

		priv->desc_before_addr = true;

		host->host = musb_init_controller(&plat->plat,
						  (struct device *)otg_board_data,
						  plat->base);
		if (!host->host)
			return -EIO;

		return musb_lowlevel_init(host);
	} else if (CONFIG_IS_ENABLED(DM_USB_GADGET)) {
		struct musb_host_data *host = dev_get_priv(dev);

		host->host = musb_init_controller(&plat->plat,
						  (struct device *)otg_board_data,
						  plat->base);
		if (!host->host)
			return -EIO;

		return usb_add_gadget_udc((struct device *)otg_board_data, &host->host->g);
	}

	musbp = musb_register(&plat->plat, (struct device *)otg_board_data,
			      plat->base);
	if (IS_ERR_OR_NULL(musbp))
		return -EINVAL;

	return 0;
}

static int omap2430_musb_remove(struct udevice *dev)
{
	struct musb_host_data *host = dev_get_priv(dev);

	musb_stop(host->host);

	return 0;
}

#ifndef CONFIG_USB_MUSB_HOST
static int omap2340_gadget_handle_interrupts(struct udevice *dev)
{
	struct musb_host_data *host = dev_get_priv(dev);

	host->host->isr(0, host->host);

	return 0;
}

static const struct usb_gadget_generic_ops omap2340_gadget_ops = {
	.handle_interrupts	= omap2340_gadget_handle_interrupts,
};
#endif

static const struct udevice_id omap2430_musb_ids[] = {
	{ .compatible = "ti,omap3-musb" },
	{ .compatible = "ti,omap4-musb" },
	{ }
};

U_BOOT_DRIVER(omap2430_musb) = {
	.name	= "omap2430-musb",
#ifdef CONFIG_USB_MUSB_HOST
	.id		= UCLASS_USB,
#else
	.id		= UCLASS_USB_GADGET_GENERIC,
	.ops		= &omap2340_gadget_ops,
#endif
	.of_match = omap2430_musb_ids,
	.of_to_plat = omap2430_musb_of_to_plat,
	.probe = omap2430_musb_probe,
	.remove = omap2430_musb_remove,
#ifdef CONFIG_USB_MUSB_HOST
	.ops = &musb_usb_ops,
#endif
	.plat_auto	= sizeof(struct omap2430_musb_plat),
	.priv_auto	= sizeof(struct musb_host_data),
};

#endif /* CONFIG_IS_ENABLED(DM_USB) */
