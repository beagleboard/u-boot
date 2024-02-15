// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62x platforms
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Suman Anna <s-anna@ti.com>
 *
 */

#include <common.h>
#include <env.h>
#include <spl.h>
#include <init.h>
#include <video.h>
#include <splash.h>
#include <cpu_func.h>
#include <k3-ddrss.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <dm/uclass.h>
#include <net.h>
#include <asm/gpio.h>
#include <cpu_func.h>

#include <linux/sizes.h>

#include "../common/board_detect.h"
#include "../common/rtc.h"

#include "../common/k3-ddr-init.h"

DECLARE_GLOBAL_DATA_PTR;

#define AM62X_MAX_DAUGHTER_CARDS	8

/* Daughter card presence detection signals */
enum {
	AM62X_LPSK_HSE_BRD_DET,
	AM62X_LPSK_BRD_DET_COUNT,
};

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_ARM64)
static struct gpio_desc board_det_gpios[AM62X_LPSK_BRD_DET_COUNT];
#endif

/* Max number of MAC addresses that are parsed/processed per daughter card */
#define DAUGHTER_CARD_NO_OF_MAC_ADDR	8

#define board_is_am62x_skevm()  (board_ti_k3_is("AM62-SKEVM") || \
				 board_ti_k3_is("AM62B-SKEVM"))
#define board_is_am62b_p1_skevm() board_ti_k3_is("AM62B-SKEVM-P1")
#define board_is_am62x_lp_skevm()  board_ti_k3_is("AM62-LP-SKEVM")
#define board_is_am62x_sip_skevm()  board_ti_k3_is("AM62SIP-SKEVM")
#define board_is_am62x_play()	board_ti_k3_is("BEAGLEPLAY-A0-")

#if CONFIG_IS_ENABLED(SPLASH_SCREEN)
static struct splash_location default_splash_locations[] = {
	{
		.name = "sf",
		.storage = SPLASH_STORAGE_SF,
		.flags = SPLASH_STORAGE_RAW,
		.offset = 0x700000,
	},
	{
		.name		= "mmc",
		.storage	= SPLASH_STORAGE_MMC,
		.flags		= SPLASH_STORAGE_FS,
		.devpart	= "1:1",
	},
};

int splash_screen_prepare(void)
{
	return splash_source_load(default_splash_locations,
				ARRAY_SIZE(default_splash_locations));
}
#endif

int board_init(void)
{
	if (IS_ENABLED(CONFIG_BOARD_HAS_32K_RTC_CRYSTAL))
		board_rtc_init();

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	/*
	 * Just below 512MB are TF-A and OPTEE reserve regions, thus
	 * SPL/U-Boot RAM has to start below that. Leave 64MB space for
	 * all reserved memories.
	 */
	return gd->ram_size == SZ_512M ? SZ_512M - SZ_64M  : gd->ram_size;
}

#if defined(CONFIG_SPL_BUILD)
static int video_setup(void)
{
	if (CONFIG_IS_ENABLED(VIDEO)) {
		ulong addr;
		int ret;

		addr = gd->relocaddr;
		ret = video_reserve(&addr);
		if (ret)
			return ret;
		debug("Reserving %luk for video at: %08lx\n",
		      ((unsigned long)gd->relocaddr - addr) >> 10, addr);
		gd->relocaddr = addr;
	}

	return 0;
}

#define CTRLMMR_USB0_PHY_CTRL	0x43004008
#define CTRLMMR_USB1_PHY_CTRL	0x43004018
#define CORE_VOLTAGE		0x80000000

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

	video_setup();
	enable_caches();
	if (IS_ENABLED(CONFIG_SPL_SPLASH_SCREEN) && IS_ENABLED(CONFIG_SPL_BMP))
		splash_display();

	if (IS_ENABLED(CONFIG_SPL_ETH))
		/* Init DRAM size for R5/A53 SPL */
		dram_init_banksize();
}

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	if (IS_ENABLED(CONFIG_K3_INLINE_ECC))
		fixup_ddr_driver_for_ecc(spl_image);
	else
		fixup_memory_node(spl_image);
}
#endif

