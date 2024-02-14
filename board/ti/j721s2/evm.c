// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for J721S2 EVM
 *
 * Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
 *	David Huang <d-huang@ti.com>
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
#include <dm/root.h>

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
 * Functions specific to EVM and SK designs of J721S2/AM68 family.
 */

#define board_is_j721s2_som()	board_ti_k3_is("J721S2X-PM1-SOM")

#define board_is_am68_sk_som() board_ti_k3_is("AM68-SK-SOM")

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
		printf("Board: %s rev %s\n", "J721S2X-PM1-SOM", "E1");
	else
		printf("Board: %s rev %s\n", ep->name, ep->version);

	return 0;
}

static void setup_board_eeprom_env(void)
{
	char *name = "j721s2";

	if (do_board_detect())
		goto invalid_eeprom;

	if (board_is_j721s2_som())
		name = "j721s2";
	else if (board_is_am68_sk_som())
		name = "am68-sk";
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

/*
 * Declaration of daughtercards to probe. Note that when adding more
 * cards they should be grouped by the 'i2c_addr' field to allow for a
 * more efficient probing process.
 */
static const struct {
	u8 i2c_addr;		/* I2C address of card EEPROM */
	char *card_name;	/* EEPROM-programmed card name */
	char *dtbo_name;	/* Device tree overlay to apply */
	u8 eth_offset;		/* ethXaddr MAC address index offset */
} ext_cards[] = {
	{
		0x52,
		"J7X-GESI-EXP",
		"k3-j721s2-gesi-exp-board.dtbo",
		1,		/* Start populating from eth1addr */
	},
};

#define DAUGHTER_CARD_NO_OF_MAC_ADDR	5
static bool daughter_card_detect_flags[ARRAY_SIZE(ext_cards)];

static int probe_daughtercards(void)
{
	char mac_addr[DAUGHTER_CARD_NO_OF_MAC_ADDR][TI_EEPROM_HDR_ETH_ALEN];
	bool eeprom_read_success;
	struct ti_am6_eeprom ep;
	u8 previous_i2c_addr;
	u8 mac_addr_cnt;
	int i;
	int ret;

	/* Mark previous I2C address variable as not populated */
	previous_i2c_addr = 0xff;

	/* No EEPROM data was read yet */
	eeprom_read_success = false;

	/* Iterate through list of daughtercards */
	for (i = 0; i < ARRAY_SIZE(ext_cards); i++) {
		/* Obtain card-specific I2C address */
		u8 i2c_addr = ext_cards[i].i2c_addr;

		/* Read card EEPROM if not already read previously */
		if (i2c_addr != previous_i2c_addr) {
			/* Store I2C address so we can avoid reading twice */
			previous_i2c_addr = i2c_addr;

			/* Get and parse the daughter card EEPROM record */
			ret = ti_i2c_eeprom_am6_get(CONFIG_EEPROM_BUS_ADDRESS,
						    i2c_addr,
						    &ep,
						    (char **)mac_addr,
						    DAUGHTER_CARD_NO_OF_MAC_ADDR,
						    &mac_addr_cnt);
			if (ret) {
				debug("%s: No daughtercard EEPROM at 0x%02x found %d\n",
				      __func__, i2c_addr, ret);
				eeprom_read_success = false;
				/* Skip to the next daughtercard to probe */
				continue;
			}

			/* EEPROM read successful, okay to further process. */
			eeprom_read_success = true;
		}

		/* Only continue processing if EEPROM data was read */
		if (!eeprom_read_success)
			continue;

		/* Only process the parsed data if we found a match */
		if (strncmp(ep.name, ext_cards[i].card_name, sizeof(ep.name)))
			continue;

		printf("Detected: %s rev %s\n", ep.name, ep.version);
		daughter_card_detect_flags[i] = true;

		if (!IS_ENABLED(CONFIG_SPL_BUILD)) {
			int j;
			/*
			 * Populate any MAC addresses from daughtercard into the U-Boot
			 * environment, starting with a card-specific offset so we can
			 * have multiple ext_cards contribute to the MAC pool in a well-
			 * defined manner.
			 */
			for (j = 0; j < mac_addr_cnt; j++) {
				if (!is_valid_ethaddr((u8 *)mac_addr[j]))
					continue;

				eth_env_set_enetaddr_by_index("eth", ext_cards[i].eth_offset + j,
							      (uchar *)mac_addr[j]);
			}
		}
	}

	if (!IS_ENABLED(CONFIG_SPL_BUILD)) {
		char name_overlays[1024] = { 0 };

		for (i = 0; i < ARRAY_SIZE(ext_cards); i++) {
			if (!daughter_card_detect_flags[i])
				continue;

			/* Skip if no overlays are to be added */
			if (!strlen(ext_cards[i].dtbo_name))
				continue;

			/*
			 * Make sure we are not running out of buffer space by checking
			 * if we can fit the new overlay, a trailing space to be used
			 * as a separator, plus the terminating zero.
			 */
			if (strlen(name_overlays) + strlen(ext_cards[i].dtbo_name) + 2 >
			    sizeof(name_overlays))
				return -ENOMEM;

			/* Append to our list of overlays */
			strcat(name_overlays, ext_cards[i].dtbo_name);
			strcat(name_overlays, " ");
		}

		/* Apply device tree overlay(s) to the U-Boot environment, if any */
		if (strlen(name_overlays))
			return env_set("name_overlays", name_overlays);
	}

	return 0;
}
#endif

/*
 * This function chooses the right dtb based on the board name read from
 * EEPROM if the EEPROM is programmed. Also, by default the boot chooses
 * the EVM DTB if there is no EEPROM is programmed or not detected.
 */
#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	bool eeprom_read = board_ti_was_eeprom_read();

	if (!eeprom_read || board_is_j721s2_som()) {
		if (!strcmp(name, "k3-j721s2-common-proc-board"))
			return 0;
	} else if (!eeprom_read || board_is_am68_sk_som()) {
		if (!strcmp(name, "k3-am68-sk-base-board"))
			return 0;
	}

	return -1;
}
#endif

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_TI_I2C_BOARD_DETECT)) {
		setup_board_eeprom_env();
		setup_serial();
		probe_daughtercards();
	}

	return 0;
}

