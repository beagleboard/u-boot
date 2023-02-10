/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for K3 AM625 SoC family
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Suman Anna <s-anna@ti.com>
 */

#ifndef __CONFIG_AM625_EVM_H
#define __CONFIG_AM625_EVM_H

#include <linux/sizes.h>
#include <config_distro_bootcmd.h>
#include <environment/ti/mmc.h>
#include <environment/ti/k3_dfu.h>

/* DDR Configuration */
#define CONFIG_SYS_SDRAM_BASE1		0x880000000
#define CONFIG_SYS_BOOTM_LEN            SZ_64M

#ifdef CONFIG_SYS_K3_SPL_ATF
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"tispl.bin"
#endif

#if defined(CONFIG_TARGET_AM625_A53_EVM)
#define CONFIG_SPL_MAX_SIZE		SZ_1M
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SPL_TEXT_BASE + SZ_4M)
#else
#define CONFIG_SPL_MAX_SIZE		CONFIG_SYS_K3_MAX_DOWNLODABLE_IMAGE_SIZE
/*
 * Maximum size in memory allocated to the SPL BSS. Keep it as tight as
 * possible (to allow the build to go through), as this directly affects
 * our memory footprint. The less we use for BSS the more we have available
 * for everything else.
 */
#define CONFIG_SPL_BSS_MAX_SIZE		0x3000
/*
 * Link BSS to be within SPL in a dedicated region located near the top of
 * the MCU SRAM, this way making it available also before relocation. Note
 * that we are not using the actual top of the MCU SRAM as there is a memory
 * location allocated for Device Manager data and some memory filled in by
 * the boot ROM that we want to read out without any interference from the
 * C context.
 */
#define CONFIG_SPL_BSS_START_ADDR	(0x43c3e000 -\
					 CONFIG_SPL_BSS_MAX_SIZE)
/* Set the stack right below the SPL BSS section */
#define CONFIG_SYS_INIT_SP_ADDR         0x43c3a7f0
/* Configure R5 SPL post-relocation malloc pool in DDR */
#define CONFIG_SYS_SPL_MALLOC_START    0x84000000
#define CONFIG_SYS_SPL_MALLOC_SIZE     SZ_16M
#endif

#define PARTS_DEFAULT \
	/* Linux partitions */ \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=rootfs,start=0,size=-,uuid=${uuid_gpt_rootfs}\0" \
	/* Android partitions */ \
	"partitions_android=" \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=bootloader,start=5M,size=8M,uuid=${uuid_gpt_bootloader};" \
	"name=tiboot3,start=4M,size=1M,uuid=${uuid_gpt_tiboot3};" \
	"name=uboot-env,start=13M,size=512K,uuid=${uuid_gpt_env};" \
	"name=misc,start=13824K,size=512K,uuid=${uuid_gpt_misc};" \
	"name=boot_a,size=40M,uuid=${uuid_gpt_boot_a};" \
	"name=boot_b,size=40M,uuid=${uuid_gpt_boot_b};" \
	"name=dtbo_a,size=8M,uuid=${uuid_gpt_dtbo_a};" \
	"name=dtbo_b,size=8M,uuid=${uuid_gpt_dtbo_b};" \
	"name=vbmeta_a,size=64K,uuid=${uuid_gpt_vbmeta_a};" \
	"name=vbmeta_b,size=64K,uuid=${uuid_gpt_vbmeta_b};" \
	"name=super,size=4608M,uuid=${uuid_gpt_super};" \
	"name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
	"name=persist,size=32M,uuid=${uuid_gpt_persist};" \
	"name=userdata,size=-,uuid=${uuid_gpt_userdata}\0"

