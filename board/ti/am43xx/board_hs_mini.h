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
void board_fit_image_post_process(const void *fit, int node,
				  void **p_image, size_t *p_size)
{
	secure_boot_verify_image(p_image, p_size);
}

void board_tee_image_process(ulong tee_image, size_t tee_size)
{
	secure_tee_install((u32)tee_image);
}

U_BOOT_FIT_LOADABLE_HANDLER(IH_TYPE_TEE, board_tee_image_process);
#endif

#endif
