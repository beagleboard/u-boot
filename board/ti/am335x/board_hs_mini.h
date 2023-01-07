/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * TI AM437x boards information header
 * Derived from AM335x board.
 *
 * Copyright (C) 2020, Texas Instruments, Incorporated - http://www.ti.com/
 */

#ifndef _BOARD_HS_H_
#define _BOARD_HS_H_

#ifdef CONFIG_TI_SECURE_DEVICE
void board_fit_image_post_process(void **p_image, size_t *p_size)
{
	secure_boot_verify_image(p_image, p_size);
}
#endif

#endif