/* U-Boot general configuration */
#define EXTRA_ENV_AM625_BOARD_SETTINGS					\
	"default_device_tree=" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0"	\
	"findfdt="							\
		"setenv name_fdt ${default_device_tree};"		\
		"if test $board_name = am62x_skevm; then "		\
			"setenv name_fdt k3-am625-sk.dtb; fi;"		\
		"if test $board_name = am62x_lp_skevm; then "		\
			"setenv name_fdt k3-am62x-lp-sk.dtb; fi;"	\
		"if test $board_name = am62x_play; then "		\
			"setenv name_fdt k3-am625-beagleplay.dtb; fi;"	\
		"if test $board_name = am62x_pocketbeagle2; then "	\
			"setenv name_fdt k3-am625-pocketbeagle2.dtb; fi;" \
		"setenv fdtfile ${name_fdt}\0"				\
	"name_kern=Image\0"						\
	"console=ttyS2,115200n8\0"					\
	"args_all=setenv optargs ${optargs} "				\
		"earlycon=ns16550a,mmio32,0x02800000 ${mtdparts}\0"	\
	"run_kern=booti ${loadaddr} ${rd_spec} ${fdtaddr}\0"

/* U-Boot MMC-specific configuration */
#define EXTRA_ENV_AM625_BOARD_SETTINGS_MMC				\
	"boot=mmc\0"							\
	"mmcdev=1\0"							\
	"bootpart=1:2\0"						\
	"bootdir=/boot\0"						\
	"rd_spec=-\0"							\
	"init_mmc=run args_all args_mmc\0"				\
	"get_fdt_mmc=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${name_fdt}\0" \
	"get_overlay_mmc="						\
		"fdt address ${fdtaddr};"				\
		"fdt resize 0x100000;"					\
		"for overlay in $name_overlays;"			\
		"do;"							\
		"load mmc ${bootpart} ${dtboaddr} ${bootdir}/${overlay} && "	\
		"fdt apply ${dtboaddr};"				\
		"done;\0"						\
	"get_kern_mmc=load mmc ${bootpart} ${loadaddr} "		\
		"${bootdir}/${name_kern}\0"				\
	"get_fit_mmc=load mmc ${bootpart} ${addr_fit} "			\
		"${bootdir}/${name_fit}\0"				\
	"partitions=" PARTS_DEFAULT

#define EXTRA_ENV_AM625_BOARD_SETTING_USBMSC				\
	"args_usb=run finduuid;setenv bootargs console=${console} "	\
		"${optargs} "						\
		"root=PARTUUID=${uuid} rw "				\
		"rootfstype=${mmcrootfstype}\0"				\
	"init_usb=run args_all args_usb\0"				\
	"get_fdt_usb=load usb ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}\0"	\
	"get_overlay_usb="						\
		"fdt address ${fdtaddr};"				\
		"fdt resize 0x100000;"					\
		"for overlay in $name_overlays;"			\
		"do;"							\
		"load usb ${bootpart} ${dtboaddr} ${bootdir}/${overlay} && "	\
		"fdt apply ${dtboaddr};"				\
		"done;\0"						\
	"get_kern_usb=load usb ${bootpart} ${loadaddr} "		\
		"${bootdir}/${name_kern}\0"				\
	"get_fit_usb=load usb ${bootpart} ${addr_fit} "			\
		"${bootdir}/${name_fit}\0"				\
	"usbboot=setenv boot usb;"					\
		"setenv bootpart 0:2;"					\
		"usb start;"						\
		"run findfdt;"						\
		"run init_usb;"						\
		"run get_kern_usb;"					\
		"run get_fdt_usb;"					\
		"run run_kern\0"


