// SPDX-License-Identifier: GPL-2.0+
/*
 * FIT image loader using MTD read
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Apurva Nandan <a-nandan@ti.com>
 */
#include <common.h>
#include <config.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/io.h>
#include <mtd.h>

uint32_t __weak spl_mtd_get_uboot_offs(void)
{
	return CONFIG_SYS_MTD_U_BOOT_OFFS;
}

static ulong spl_mtd_fit_read(struct spl_load_info *load, ulong offs,
			      ulong size, void *dst)
{
	struct mtd_info *mtd = load->dev;
	int err;
	size_t ret_len;

	err = mtd_read(mtd, offs, size, &ret_len, dst);
	if (!err)
		return ret_len;

	return 0;
}

static int spl_mtd_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	int err;
	struct legacy_img_hdr *header;
	__maybe_unused struct spl_load_info load;
	struct mtd_info *mtd;
	size_t ret_len;

	mtd_probe_devices();

	switch (bootdev->boot_device) {
	case BOOT_DEVICE_SPINAND:
		mtd = get_mtd_device_nm("spi-nand0");
		if (IS_ERR_OR_NULL(mtd)) {
			printf("MTD device %s not found, ret %ld\n", "spi-nand",
			       PTR_ERR(mtd));
			err = PTR_ERR(mtd);
			goto remove_mtd_device;
		}
		break;
	default:
		puts(SPL_TPL_PROMPT "Unsupported MTD Boot Device!\n");
		err = -EINVAL;
		goto remove_mtd_device;
	}

	header = spl_get_load_buffer(0, sizeof(*header));

	err = mtd_read(mtd, spl_mtd_get_uboot_offs(), sizeof(*header),
		       &ret_len, (void *)header);
	if (err)
		goto remove_mtd_device;

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		debug("Found FIT\n");
		load.dev = mtd;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = spl_mtd_fit_read;
		err = spl_load_simple_fit(spl_image, &load,
					  spl_mtd_get_uboot_offs(), header);
	} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
		load.dev = mtd;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = spl_mtd_fit_read;
		err = spl_load_imx_container(spl_image, &load,
					     spl_mtd_get_uboot_offs());
	} else {
		err = spl_parse_image_header(spl_image, bootdev, header);
		if (err)
			goto remove_mtd_device;
		err = mtd_read(mtd, spl_mtd_get_uboot_offs(), spl_image->size,
			       &ret_len, (void *)(ulong)spl_image->load_addr);
	}

remove_mtd_device:
	mtd_remove(mtd);
	return err;
}

SPL_LOAD_IMAGE_METHOD("SPINAND", 0, BOOT_DEVICE_SPINAND, spl_mtd_load_image);
