/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * TI AM437x boards information header
 * Derived from AM335x board.
 *
 * Copyright (C) 2020, Texas Instruments, Incorporated - http://www.ti.com/
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include <asm/arch/omap.h>

#define DEV_ATTR_MAX_OFFSET    5
#define DEV_ATTR_MIN_OFFSET    0

void enable_uart0_pin_mux(void);
void enable_board_pin_mux(void);
void enable_i2c0_pin_mux(void);

#ifdef CONFIG_DM_I2C
#define pmic_get(name) (NULL)
#define pmic_probe(p) (NULL)
#define i2c_init(speed, slaveaddr)
#define i2c_probe(chip) (NULL)
#endif

#endif