#define EXTRA_ENV_AM625_BEAGPLAY_PRODUCTION_BOARD_SETTINGS			\
	"play_eeprom_i2c_dev=0;\0"						\
	"play_eeprom_i2c_addr=0x50;\0"						\
	"play_eeprom_wp_gpio=10;\0"						\
	"play_eeprom_dump=i2c dev ${play_eeprom_i2c_dev}; "			\
		"i2c md ${play_eeprom_i2c_addr} 0x00.2 40; "			\
		"\0"								\
	"play_eeprom_production_program=i2c dev ${play_eeprom_i2c_dev}; "	\
		"i2c md ${play_eeprom_i2c_addr} 0x00.2 40; "			\
		"gpio clear ${play_eeprom_wp_gpio}; "				\
		"i2c mw ${play_eeprom_i2c_addr} 0x00.2 aa; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x01.2 55; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x02.2 33; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x03.2 ee; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x04.2 01; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x05.2 37; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x06.2 00; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x07.2 10; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x08.2 2e; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x09.2 00; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0a.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0b.2 45; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0c.2 41; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0d.2 47; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0e.2 4c; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0f.2 45; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x10.2 50; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x11.2 4c; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x12.2 41; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x13.2 59; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x14.2 2d; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x15.2 41; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x16.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x17.2 2d; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x18.2 00; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x19.2 00; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1a.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1b.2 32; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1c.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1d.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1e.2 37; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1f.2 38; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x20.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x21.2 31; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x22.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x23.2 32; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x24.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x25.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x26.2 30; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x27.2 31; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x28.2 36; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x29.2 34; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2a.2 57; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2b.2 57; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2c.2 32; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2d.2 33; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2e.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2f.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x30.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x31.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x32.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x33.2 42; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x34.2 53; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x35.2 53; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x36.2 53; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x37.2 53; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x38.2 11; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x39.2 02; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3a.2 00; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3b.2 60; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3c.2 7d; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3d.2 fe; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3e.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3f.2 ff; "			\
		"gpio set ${play_eeprom_wp_gpio}; "				\
		"i2c md ${play_eeprom_i2c_addr} 0x00.2 40; "			\
		"\0"								\
	"play_eeprom_erase=i2c dev ${play_eeprom_i2c_dev}; "			\
		"i2c md ${play_eeprom_i2c_addr} 0x00.2 40; "			\
		"gpio clear ${play_eeprom_wp_gpio}; "				\
		"i2c mw ${play_eeprom_i2c_addr} 0x00.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x01.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x02.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x03.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x04.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x05.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x06.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x07.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x08.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x09.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0a.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0b.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0c.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0d.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0e.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x0f.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x10.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x11.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x12.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x13.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x14.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x15.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x16.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x17.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x18.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x19.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1a.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1b.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1c.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1d.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1e.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x1f.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x20.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x21.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x22.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x23.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x24.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x25.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x26.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x27.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x28.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x29.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2a.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2b.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2c.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2d.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2e.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x2f.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x30.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x31.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x32.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x33.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x34.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x35.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x36.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x37.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x38.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x39.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3a.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3b.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3c.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3d.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3e.2 ff; "			\
		"i2c mw ${play_eeprom_i2c_addr} 0x3f.2 ff; "			\
		"gpio set ${play_eeprom_wp_gpio}; "				\
		"i2c md ${play_eeprom_i2c_addr} 0x00.2 40; "			\
		"\0"								\
	"emmc_erase_boot0=mmc dev 0 1; "					\
		"mmc erase 0 0x2400; "						\
		"\0"								\

#define BOOTENV_DEV_LINUX(devtypeu, devtypel, instance) \
	"bootcmd_linux=" \
		"if test \"${android_boot}\" -eq 0; then;" \
			"run findfdt; run envboot; run init_${boot};" \
			"if test ${boot_fit} -eq 1; then;" \
				"run get_fit_${boot}; run get_fit_${boot}; run get_overlaystring; run run_fit;"\
			"else;" \
				"run get_kern_${boot}; run get_fdt_${boot}; run get_overlay_${boot}; run run_kern;" \
			"fi;" \
		"fi\0"

#define BOOTENV_DEV_NAME_LINUX(devtypeu, devtypel, instance)	\
		"linux "

#if 0

/* ANDROID BOOT */
#ifndef BOOT_PARTITION
#define BOOT_PARTITION "boot"
#endif

#ifndef CONTROL_PARTITION
#define CONTROL_PARTITION "misc"
#endif

