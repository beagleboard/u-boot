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

#include "../../ti/common/board_detect.h"
#include "../../ti/common/fdt_ops.h"

#define board_is_am62x_skevm()  (board_ti_k3_is("AM62-SKEVM") || \
				 board_ti_k3_is("AM62B-SKEVM"))
#define board_is_am62b_p1_skevm() board_ti_k3_is("AM62B-SKEVM-P1")
#define board_is_am62x_lp_skevm()  board_ti_k3_is("AM62-LP-SKEVM")
#define board_is_am62x_sip_skevm()  board_ti_k3_is("AM62SIP-SKEVM")

#if CONFIG_IS_ENABLED(TI_I2C_BOARD_DETECT)
int checkboard(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

	if (!do_board_detect_am6())
		printf("Board: %s rev %s\n", ep->name, ep->version);

	return 0;
}

#if CONFIG_IS_ENABLED(BOARD_LATE_INIT)
static void setup_board_eeprom_env(void)
{
	char *name = "am62x_skevm";

	if (do_board_detect_am6())
		goto invalid_eeprom;

	if (board_is_am62x_skevm())
		name = "am62x_skevm";
	else if (board_is_am62b_p1_skevm())
		name = "am62b_p1_skevm";
	else if (board_is_am62x_lp_skevm())
		name = "am62x_lp_skevm";
	else if (board_is_am62x_sip_skevm())
		name = "am62x_sip_skevm";
	else
		printf("Unidentified board claims %s in eeprom header\n",
		       board_ti_get_name());

invalid_eeprom:
	set_board_info_env_am6(name);
}
#endif
#endif


int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

#if CONFIG_IS_ENABLED(BOARD_LATE_INIT)
int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_TI_I2C_BOARD_DETECT)) {
		setup_board_eeprom_env();
	}

	char fdtfile[50];

	snprintf(fdtfile, sizeof(fdtfile), "%s.dtb", CONFIG_DEFAULT_DEVICE_TREE);

	env_set("fdtfile", fdtfile);

	return 0;
}
#endif
