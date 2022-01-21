/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Texas Instruments Incorporated - http://www.ti.com/
 */

#include <config.h>
#include <linux/linkage.h>

#if defined(CONFIG_SPL_BUILD)
ENTRY(__asm_k3_enable_serror)
	/* unmask SError and abort */
	msr daifclr, #4

	/* Set HCR_EL2[AMO] so SError @EL2 is taken */
	mrs	x0, hcr_el2
	orr	x0, x0, #0x20			/* AMO */
	msr	hcr_el2, x0
	isb
	ret
ENDPROC(__asm_k3_enable_serror)
#endif