#if defined(CONFIG_CMD_AVB)
#define AVB_VERIFY_CHECK \
	"if test \"${force_avb}\" -eq 1; then " \
		"if run avb_verify; then " \
			"echo AVB verification OK.;" \
			"setenv bootargs \"$bootargs $avb_bootargs\";" \
		"else " \
			"echo AVB verification failed.;" \
		"exit; fi;" \
	"else " \
		"setenv bootargs \"$bootargs androidboot.verifiedbootstate=orange\";" \
		"echo Running without AVB...; "\
	"fi;"

#define AVB_VERIFY_CMD "avb_verify=avb init ${mmcdev}; avb verify $slot_suffix;\0"
#else
#define AVB_VERIFY_CHECK ""
#define AVB_VERIFY_CMD ""
#endif

#if defined(CONFIG_CMD_AB_SELECT)
#define ANDROIDBOOT_GET_CURRENT_SLOT_CMD "get_current_slot=" \
	"if part number mmc ${mmcdev} " CONTROL_PARTITION " control_part_number; " \
	"then " \
		"echo " CONTROL_PARTITION \
			" partition number:${control_part_number};" \
		"ab_select current_slot mmc ${mmcdev}:${control_part_number};" \
	"else " \
		"echo " CONTROL_PARTITION " partition not found;" \
	"fi;\0"

#define AB_SELECT_SLOT \
	"run get_current_slot; " \
	"if test -e \"${current_slot}\"; " \
	"then " \
		"setenv slot_suffix _${current_slot}; " \
	"else " \
		"echo current_slot not found;" \
		"exit;" \
	"fi;"

#define AB_SELECT_ARGS \
	"setenv bootargs_ab androidboot.slot_suffix=${slot_suffix}; " \
	"echo A/B cmdline addition: ${bootargs_ab};" \
	"setenv bootargs ${bootargs} ${bootargs_ab};"

#define AB_BOOTARGS " androidboot.force_normal_boot=1"
#define RECOVERY_PARTITION "boot"
#else
#define AB_SELECT_SLOT ""
#define AB_SELECT_ARGS " "
#define ANDROIDBOOT_GET_CURRENT_SLOT_CMD ""
#define AB_BOOTARGS " "
#define RECOVERY_PARTITION "recovery"
#endif

/*
 * Prepares complete device tree blob for current board (for Android boot).
 *
 * Boot image or recovery image should be loaded into $loadaddr prior to running
 * these commands. The logic of these commnads is next:
 *
 *   1. Read correct DTB for current SoC/board from boot image in $loadaddr
 *      to $fdtaddr
 *   2. Merge all needed DTBO for current board from 'dtbo' partition into read
 *      DTB
 *   3. User should provide $fdtaddr as 3rd argument to 'bootm'
 */
#define PREPARE_FDT \
	"echo Preparing FDT...; " \
	"if test $board_name = am62x_skevm; then " \
		"echo \"  Reading DTB for am62x_skevm...\"; " \
		"setenv dtb_index 0;" \
	"elif test $board_name = am62x_lp_skevm; then " \
		"echo \"  Reading DTB for am62x_lp_skevm...\"; " \
		"setenv dtb_index 1;" \
	"else " \
		"echo Error: Android boot is not supported for $board_name; " \
		"exit; " \
	"fi; " \
	"abootimg get dtb --index=$dtb_index dtb_start dtb_size; " \
	"cp.b $dtb_start $fdt_addr_r $dtb_size; " \
	"fdt addr $fdt_addr_r $fdt_size; " \
	"part start mmc ${mmcdev} dtbo${slot_suffix} dtbo_start; " \
	"part size mmc ${mmcdev} dtbo${slot_suffix} dtbo_size; " \
	"mmc read ${dtboaddr} ${dtbo_start} ${dtbo_size}; " \
	"echo \"  Applying DTBOs...\"; " \
	"adtimg addr $dtboaddr; " \
	"dtbo_idx=''; " \
	"for index in $dtbo_index; do " \
		"adtimg get dt --index=$index dtbo_addr; " \
		"fdt resize; " \
		"fdt apply $dtbo_addr; " \
		"if test $dtbo_idx = ''; then " \
			"dtbo_idx=${index}; " \
		"else " \
			"dtbo_idx=${dtbo_idx},${index}; " \
		"fi; " \
	"done; " \
	"setenv bootargs \"$bootargs androidboot.dtbo_idx=$dtbo_idx \"; "


