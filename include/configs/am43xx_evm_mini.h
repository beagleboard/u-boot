/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2020 Texas Instruments Incorporated - http://www.ti.com/
 */

#ifndef __CONFIG_AM43XX_EVM_MINI_H
#define __CONFIG_AM43XX_EVM_MINI_H

#define CONFIG_MAX_RAM_BANK_SIZE	(1024 << 21)	/* 2GB */
#define CONFIG_SYS_TIMERBASE		0x48040000	/* Use Timer2 */

#include <asm/arch/omap.h>

/* NS16550 Configuration */
#define CONFIG_SYS_NS16550_CLK		48000000
#if !defined(CONFIG_SPL_DM) || !defined(CONFIG_DM_SERIAL)
#define CONFIG_SYS_NS16550_REG_SIZE    (-4)
#define CONFIG_SYS_NS16550_SERIAL
#endif

/* I2C Configuration */
#define CONFIG_ENV_EEPROM_IS_ON_I2C
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50	/* Main EEPROM */
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	2

/* Power */
#ifndef CONFIG_DM_I2C
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#endif
#define CONFIG_POWER_TPS65218

/* SPL defines. */
#define CONFIG_SYS_SPL_ARGS_ADDR	(CONFIG_SYS_SDRAM_BASE + \
					 (128 << 20))

/* Enabling L2 Cache */
#define CONFIG_SYS_L2_PL310
#define CONFIG_SYS_PL310_BASE	0x48242000

/*
 * Since SPL did pll and ddr initialization for us,
 * we don't need to do it twice.
 */
#if !defined(CONFIG_SPL_BUILD) && !defined(CONFIG_QSPI_BOOT)
#define CONFIG_SKIP_LOWLEVEL_INIT
#endif

/*
 * When building U-Boot such that there is no previous loader
 * we need to call board_early_init_f.  This is taken care of in
 * s_init when we have SPL used.
 */

/* Now bring in the rest of the common code. */
#include <configs/ti_armv7_omap.h>

/* Clock Defines */
#define V_OSCK				24000000  /* Clock output from T2 */
#define V_SCLK				(V_OSCK)

/* NS16550 Configuration */
#define CONFIG_SYS_NS16550_COM1		0x44e09000	/* Base EVM has UART0 */

/*
 * Disable MMC DM for SPL build and can be re-enabled after adding
 * DM support in SPL
 */
#ifdef CONFIG_SPL_BUILD
#undef CONFIG_TIMER
#endif

#ifndef CONFIG_SPL_BUILD
#include <environment/ti/mmc.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	DEFAULT_MMC_TI_ARGS \
	DEFAULT_FIT_TI_ARGS \
	"fdtfile=undefined\0" \
	"bootpart=0:2\0" \
	"bootdir=/boot\0" \
	"bootfile=zImage\0" \
	"console=ttyS0,115200n8\0" \
	"partitions=" \
		"uuid_disk=${uuid_gpt_disk};" \
		"name=rootfs,start=2MiB,size=-,uuid=${uuid_gpt_rootfs}\0" \
	"optargs=\0" \
	"ramroot=/dev/ram0 rw\0" \
	"ramrootfstype=ext2\0" \
	"usbargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${usbroot} " \
		"rootfstype=${usbrootfstype}\0" \
	"ramargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${ramroot} " \
		"rootfstype=${ramrootfstype}\0" \
	"loadramdisk=load ${devtype} ${devnum} ${rdaddr} ramdisk.gz\0" \
	"default_device_tree=am437x-gp-evm.dtb\0" \
	"findfdt=" \
		"setenv name_fdt ${default_device_tree};" \
		"setenv fdtfile ${name_fdt}\0"

#define CONFIG_BOOTCOMMAND \
	"if test ${boot_fit} -eq 1; then "	\
		"run update_to_fit;"	\
	"fi;"	\
	"run findfdt;" \
	"run envboot;" \
	"run mmcboot;"

#endif

#if defined(CONFIG_TI_SECURE_DEVICE)
/* Avoid relocating onto firewalled area at end of DRAM */
#define CONFIG_PRAM (64 * 1024)
#endif /* CONFIG_TI_SECURE_DEVICE */

#endif	/* __CONFIG_AM43XX_EVM_MINI_H */
