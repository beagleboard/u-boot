/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * RTC setup include file for TI Platforms
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef __RTC_H
#define __RTC_H

/**
 * board_rtc_init() - Enable the external 32k crystal and configure debounce
 * registers.
 * This function enables the 32k crystal, add required TRIM settings for the
 * crystals and add debounce configuration.
 */
void board_rtc_init(void);

#endif	/* __RTC_H */
