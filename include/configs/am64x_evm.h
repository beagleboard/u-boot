/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for K3 AM642 SoC family
 *
 * Copyright (C) 2020 Texas Instruments Incorporated - https://www.ti.com/
 *	Keerthy <j-keerthy@ti.com>
 */

#ifndef __CONFIG_AM642_EVM_H
#define __CONFIG_AM642_EVM_H

#include <linux/sizes.h>
#include <config_distro_bootcmd.h>
#include <environment/ti/mmc.h>
#include <asm/arch/am64_hardware.h>
#include <environment/ti/k3_dfu.h>

/* DDR Configuration */
#define CONFIG_SYS_SDRAM_BASE1		0x880000000

/* NAND support */

/* NAND Device Configuration : MT29F8G08ADAFAH4 chip */
#define CONFIG_SYS_NAND_PAGE_SIZE	4096
#define CONFIG_SYS_NAND_OOBSIZE		256
#define CONFIG_SYS_NAND_BLOCK_SIZE      SZ_256K
#define CONFIG_SYS_NAND_PAGE_COUNT	(CONFIG_SYS_NAND_BLOCK_SIZE / \
					 CONFIG_SYS_NAND_PAGE_SIZE)
#define CONFIG_SYS_NAND_5_ADDR_CYCLE

/* NAND Driver config */
#define CONFIG_SPL_NAND_INIT	1
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_NAND_OMAP_ECCSCHEME      OMAP_ECC_BCH8_CODE_HW
#define CONFIG_SYS_NAND_BAD_BLOCK_POS   NAND_LARGE_BADBLOCK_POS

#define CONFIG_SYS_NAND_ECCPOS		{ 2, 3, 4, 5, 6, 7, 8, 9, \
					 10, 11, 12, 13, 14, 15, 16, 17, \
					 18, 19, 20, 21, 22, 23, 24, 25, \
					 26, 27, 28, 29, 30, 31, 32, 33, \
					 34, 35, 36, 37, 38, 39, 40, 41, \
					 42, 43, 44, 45, 46, 47, 48, 49, \
					 50, 51, 52, 53, 54, 55, 56, 57, }
#define CONFIG_SYS_NAND_ECCSIZE         512
#define CONFIG_SYS_NAND_ECCBYTES        14

#ifdef CONFIG_SYS_K3_SPL_ATF
#define CONFIG_SYS_NAND_U_BOOT_OFFS     0x200000	/* tispl.bin partition */
#else
#define CONFIG_SYS_NAND_U_BOOT_OFFS     0x600000	/* u-boot.img partition */
#endif

#define CONFIG_SYS_NAND_MAX_CHIPS       1

#define CONFIG_SYS_MAX_NAND_DEVICE      1

#define CONFIG_SYS_NAND_BASE            0x51000000

#if defined(CONFIG_ENV_IS_IN_NAND)
#define CONFIG_SYS_ENV_SECT_SIZE        CONFIG_SYS_NAND_BLOCK_SIZE
#endif

/*-- end NAND config --*/

#ifdef CONFIG_SYS_K3_SPL_ATF
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"tispl.bin"
#endif

#ifndef CONFIG_CPU_V7R
#define CONFIG_SKIP_LOWLEVEL_INIT
#endif

#define CONFIG_SPL_MAX_SIZE		CONFIG_SYS_K3_MAX_DOWNLODABLE_IMAGE_SIZE
#if defined(CONFIG_TARGET_AM642_A53_EVM)
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SPL_TEXT_BASE + SZ_4M)
#else
/*
 * Maximum size in memory allocated to the SPL BSS. Keep it as tight as
 * possible (to allow the build to go through), as this directly affects
 * our memory footprint. The less we use for BSS the more we have available
 * for everything else.
 */
#define CONFIG_SPL_BSS_MAX_SIZE		0x4000
/*
 * Link BSS to be within SPL in a dedicated region located near the top of
 * the MCU SRAM, this way making it available also before relocation. Note
 * that we are not using the actual top of the MCU SRAM as there is a memory
 * location filled in by the boot ROM that we want to read out without any
 * interference from the C context.
 */
#define CONFIG_SPL_BSS_START_ADDR	(TI_SRAM_SCRATCH_BOARD_EEPROM_START -\
					 CONFIG_SPL_BSS_MAX_SIZE)
/* Set the stack right below the SPL BSS section */
#define CONFIG_SYS_INIT_SP_ADDR         CONFIG_SPL_BSS_START_ADDR
/* Configure R5 SPL post-relocation malloc pool in DDR */
#define CONFIG_SYS_SPL_MALLOC_START    0x84000000
#define CONFIG_SYS_SPL_MALLOC_SIZE     SZ_16M
#endif