#ifdef CONFIG_TI_I2C_BOARD_DETECT
int do_board_detect(void)
{
	int ret;

	ret = ti_i2c_eeprom_am6_get_base(CONFIG_EEPROM_BUS_ADDRESS,
					 CONFIG_EEPROM_CHIP_ADDRESS);

	if (ret) {
		printf("EEPROM not available at %d, trying to read at %d\n",
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
#endif

#ifdef CONFIG_BOARD_LATE_INIT
static void setup_board_eeprom_env(void)
{
	char *name = "am62x_skevm";

	if (do_board_detect())
		goto invalid_eeprom;

	if (board_is_am62x_skevm())
		name = "am62x_skevm";
	else if (board_is_am62b_p1_skevm())
		name = "am62b_p1_skevm";
	else if (board_is_am62x_lp_skevm())
		name = "am62x_lp_skevm";
	else if (board_is_am62x_sip_skevm())
		name = "am62x_sip_skevm";
	else if (board_is_am62x_play())
		name = "am62x_beagleplay";
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

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_ARM64)
static const char *k3_dtbo_list[AM62X_MAX_DAUGHTER_CARDS] = {NULL};

static int init_daughtercard_det_gpio(char *gpio_name, struct gpio_desc *desc)
{
	int ret;

	memset(desc, 0, sizeof(*desc));
	ret = dm_gpio_lookup_name(gpio_name, desc);
	if (ret < 0) {
		pr_err("Failed to lookup gpio %s: %d\n", gpio_name, ret);
		return ret;
	}

	/* Request GPIO, simply re-using the name as label */
	ret = dm_gpio_request(desc, gpio_name);
	if (ret < 0) {
		pr_err("Failed to request gpio %s: %d\n", gpio_name, ret);
		return ret;
	}

	return dm_gpio_set_dir_flags(desc, GPIOD_IS_IN);
}

static int probe_daughtercards(void)
{
	struct ti_am6_eeprom ep;
	char mac_addr[DAUGHTER_CARD_NO_OF_MAC_ADDR][TI_EEPROM_HDR_ETH_ALEN];
	u8 mac_addr_cnt;
	char name_overlays[1024] = { 0 };
	int i, nb_dtbos = 0;
	int ret;

	/*
	 * Daughter card presence detection signal name to GPIO (via I2C I/O
	 * expander @ address 0x53) name and EEPROM I2C address mapping.
	 */
	const struct {
		char *gpio_name;
		u8 i2c_addr;
	} slot_map[AM62X_LPSK_BRD_DET_COUNT] = {
		{ "gpio@22_2", 0x53, },	/* AM62X_LPSK_HSE_BRD_DET */
	};

	/* Declaration of daughtercards to probe */
	const struct {
		u8 slot_index;		/* Slot the card is installed */
		char *card_name;	/* EEPROM-programmed card name */
		char *dtbo_name;	/* Device tree overlay to apply */
		u8 eth_offset;		/* ethXaddr MAC address index offset */
	} cards[] = {
		{
			AM62X_LPSK_HSE_BRD_DET,
			"SK-NAND-DC01",
			"k3-am62x-lp-sk-nand.dtbo",
			0,
		},
	};

	/*
	 * Initialize GPIO used for daughtercard slot presence detection and
	 * keep the resulting handles in local array for easier access.
	 */
	for (i = 0; i < AM62X_LPSK_BRD_DET_COUNT; i++) {
		ret = init_daughtercard_det_gpio(slot_map[i].gpio_name,
						 &board_det_gpios[i]);
		if (ret < 0)
			return ret;
	}

	memset(k3_dtbo_list, 0, sizeof(k3_dtbo_list));
	for (i = 0; i < ARRAY_SIZE(cards); i++) {
		/* Obtain card-specific slot index and associated I2C address */
		u8 slot_index = cards[i].slot_index;
		u8 i2c_addr = slot_map[slot_index].i2c_addr;
		const char *dtboname;

		/*
		 * The presence detection signal is active-low, hence skip
		 * over this card slot if anything other than 0 is returned.
		 */
		ret = dm_gpio_get_value(&board_det_gpios[slot_index]);
		if (ret < 0)
			return ret;
		else if (ret)
			continue;

		/* Get and parse the daughter card EEPROM record */
		ret = ti_i2c_eeprom_am6_get(CONFIG_EEPROM_BUS_ADDRESS, i2c_addr,
					    &ep,
					    (char **)mac_addr,
					    DAUGHTER_CARD_NO_OF_MAC_ADDR,
					    &mac_addr_cnt);

		if (ret) {
			pr_err("Reading daughtercard EEPROM at 0x%02x failed %d\n",
			       i2c_addr, ret);
			/*
			 * Even this is pretty serious let's just skip over
			 * this particular daughtercard, rather than ending
			 * the probing process altogether.
			 */
			continue;
		}

		/* Only process the parsed data if we found a match */
		if (strncmp(ep.name, cards[i].card_name, sizeof(ep.name)))
			continue;
		printf("Detected: %s rev %s\n", ep.name, ep.version);

		int j;

		for (j = 0; j < mac_addr_cnt; j++) {
			if (!is_valid_ethaddr((u8 *)mac_addr[j]))
				continue;

			eth_env_set_enetaddr_by_index("eth",
						      cards[i].eth_offset + j,
						      (uchar *)mac_addr[j]);
		}
		/* Skip if no overlays are to be added */
		if (!strlen(cards[i].dtbo_name))
			continue;

		dtboname = cards[i].dtbo_name;
		k3_dtbo_list[nb_dtbos++] = dtboname;

		/*
		 * Make sure we are not running out of buffer space by checking
		 * if we can fit the new overlay, a trailing space to be used
		 * as a separator, plus the terminating zero.
		 */
		if (strlen(name_overlays) + strlen(dtboname) + 2 >
		    sizeof(name_overlays))
			return -ENOMEM;

		/* Append to our list of overlays */
		strcat(name_overlays, dtboname);
		strcat(name_overlays, " ");
	}
	/* Apply device tree overlay(s) to the U-Boot environment, if any */
	if (strlen(name_overlays))
		return env_set("name_overlays", name_overlays);
	return 0;
}
#endif

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

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_ARM64)
		/* Check for and probe any plugged-in daughtercards */
		if (board_is_am62x_lp_skevm())
			probe_daughtercards();
#endif
	}

	return 0;
}
#endif
