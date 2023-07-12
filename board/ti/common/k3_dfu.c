// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023, Texas Instruments Incorporated - https://www.ti.com/
 */

#include <common.h>
#include <blk.h>
#include <dm.h>
#include <dfu.h>
#include <env.h>
#include <memalign.h>
#include <misc.h>
#include <mtd.h>
#include <mtd_node.h>

#define DFU_ALT_BUF_LEN 512

static void board_get_alt_info_sf(struct mtd_info *mtd, char *buf)
{
	struct mtd_info *part;
	bool first = true;
	const char *name;
	int len;

	name = mtd->name;
	len = strlen(buf);

	list_for_each_entry(part, &mtd->partitions, node) {
		if (!first)
			len += snprintf(buf + len, DFU_ALT_BUF_LEN - len, "; ");
		first = false;
		len += snprintf(buf + len, DFU_ALT_BUF_LEN - len,
				"%s raw %llx %llx", part->name, part->offset, part->size);
	}
}

void set_dfu_alt_info(char *interface, char *devstr)
{
	struct udevice *dev;
	struct mtd_info *mtd;
	char *st, *devstr_bkup;
	unsigned int bus;

	ALLOC_CACHE_ALIGN_BUFFER(char, buf, DFU_ALT_BUF_LEN);

	if (env_get("dfu_alt_info"))
		return;

	memset(buf, 0, sizeof(buf));

	if (!strcmp(interface, "sf")) {
		devstr_bkup = strdup(devstr);
		st = strsep(&devstr_bkup, ":");
		if (!st || !*st) {
			printf("Invalid SPI bus %s\n", st);
			return;
		}
		bus = simple_strtoul(st, NULL, 0);

		if (uclass_get_device(UCLASS_SPI_FLASH, bus, &dev)) {
			printf("Failed to get device on bus %d\n", bus);
			return;
		}
		if (CONFIG_IS_ENABLED(MTD)) {
			mtd_probe_devices();
			mtd_for_each_device(mtd) {
				if (!mtd_is_partition(mtd) && mtd->dev && dev == mtd->dev)
					board_get_alt_info_sf(mtd, buf);
			}
		}
	} else {
		printf("dynamic dfu_alt_info supported only for sf\n");
		return;
	}

	env_set("dfu_alt_info", buf);
}
