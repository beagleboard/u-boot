// SPDX-License-Identifier: GPL-2.0+
/*
 * https://www.beagleboard.org/boards/pocketbeagle-2
 *
 * Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com/
 * Copyright (C) 2025 Robert Nelson, BeagleBoard.org Foundation
 */

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>
#include <asm/arch/k3-ddr.h>
#include <dm.h>
#include <i2c.h>

#include "../../ti/common/board_detect.h"

#include "pocketbeagle2-ddr-data.h"
#include "../../phytec/common/k3/k3_ddrss_patch.h"

enum {
	EEPROM_RAM_SIZE_512MB = 0,
	EEPROM_RAM_SIZE_1GB = 1
};

DECLARE_GLOBAL_DATA_PTR;

int do_eeprom_read(void)
{
	int ret;

	if (board_ti_was_eeprom_read())
		return 0;

	ret = ti_i2c_eeprom_am6_get_base(CONFIG_EEPROM_BUS_ADDRESS,
					 CONFIG_EEPROM_CHIP_ADDRESS);
	if (ret) {
		printf("EEPROM not available at 0x%02x, trying to read at 0x%02x\n",
		       CONFIG_EEPROM_CHIP_ADDRESS, CONFIG_EEPROM_CHIP_ADDRESS + 1);
		ret = ti_i2c_eeprom_am6_get_base(CONFIG_EEPROM_BUS_ADDRESS,
						 CONFIG_EEPROM_CHIP_ADDRESS + 1);
		if (ret)
			pr_err("Reading on-board EEPROM at 0x%02x failed %d\n",
			       CONFIG_EEPROM_CHIP_ADDRESS + 1, ret);
	}

	return ret;
}

static u8 pocketbeagle2_get_am62_ddr_size_default(void)
{
	int ret;
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

	ret = do_eeprom_read();
	if (!ret) {
		if (ep->name[0] == 0x50) {
			printf("EEPROM: [%s]\n", ep->name);
		}
		/*
		 * POCKETBEAGL2A00 (am6232 512MB)
		 * POCKETBEAGL2A10 (am625 512MB)
		 * POCKETBEAGL2A1I (am625 1GB)
		 */
		if (!strncmp(&ep->name[11], "2A1I", 4)) {
			puts("EEPROM: EEPROM_RAM_SIZE_1GB\n");
			return EEPROM_RAM_SIZE_1GB;
		}
	}

	/* Default DDR size is 512MB */
	return EEPROM_RAM_SIZE_512MB;
}

int dram_init(void)
{
	u8 ram_size;

	if (!IS_ENABLED(CONFIG_CPU_V7R))
		return fdtdec_setup_mem_size_base();

	ram_size = pocketbeagle2_get_am62_ddr_size_default();

	switch (ram_size) {
	case EEPROM_RAM_SIZE_1GB:
		gd->ram_size = 0x40000000;
		break;
	default:
		gd->ram_size = 0x20000000;
	}

	return 0;
}

int dram_init_banksize(void)
{
	u8 ram_size;

	memset(gd->bd->bi_dram, 0, sizeof(gd->bd->bi_dram[0]) * CONFIG_NR_DRAM_BANKS);

	if (!IS_ENABLED(CONFIG_CPU_V7R))
		return fdtdec_setup_memory_banksize();

	ram_size = pocketbeagle2_get_am62_ddr_size_default();
	switch (ram_size) {
	case EEPROM_RAM_SIZE_1GB:
		gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[0].size = 0x40000000;
		gd->ram_size = 0x40000000;
		break;
	default:
		/* Continue with default 512MB setup */
		gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[0].size = 0x20000000;
		gd->ram_size = 0x20000000;
	}

	return 0;
}

#if defined(CONFIG_K3_DDRSS)
int update_ddrss_timings(void)
{
	int ret;
	u8 ram_size;
	struct ddrss *ddr_patch = NULL;
	void *fdt = (void *)gd->fdt_blob;

	ram_size = pocketbeagle2_get_am62_ddr_size_default();
	switch (ram_size) {
	case EEPROM_RAM_SIZE_512MB:
		ddr_patch = NULL;
		break;
	case EEPROM_RAM_SIZE_1GB:
		ddr_patch = &phycore_ddrss_data[POCKETBEAGLE2_1GB];
		break;
	default:
		break;
	}

	/* Nothing to patch */
	if (!ddr_patch)
		return 0;

	printf("Applying DDRSS timings patch for ram_size %d\n", ram_size);

	ret = fdt_apply_ddrss_timings_patch(fdt, ddr_patch);
	if (ret < 0) {
		printf("Failed to apply ddrs timings patch %d\n", ret);
		return ret;
	}

	return 0;
}
#else
int update_ddrss_timings(void)
{
	puts("update_ddrss_timings (on A53 don't change anything)\n");
	return 0;
}
#endif

int do_board_detect(void)
{
	int ret;
	void *fdt = (void *)gd->fdt_blob;
	int bank;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	dram_init();
	dram_init_banksize();

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = gd->bd->bi_dram[bank].start;
		size[bank] = gd->bd->bi_dram[bank].size;
	}

	ret = fdt_fixup_memory_banks(fdt, start, size, CONFIG_NR_DRAM_BANKS);
	if (ret)
		return ret;

	return update_ddrss_timings();
}

#if IS_ENABLED(CONFIG_BOARD_LATE_INIT)
int board_late_init(void)
{
	char fdtfile[50];

	snprintf(fdtfile, sizeof(fdtfile), "%s.dtb", CONFIG_DEFAULT_DEVICE_TREE);

	env_set("fdtfile", fdtfile);

	return 0;
}
#endif
