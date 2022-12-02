// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62Ax platforms
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <common.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>

#include "../common/board_detect.h"

#define board_is_am62ax_skevm()		board_ti_k3_is("AM62A-SKEVM")

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

	if (!eeprom_read)
		return -1;

	if (board_is_am62ax_skevm()) {
		if (!strcmp(name, "k3-am62a7-r5-sk") || !strcmp(name, "k3-am62a7-sk"))
			return 0;
	}

	return -1;
}
#endif