#define CONFIG_SYS_BOOTM_LEN            SZ_64M

#define PARTS_DEFAULT \
	/* Linux partitions */ \
	"name=rootfs,start=0,size=-,uuid=${uuid_gpt_rootfs}\0"

/* U-Boot general configuration */
#define EXTRA_ENV_AM642_BOARD_SETTINGS					\
	"findfdt="							\
		"if test $board_name = am64x_gpevm; then " \
			"setenv fdtfile k3-am642-evm.dtb; fi; " \
		"if test $board_name = am64x_skevm; then " \
			"setenv fdtfile k3-am642-sk.dtb; fi;" \
		"if test $fdtfile = undefined; then " \
			"echo WARNING: Could not determine device tree to use; fi; \0" \
	"name_kern=Image\0"						\
	"console=ttyS2,115200n8\0"					\
	"args_all=setenv optargs ${optargs} "				\
		"earlycon=ns16550a,mmio32,0x02800000 ${mtdparts}\0"	\
	"run_kern=booti ${loadaddr} ${rd_spec} ${fdtaddr}\0"

/* U-Boot MMC-specific configuration */
#define EXTRA_ENV_AM642_BOARD_SETTINGS_MMC				\
	"boot=mmc\0"							\
	"mmcdev=1\0"							\
	"bootpart=1:2\0"						\
	"bootdir=/boot\0"						\
	"rd_spec=-\0"							\
	"init_mmc=run args_all args_mmc\0"				\
	"get_fdt_mmc=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}\0" \
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

#define EXTRA_ENV_AM642_BOARD_SETTING_USBMSC				\
	"args_usb=run finduuid;setenv bootargs console=${console} "	\
		"${optargs} "						\
		"root=PARTUUID=${uuid} rw "				\
		"rootfstype=${mmcrootfstype}\0"				\
	"init_usb=run args_all args_usb\0"				\
	"get_fdt_usb=load usb ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}\0" \
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
		"usb start;"							\
		"run findfdt;"						\
		"run init_usb;"						\
		"run get_kern_usb;"					\
		"run get_fdt_usb;"					\
		"run run_kern\0"

#define EXTRA_ENV_AM642_BOARD_SETTINGS_NAND				\
	"nbootpart=NAND.file-system\0"					\
	"nbootvolume=ubi0:rootfs\0"					\
	"bootdir=/boot\0"						\
	"rd_spec=-\0"							\
	"ubi_init=ubi part ${nbootpart}; ubifsmount ${nbootvolume};\0"	\
	"args_nand=setenv bootargs console=${console} "			\
		"${optargs} ubi.mtd=${nbootpart} "			\
		"root=${nbootvolume} rootfstype=ubifs\0"			\
	"init_nand=run args_all args_nand ubi_init\0"			\
	"get_fdt_nand=ubifsload ${fdtaddr} ${bootdir}/${fdtfile};\0"	\
	"get_overlay_nand="						\
		"fdt address ${fdtaddr};"				\
		"fdt resize 0x100000;"					\
		"for overlay in $name_overlays;"			\
		"do;"							\
		"ubifsload ${dtboaddr} ${bootdir}/${overlay} && "	\
		"fdt apply ${dtboaddr};"				\
		"done;\0"						\
	"get_kern_nand=ubifsload ${loadaddr} ${bootdir}/${name_kern}\0"	\
	"get_fit_nand=ubifsload ${addr_fit} ${bootdir}/${name_fit}\0"

#ifdef CONFIG_TARGET_AM642_A53_EVM
#define EXTRA_ENV_AM642_BOARD_SETTINGS_MTD				\
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0"				\
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"
#else
#define EXTRA_ENV_AM642_BOARD_SETTINGS_MTD
#endif


#define EXTRA_ENV_DFUARGS \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_EMMC_COMBINED \
	DFU_ALT_INFO_RAM \
	DFU_ALT_INFO_OSPI_COMBINED

/* Incorporate settings into the U-Boot environment */
#define CONFIG_EXTRA_ENV_SETTINGS					\
	DEFAULT_LINUX_BOOT_ENV						\
	DEFAULT_FIT_TI_ARGS						\
	DEFAULT_MMC_TI_ARGS						\
	EXTRA_ENV_AM642_BOARD_SETTINGS					\
	EXTRA_ENV_AM642_BOARD_SETTINGS_MMC				\
	EXTRA_ENV_AM642_BOARD_SETTINGS_MTD				\
	EXTRA_ENV_DFUARGS						\
	EXTRA_ENV_AM642_BOARD_SETTING_USBMSC				\
	EXTRA_ENV_AM642_BOARD_SETTINGS_NAND

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#define CONFIG_SYS_USB_FAT_BOOT_PARTITION 1

#endif /* __CONFIG_AM642_EVM_H */
