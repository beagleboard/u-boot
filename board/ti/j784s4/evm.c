// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for J784S4 EVM
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Hari Nagalla <hnagalla@ti.com>
 *
 */

#include <common.h>
#include <env.h>
#include <fdt_support.h>
#include <generic-phy.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <net.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <spl.h>
#include <asm/arch/sys_proto.h>
#include <dm.h>
#include <dm/uclass-internal.h>

#include "../common/board_detect.h"
#include "../common/k3-ddr-init.h"

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

phys_size_t board_get_usable_ram_top(phys_size_t total_size)
{
#ifdef CONFIG_PHYS_64BIT
	/* Limit RAM used by U-Boot to the DDR low region */
	if (gd->ram_top > 0x100000000)
		return 0x100000000;
#endif

	return gd->ram_top;
}

/* Enables the spi-nand dts node, if onboard mux is set to spinand */
static void __maybe_unused detect_enable_spinand(void *blob)
{
	if (IS_ENABLED(CONFIG_DM_GPIO) && IS_ENABLED(CONFIG_OF_LIBFDT)) {
		struct gpio_desc desc = {0};
		char *ospi_mux_sel_gpio = "6";
		int nand_offset, nor_offset;

		if (dm_gpio_lookup_name(ospi_mux_sel_gpio, &desc))
			return;

		if (dm_gpio_request(&desc, ospi_mux_sel_gpio))
			return;

		if (dm_gpio_set_dir_flags(&desc, GPIOD_IS_IN))
			return;

		nand_offset = fdt_node_offset_by_compatible(blob, -1, "spi-nand");
		nor_offset = fdt_node_offset_by_compatible(blob,
							   fdt_parent_offset(blob, nand_offset),
							   "jedec,spi-nor");

		if (dm_gpio_get_value(&desc)) {
			fdt_status_okay(blob, nand_offset);
			fdt_del_node(blob, nor_offset);
		} else {
			fdt_del_node(blob, nand_offset);
		}
	}
}

#if defined(CONFIG_SPL_BUILD)
void spl_perform_fixups(struct spl_image_info *spl_image)
{
	detect_enable_spinand(spl_image->fdt_addr);
	if (IS_ENABLED(CONFIG_K3_INLINE_ECC))
		fixup_ddr_driver_for_ecc(spl_image);
}
#endif

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret;

	ret = fdt_fixup_msmc_ram(blob, "/bus@100000", "sram@70000000");
	if (ret < 0)
		ret = fdt_fixup_msmc_ram(blob, "/interconnect@100000",
					 "sram@70000000");
	if (ret)
		printf("%s: fixing up msmc ram failed %d\n", __func__, ret);

	detect_enable_spinand(blob);

	return ret;
}
#endif

#ifdef CONFIG_TI_I2C_BOARD_DETECT

/*
* Functions specific to EVM and SK designs of J784S4/AM69 family.
*/
#define board_is_j784s4_evm()	board_ti_k3_is("J784S4X-EVM")

#define board_is_am69_sk()	board_ti_k3_is("AM69-SK")

int do_board_detect(void)
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

int checkboard(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

	if (do_board_detect())
		/* EEPROM not populated */
		printf("Board: %s rev %s\n", "J784S4-EVM", "E1");
	else
		printf("Board: %s rev %s\n", ep->name, ep->version);

	return 0;
}

static void setup_board_eeprom_env(void)
{
	char *name = "j784s4";

	if (do_board_detect())
		goto invalid_eeprom;

	if (board_is_am69_sk())
		name = "am69-sk";
	else if (!board_is_j784s4_evm())
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

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	bool eeprom_read = board_ti_was_eeprom_read();

	if (!eeprom_read || board_is_j784s4_evm()) {
		if ((!strcmp(name, "k3-j784s4-evm")) || (!strcmp(name, "k3-j784s4-r5-evm")))
				return 0;
	} else if (!eeprom_read || board_is_am69_sk()) {
		if ((!strcmp(name, "k3-am69-sk")) || (!strcmp(name, "k3-am69-r5-sk")))
				return 0;
	}

	return -1;
}
#endif

#endif

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_TI_I2C_BOARD_DETECT)) {
		setup_board_eeprom_env();
		setup_serial();
	}

	return 0;
}

ofnode cadence_qspi_get_subnode(struct udevice *dev)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD) &&
	    IS_ENABLED(CONFIG_TARGET_J784S4_R5_EVM)) {
		if (spl_boot_device() == BOOT_DEVICE_SPINAND)
			return ofnode_by_compatible(dev_ofnode(dev), "spi-nand");
	}

	return dev_read_first_subnode(dev);
}

void spl_board_init(void)
{
	struct udevice *dev;
	int ret;

	if (IS_ENABLED(CONFIG_ESM_K3)) {
		ret = uclass_get_device_by_name(UCLASS_MISC, "esm@700000",
						&dev);
		if (ret)
			printf("MISC init for esm@700000 failed: %d\n", ret);

		ret = uclass_get_device_by_name(UCLASS_MISC, "esm@40800000",
						&dev);
		if (ret)
			printf("MISC init for esm@40800000 failed: %d\n", ret);

		ret = uclass_get_device_by_name(UCLASS_MISC, "esm@42080000",
						&dev);
		if (ret)
			printf("MISC init for esm@42080000 failed: %d\n", ret);
	}

	if (IS_ENABLED(CONFIG_ESM_PMIC)) {
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(pmic_esm),
						  &dev);
		if (ret)
			printf("ESM PMIC init failed: %d\n", ret);
	}
}
