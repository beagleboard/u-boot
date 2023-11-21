/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for K3 AM625 SoC family: Android
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef BOOT_PARTITION
#define BOOT_PARTITION "boot"
#endif

#ifndef CONTROL_PARTITION
#define CONTROL_PARTITION "misc"
#endif

#if defined(CONFIG_CMD_AVB)
#define AVB_VERIFY_CMD "avb_verify_cmd=" \
	"if test \"${force_avb}\" -eq 1; then " \
		"avb init ${mmcdev};" \
		"if avb verify $slot_suffix; then " \
			"echo AVB verification OK.;" \
			"setenv bootargs \"$bootargs $avb_bootargs\";" \
		"else " \
			"echo AVB verification failed.;" \
		"exit; fi;" \
	"else " \
		"setenv bootargs \"$bootargs androidboot.verifiedbootstate=orange\";" \
		"echo Running without AVB...; "\
	"fi;\0"
#else
#define AVB_VERIFY_CMD "avb_verify_cmd=true\0"
#endif

#if defined(CONFIG_CMD_AB_SELECT)
#define AB_SELECT_SLOT_CMD "ab_select_slot_cmd=" \
	"part number mmc ${mmcdev} " CONTROL_PARTITION " control_part_number;" \
	"if test $? -ne 0; then " \
		"echo " CONTROL_PARTITION " partition not found;" \
		"exit;" \
	"fi;" \
	"echo " CONTROL_PARTITION " partition number:${control_part_number};" \
	"ab_select current_slot mmc ${mmcdev}:${control_part_number};" \
	"if test -z ${current_slot}; then " \
		"echo current_slot not found;" \
		"exit;" \
	"fi;" \
	"setenv slot_suffix _${current_slot}; " \
	"setenv bootargs_ab androidboot.slot_suffix=${slot_suffix}; " \
	"echo A/B cmdline addition: ${bootargs_ab};" \
	"setenv bootargs ${bootargs} ${bootargs_ab};\0"

#define AB_BOOTARGS " androidboot.force_normal_boot=1"
#define RECOVERY_PARTITION "boot"
#else
#define AB_SELECT_SLOT_CMD "ab_select_slot_cmd=true\0"
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
#define PREPARE_FDT_CMD "prepare_fdt_cmd=" \
	"echo Preparing FDT...; " \
	"if test $board_name = am62x_skevm; then " \
		"echo \"  Reading DTB for am62x_skevm...\"; " \
		"setenv dtb_index 0;" \
	"elif test $board_name = am62b_p1_skevm; then " \
		"echo \"  Reading DTB for am62b_p1_skevm...\"; " \
		"setenv dtb_index 0;" \
	"elif test $board_name = am62x_lp_skevm; then " \
		"echo \"  Reading DTB for am62x_lp_skevm...\"; " \
		"setenv dtb_index 1;" \
	"elif test $board_name =  am62x_beagleplay; then " \
		"echo \"  Reading DTB for am62x_beagleplay...\"; " \
		"setenv dtb_index 2;" \
	"elif test $board_name =  am62px; then " \
		"echo \"  Reading DTB for am62px...\"; " \
		"setenv dtb_index 3;" \
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
	"setenv bootargs \"$bootargs androidboot.dtbo_idx=$dtbo_idx \"; \0"

#define BOOT_CMD "bootm ${loadaddr} ${loadaddr} ${fdt_addr_r};"

#define BOOTENV_DEV_FASTBOOT(devtypeu, devtypel, instance) \
	"bootcmd_fastboot=" \
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
		"fi\0"

#define BOOTENV_DEV_NAME_FASTBOOT(devtypeu, devtypel, instance)	\
		"fastboot "

#define BOOTENV_DEV_RECOVERY(devtypeu, devtypel, instance) \
	"bootcmd_recovery=" \
		"setenv run_recovery 0;" \
		"setenv ramdisk_addr_r 0xe9000000;" \
		"setenv vloadaddr 0xe0000000;" \
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
			"run ab_select_slot_cmd;" \
			"run avb_verify_cmd;" \
			"part start mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_start;" \
			"part size mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_size;" \
			"part start mmc ${mmcdev} vendor_boot${slot_suffix} vendor_boot_start;" \
			"part size  mmc ${mmcdev} vendor_boot${slot_suffix} vendor_boot_size;" \
			"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
				"if mmc read $vloadaddr ${vendor_boot_start} ${vendor_boot_size}; then " \
					"abootimg addr $loadaddr $vloadaddr;" \
					"run prepare_fdt_cmd;" \
					"echo Running Android Recovery...;" \
					BOOT_CMD \
				"fi;" \
			"fi;" \
			"echo Failed to boot Android...;" \
			"reset;" \
		"fi\0" \

#define BOOTENV_DEV_NAME_RECOVERY(devtypeu, devtypel, instance)	\
		"recovery "

#define BOOTENV_DEV_SYSTEM(devtypeu, devtypel, instance) \
	"bootcmd_system=" \
		"echo Loading Android " BOOT_PARTITION " partition...;" \
		"mmc dev ${mmcdev};" \
		"setenv bootargs ${bootargs} androidboot.serialno=${serial#};" \
		"run ab_select_slot_cmd;" \
		"run avb_verify_cmd;" \
		"setenv ramdisk_addr_r 0xe9000000;" \
		"setenv vloadaddr 0xe0000000;" \
		"part start mmc ${mmcdev} " BOOT_PARTITION "${slot_suffix} boot_start;" \
		"part size mmc ${mmcdev} " BOOT_PARTITION "${slot_suffix} boot_size;" \
		"part start mmc ${mmcdev} vendor_boot${slot_suffix} vendor_boot_start;" \
		"part size  mmc ${mmcdev} vendor_boot${slot_suffix} vendor_boot_size;" \
		"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
			"if mmc read $vloadaddr ${vendor_boot_start} ${vendor_boot_size}; then " \
				"abootimg addr $loadaddr $vloadaddr;" \
				"run prepare_fdt_cmd;" \
				"setenv bootargs \"${bootargs} " AB_BOOTARGS "\"  ; " \
				"echo Running Android...;" \
				BOOT_CMD \
			"fi;" \
		"fi;" \
		"echo Failed to boot Android...\0"

#define BOOTENV_DEV_NAME_SYSTEM(devtypeu, devtypel, instance)	\
		"system "

#define BOOTENV_DEV_PANIC(devtypeu, devtypel, instance) \
	"bootcmd_panic=" \
		"fastboot " __stringify(CONFIG_FASTBOOT_USB_DEV) "; " \
		"reset\0" \

#define BOOTENV_DEV_NAME_PANIC(devtypeu, devtypel, instance)	\
		"panic "

#define EXTRA_ENV_ANDROID_ARGS                                     \
	AB_SELECT_SLOT_CMD \
	AVB_VERIFY_CMD \
	PREPARE_FDT_CMD \
	BOOTENV

#undef BOOT_TARGET_DEVICES
#define BOOT_TARGET_DEVICES(func) \
	func(FASTBOOT, fastboot, na) \
	func(RECOVERY, recovery, na) \
	func(SYSTEM, system, na) \
	func(PANIC, panic, na) \

#undef CFG_EXTRA_ENV_SETTINGS
#define CFG_EXTRA_ENV_SETTINGS					\
	BOOTENV						\
	EXTRA_ENV_ANDROID_ARGS