#define BOOT_CMD "bootm ${loadaddr} ${loadaddr} ${fdt_addr_r};"

#define BOOTENV_DEV_FASTBOOT(devtypeu, devtypel, instance) \
	"bootcmd_fastboot=" \
		"if test \"${android_boot}\" -eq 1; then;" \
			"setenv run_fastboot 0;" \
			"if gpt verify mmc ${mmcdev} ${partitions}; then; " \
			"else " \
				"echo Broken MMC partition scheme;" \
				"setenv run_fastboot 1;" \
			"fi; " \
			"if test \"${run_fastboot}\" -eq 0; then " \
				"if bcb load " __stringify(CONFIG_FASTBOOT_FLASH_MMC_DEV) " " \
				CONTROL_PARTITION "; then " \
					"if bcb test command = bootonce-bootloader; then " \
						"echo BCB: Bootloader boot...; " \
						"bcb clear command; bcb store; " \
						"setenv run_fastboot 1;" \
					"elif bcb test command = boot-fastboot; then " \
						"echo BCB: fastboot userspace boot...; " \
						"setenv force_recovery 1;" \
					"fi; " \
				"else " \
					"echo Warning: BCB is corrupted or does not exist; " \
				"fi;" \
			"fi;" \
			"if test \"${run_fastboot}\" -eq 1; then " \
				"echo Running Fastboot...;" \
				"fastboot " __stringify(CONFIG_FASTBOOT_USB_DEV) "; " \
			"fi;" \
		"fi\0"

#define BOOTENV_DEV_NAME_FASTBOOT(devtypeu, devtypel, instance)	\
		"fastboot "

#define BOOTENV_DEV_RECOVERY(devtypeu, devtypel, instance) \
	"bootcmd_recovery=" \
		"if test \"${android_boot}\" -eq 1; then;" \
			"setenv run_recovery 0;" \
			"if bcb load " __stringify(CONFIG_FASTBOOT_FLASH_MMC_DEV) " " \
			CONTROL_PARTITION "; then " \
				"if bcb test command = boot-recovery; then; " \
					"echo BCB: Recovery boot...; " \
					"setenv run_recovery 1;" \
				"fi;" \
			"else " \
				"echo Warning: BCB is corrupted or does not exist; " \
			"fi;" \
			"if test \"${skip_recovery}\" -eq 1; then " \
				"echo Recovery skipped by environment;" \
				"setenv run_recovery 0;" \
			"fi;" \
			"if test \"${force_recovery}\" -eq 1; then " \
				"echo Recovery forced by environment;" \
				"setenv run_recovery 1;" \
			"fi;" \
			"if test \"${run_recovery}\" -eq 1; then " \
				"echo Running Recovery...;" \
				"mmc dev ${mmcdev};" \
				"setenv bootargs \"${bootargs} androidboot.serialno=${serial#}\";" \
				AB_SELECT_SLOT \
				AB_SELECT_ARGS \
				AVB_VERIFY_CHECK \
				"part start mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_start;" \
				"part size mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_size;" \
				"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
					PREPARE_FDT \
					"echo Running Android Recovery...;" \
					BOOT_CMD \
				"fi;" \
				"echo Failed to boot Android...;" \
				"reset;" \
			"fi;" \
		"fi\0"

#define BOOTENV_DEV_NAME_RECOVERY(devtypeu, devtypel, instance)	\
		"recovery "

