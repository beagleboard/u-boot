/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */


#ifndef LPDDR4_H
#define LPDDR4_H

#include "lpddr4_ctl_regs.h"
#include "lpddr4_sanity.h"
#ifdef CONFIG_K3_AM64_DDRSS
#include "lpddr4_16bit.h"
#include "lpddr4_16bit_sanity.h"
#else
#include "lpddr4_32bit.h"
#include "lpddr4_32bit_sanity.h"
#endif

#ifdef REG_WRITE_VERIF
#include "lpddr4_ctl_regs_rw_masks.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define PRODUCT_ID (0x1046U)

#define BIT_MASK    (0x1U)
#define BYTE_MASK   (0xffU)
#define NIBBLE_MASK (0xfU)

#define WORD_SHIFT (32U)
#define WORD_MASK (0xffffffffU)
#define SLICE_WIDTH (0x100)

#define CTL_OFFSET 0
#define PI_OFFSET (((uint32_t)1) << 11)
#define PHY_OFFSET (((uint32_t)1) << 12)

#define CTL_INT_MASK_ALL ((uint32_t)LPDDR4_LOR_BITS - WORD_SHIFT)

#define PLL_READY (0x3U)
#define IO_CALIB_DONE ((uint32_t)0x1U << 23U)
#define IO_CALIB_FIELD ((uint32_t)NIBBLE_MASK << 28U)
#define IO_CALIB_STATE ((uint32_t)0xBU << 28U)
#define RX_CAL_DONE ((uint32_t)BIT_MASK << 4U)
#define CA_TRAIN_RL (((uint32_t)BIT_MASK << 5U) | ((uint32_t)BIT_MASK << 4U))
#define WR_LVL_STATE (((uint32_t)NIBBLE_MASK) << 13U)
#define GATE_LVL_ERROR_FIELDS (((uint32_t)BIT_MASK << 7U) | ((uint32_t)BIT_MASK << 6U))
#define READ_LVL_ERROR_FIELDS ((((uint32_t)NIBBLE_MASK) << 28U) | (((uint32_t)BYTE_MASK) << 16U))
#define DQ_LVL_STATUS (((uint32_t)BIT_MASK << 26U) | (((uint32_t)BYTE_MASK) << 18U))

#define CDN_TRUE  1U
#define CDN_FALSE 0U

void lpddr4_setsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound);
volatile uint32_t *lpddr4_addoffset(volatile uint32_t *addr, uint32_t regOffset);
uint32_t lpddr4_pollctlirq(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt irqBit, uint32_t delay);
bool lpddr4_checklvlerrors(const lpddr4_privatedata *pD, lpddr4_debuginfo *debugInfo, bool errFound);
void lpddr4_seterrors(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, uint8_t *errFoundPtr);

uint32_t lpddr4_enablepiinitiator(const lpddr4_privatedata *pD);
void lpddr4_checkwrlvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr);
uint32_t lpddr4_checkmmrreaderror(const lpddr4_privatedata *pD, uint64_t *mmrValue, uint8_t *mrrStatus);
uint32_t lpddr4_getdslicemask(uint32_t dSliceNum, uint32_t arrayOffset);
#ifdef __cplusplus
}
#endif

#endif  /* LPDDR4_H */