ofnode cadence_qspi_get_subnode(struct udevice *dev)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD) &&
	    IS_ENABLED(CONFIG_TARGET_J721S2_R5_EVM)) {
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

	if (IS_ENABLED(CONFIG_ESM_PMIC) && !board_is_am68_sk_som()) {
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(pmic_esm),
						  &dev);
		if (ret)
			printf("ESM PMIC init failed: %d\n", ret);
	}
}

/* Support for the various EVM / SK families */
#if defined(CONFIG_SPL_OF_LIST) && defined(CONFIG_TI_I2C_BOARD_DETECT)
void do_dt_magic(void)
{
	int ret, rescan, mmc_dev = -1;
	static struct mmc *mmc;

	do_board_detect();

	/*
	 * Board detection has been done.
	 * Let us see if another dtb wouldn't be a better match
	 * for our board
	 */
	if (IS_ENABLED(CONFIG_CPU_V7R)) {
		ret = fdtdec_resetup(&rescan);
		if (!ret && rescan) {
			dm_uninit();
			dm_init_and_scan(true);
		}
	}

	/*
	 * Because of multi DTB configuration, the MMC device has
	 * to be re-initialized after reconfiguring FDT inorder to
	 * boot from MMC. Do this when boot mode is MMC and ROM has
	 * not loaded SYSFW.
	 */
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC1:
		mmc_dev = 0;
		break;
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		mmc_dev = 1;
		break;
	}

	if (mmc_dev > 0 && !check_rom_loaded_sysfw()) {
		ret = mmc_init_device(mmc_dev);
		if (!ret) {
			mmc = find_mmc_device(mmc_dev);
			if (mmc) {
				ret = mmc_init(mmc);
				if (ret)
					printf("mmc init failed with error: %d\n", ret);
			}
		}
	}
}
#endif

#ifdef CONFIG_SPL_BUILD
void board_init_f(ulong dummy)
{
	k3_spl_init();
#if defined(CONFIG_SPL_OF_LIST) && defined(CONFIG_TI_I2C_BOARD_DETECT)
	do_dt_magic();
#endif
	k3_mem_init();
}
#endif
