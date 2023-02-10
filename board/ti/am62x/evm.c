// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62x platforms
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Suman Anna <s-anna@ti.com>
 *
 */

#include <common.h>
#include <asm/io.h>
#include <spl.h>
#include <dm/uclass.h>
#include <k3-ddrss.h>
#include <fdt_support.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <env.h>

#include "../common/board_detect.h"

#define board_is_am62x_skevm()		board_ti_k3_is("AM62-SKEVM")
#define board_is_am62x_lp_skevm()	board_ti_k3_is("AM62-LP-SKEVM")
#define board_is_am62x_play()		board_ti_k3_is("BEAGLEPLAY-A0-")
#define board_is_am62x_pocketbeagle2()	board_ti_k3_is("POCKETBEAG-A0-")

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

#if defined(CONFIG_SPL_LOAD_FIT)
int board_fit_config_name_match(const char *name)
{
	bool eeprom_read = board_ti_was_eeprom_read();

	if (!eeprom_read) {
		if (!strcmp(name, "k3-am625-r5-pocketbeagle2") || !strcmp(name, "k3-am625-pocketbeagle2"))
			return 0;
		return -1;
	}

	if (board_is_am62x_lp_skevm()) {
		if (!strcmp(name, "k3-am62x-r5-lp-sk") || !strcmp(name, "k3-am62x-lp-sk"))
			return 0;
	} else if (board_is_am62x_skevm()) {
		if (!strcmp(name, "k3-am625-r5-sk") || !strcmp(name, "k3-am625-sk"))
			return 0;
	} else if (board_is_am62x_play()) {
		if (!strcmp(name, "k3-am625-r5-beagleplay") || !strcmp(name, "k3-am625-beagleplay"))
			return 0;
	} else if (board_is_am62x_pocketbeagle2()) {
		if (!strcmp(name, "k3-am625-r5-pocketbeagle2") || !strcmp(name, "k3-am625-pocketbeagle2"))
			return 0;
	}

	return -1;
}
#endif

#if defined(CONFIG_SPL_BUILD)
#if defined(CONFIG_K3_AM64_DDRSS)
static void fixup_ddr_driver_for_ecc(struct spl_image_info *spl_image)
{
	struct udevice *dev;
	int ret;

	dram_init_banksize();

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret)
		panic("Cannot get RAM device for ddr size fixup: %d\n", ret);

	ret = k3_ddrss_ddr_fdt_fixup(dev, spl_image->fdt_addr, gd->bd);
	if (ret)
		printf("Error fixing up ddr node for ECC use! %d\n", ret);
}
#else
static void fixup_memory_node(struct spl_image_info *spl_image)
{
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	int bank;
	int ret;

	dram_init();
	dram_init_banksize();

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] =  gd->bd->bi_dram[bank].start;
		size[bank] = gd->bd->bi_dram[bank].size;
	}

	/* dram_init functions use SPL fdt, and we must fixup u-boot fdt */
	ret = fdt_fixup_memory_banks(spl_image->fdt_addr,
				     start, size, CONFIG_NR_DRAM_BANKS);
	if (ret)
		printf("Error fixing up memory node! %d\n", ret);
}
#endif

void spl_perform_fixups(struct spl_image_info *spl_image)
{
#if defined(CONFIG_K3_AM64_DDRSS)
	fixup_ddr_driver_for_ecc(spl_image);
#else
	fixup_memory_node(spl_image);
#endif
}
#endif

#ifdef CONFIG_TI_I2C_BOARD_DETECT
int do_board_detect(void)
{
	int ret;

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

int checkboard(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

	if (!do_board_detect())
		printf("Board: %s rev %s\n", ep->name, ep->version);

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
static void setup_board_eeprom_env(void)
{
	char *name = "am62x_pocketbeagle2";

	if (do_board_detect())
		goto invalid_eeprom;

	if (board_is_am62x_skevm())
		name = "am62x_skevm";
	else if (board_is_am62x_lp_skevm())
		name = "am62x_lp_skevm";
	else if (board_is_am62x_play())
		name = "am62x_play";
	else if (board_is_am62x_pocketbeagle2())
		name = "am62x_pocketbeagle2";
	else
		printf("Unidentified board claims %s in eeprom header\n",
		       board_ti_get_name());

invalid_eeprom:
	set_board_info_env_am6(name);
}

static void setup_serial(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;
	unsigned long board_serial;
	char *endp;
	char serial_string[17] = { 0 };

	if (env_get("serial#"))
		return;

	board_serial = simple_strtoul(ep->serial, &endp, 16);
	if (*endp != '\0') {
		pr_err("Error: Can't set serial# to %s\n", ep->serial);
		return;
	}

	snprintf(serial_string, sizeof(serial_string), "%016lx", board_serial);
	env_set("serial#", serial_string);
}
#endif
#endif

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_TI_I2C_BOARD_DETECT)) {
		struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

		setup_board_eeprom_env();
		setup_serial();
		/*
		 * The first MAC address for ethernet a.k.a. ethernet0 comes from
		 * efuse populated via the am654 gigabit eth switch subsystem driver.
		 * All the other ones are populated via EEPROM, hence continue with
		 * an index of 1.
		 */
		board_ti_am6_set_ethaddr(1, ep->mac_addr_cnt);
	}

	/* Default FIT boot on non-GP devices */
	if (get_device_type() != K3_DEVICE_TYPE_GP)
		env_set("boot_fit", "1");

	return 0;
}
#endif

#define CTRLMMR_USB0_PHY_CTRL	0x43004008
#define CTRLMMR_USB1_PHY_CTRL	0x43004018
#define CORE_VOLTAGE		0x80000000

#define WKUP_CTRLMMR_DBOUNCE_CFG1 0x04504084
#define WKUP_CTRLMMR_DBOUNCE_CFG2 0x04504088
#define WKUP_CTRLMMR_DBOUNCE_CFG3 0x0450408c
#define WKUP_CTRLMMR_DBOUNCE_CFG4 0x04504090
#define WKUP_CTRLMMR_DBOUNCE_CFG5 0x04504094
#define WKUP_CTRLMMR_DBOUNCE_CFG6 0x04504098

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void)
{
	u32 val;

	/* Set USB0 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB0_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB0_PHY_CTRL);

	/* Set USB1 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB1_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB1_PHY_CTRL);

	/* We have 32k crystal, so lets enable it */
	val = readl(MCU_CTRL_LFXOSC_CTRL);
	val &= ~(MCU_CTRL_LFXOSC_32K_DISABLE_VAL);
	writel(val, MCU_CTRL_LFXOSC_CTRL);
	/* Add any TRIM needed for the crystal here.. */
	/* Make sure to mux up to take the SoC 32k from the crystal */
	writel(MCU_CTRL_DEVICE_CLKOUT_LFOSC_SELECT_VAL,
	       MCU_CTRL_DEVICE_CLKOUT_32K_CTRL);

	/* Setup debounce conf registers - arbitrary values. Times are approx */
	/* 1.9ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG1, 0x1);
	/* 5ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG2, 0x5);
	/* 20ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG3, 0x14);
	/* 46ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG4, 0x18);
	/* 100ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG5, 0x1c);
	/* 156ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG6, 0x1f);

	/* Init DRAM size for R5/A53 SPL */
	dram_init_banksize();
}
#endif
