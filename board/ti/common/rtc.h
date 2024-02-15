/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * RTC setup include file for TI Platforms
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef __RTC_H
#define __RTC_H

/* Debounce configuration register*/
#define WKUP_CTRLMMR_DBOUNCE_CFG1 0x04504084
#define WKUP_CTRLMMR_DBOUNCE_CFG2 0x04504088
#define WKUP_CTRLMMR_DBOUNCE_CFG3 0x0450408c
#define WKUP_CTRLMMR_DBOUNCE_CFG4 0x04504090
#define WKUP_CTRLMMR_DBOUNCE_CFG5 0x04504094
#define WKUP_CTRLMMR_DBOUNCE_CFG6 0x04504098

/**
 * board_rtc_init() - Enable the external 32k crystal and configure debounce
 * registers.
 * This function enables the 32k crystal, add required TRIM settings for the
 * crystals and add debounce configuration.
 */
void board_rtc_init(void);

#endif	/* __RTC_H */