#define BOOTENV_DEV_SYSTEM(devtypeu, devtypel, instance) \
	"bootcmd_system=" \
		"if test \"${android_boot}\" -eq 1; then;" \
			"echo Loading Android " BOOT_PARTITION " partition...;" \
			"mmc dev ${mmcdev};" \
			"setenv bootargs ${bootargs} androidboot.serialno=${serial#};" \
			AB_SELECT_SLOT \
			AB_SELECT_ARGS \
			AVB_VERIFY_CHECK \
			"part start mmc ${mmcdev} " BOOT_PARTITION "${slot_suffix} boot_start;" \
			"part size mmc ${mmcdev} " BOOT_PARTITION "${slot_suffix} boot_size;" \
			"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
				PREPARE_FDT \
				"setenv bootargs \"${bootargs} " AB_BOOTARGS "\"  ; " \
				"echo Running Android...;" \
				BOOT_CMD \
			"fi;" \
			"echo Failed to boot Android...;" \
		"fi\0"

#define BOOTENV_DEV_NAME_SYSTEM(devtypeu, devtypel, instance)	\
		"system "

#define BOOTENV_DEV_PANIC(devtypeu, devtypel, instance) \
	"bootcmd_panic=" \
		"if test \"${android_boot}\" -eq 1; then;" \
			"fastboot " __stringify(CONFIG_FASTBOOT_USB_DEV) "; " \
			"reset;" \
		"fi\0"

#define BOOTENV_DEV_NAME_PANIC(devtypeu, devtypel, instance)	\
		"panic "

#define EXTRA_ANDROID_ENV_SETTINGS                                     \
	"set_android_boot=setenv android_boot 1;setenv partitions $partitions_android;setenv mmcdev 0;setenv force_avb 0;saveenv;\0" \
	ANDROIDBOOT_GET_CURRENT_SLOT_CMD                              \
	AVB_VERIFY_CMD                                                \
	BOOTENV

#define BOOT_TARGET_DEVICES(func) \
	func(LINUX, linux, na) \
	func(FASTBOOT, fastboot, na) \
	func(RECOVERY, recovery, na) \
	func(SYSTEM, system, na) \
	func(PANIC, panic, na) \

#else

#define EXTRA_ANDROID_ENV_SETTINGS  ""

#if CONFIG_IS_ENABLED(CMD_PXE)
# define BOOT_TARGET_PXE(func) func(PXE, pxe, na)
#else
# define BOOT_TARGET_PXE(func)
#endif

#if CONFIG_IS_ENABLED(CMD_DHCP)
# define BOOT_TARGET_DHCP(func) func(DHCP, dhcp, na)
#else
# define BOOT_TARGET_DHCP(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	BOOT_TARGET_PXE(func) \
	BOOT_TARGET_DHCP(func) \
	func(LINUX, linux, na) \

#endif

#define EXTRA_ENV_DFUARGS \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_RAM \
	DFU_ALT_INFO_OSPI

/* Incorporate settings into the U-Boot environment */
#define CONFIG_EXTRA_ENV_SETTINGS					\
	EXTRA_ANDROID_ENV_SETTINGS					\
	DEFAULT_LINUX_BOOT_ENV						\
	DEFAULT_FIT_TI_ARGS						\
	DEFAULT_MMC_TI_ARGS						\
	EXTRA_ENV_AM625_BOARD_SETTINGS					\
	EXTRA_ENV_AM625_BOARD_SETTINGS_MMC				\
	EXTRA_ENV_DFUARGS						\
	EXTRA_ENV_AM625_BOARD_SETTING_USBMSC				\
	EXTRA_ENV_AM625_BEAGPLAY_PRODUCTION_BOARD_SETTINGS		\
	BOOTENV

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

/* MMC ENV related defines */
#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		1
#endif

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#endif
#define CONFIG_SYS_MALLOC_LEN           SZ_128M

#endif /* __CONFIG_AM625_EVM_H */
