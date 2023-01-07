/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */

#include <cdn_errno.h>

#include "cps_drv_lpddr4.h"
#include "lpddr4_ctl_regs.h"
#include "lpddr4_if.h"
#include "lpddr4.h"
#include "lpddr4_structs_if.h"

static uint32_t ctlintmap[51][3] = {
	{ 0,  0,  7  },
	{ 1,  0,  8  },
	{ 2,  0,  9  },
	{ 3,  0,  14 },
	{ 4,  0,  15 },
	{ 5,  0,  16 },
	{ 6,  0,  17 },
	{ 7,  0,  19 },
	{ 8,  1,  0  },
	{ 9,  2,  0  },
	{ 10, 2,  3  },
	{ 11, 3,  0  },
	{ 12, 4,  0  },
	{ 13, 5,  11 },
	{ 14, 5,  12 },
	{ 15, 5,  13 },
	{ 16, 5,  14 },
	{ 17, 5,  15 },
	{ 18, 6,  0  },
	{ 19, 6,  1  },
	{ 20, 6,  2  },
	{ 21, 6,  6  },
	{ 22, 6,  7  },
	{ 23, 7,  3  },
	{ 24, 7,  4  },
	{ 25, 7,  5  },
	{ 26, 7,  6  },
	{ 27, 7,  7  },
	{ 28, 8,  0  },
	{ 29, 9,  0  },
	{ 30, 10, 0  },
	{ 31, 10, 1  },
	{ 32, 10, 2  },
	{ 33, 10, 3  },
	{ 34, 10, 4  },
	{ 35, 10, 5  },
	{ 36, 11, 0  },
	{ 37, 12, 0  },
	{ 38, 12, 1  },
	{ 39, 12, 2  },
	{ 40, 12, 3  },
	{ 41, 12, 4  },
	{ 42, 12, 5  },
	{ 43, 13, 0  },
	{ 44, 13, 1  },
	{ 45, 13, 3  },
	{ 46, 14, 0  },
	{ 47, 14, 2  },
	{ 48, 14, 3  },
	{ 49, 15, 2  },
	{ 50, 16, 0  },
};

static void lpddr4_checkctlinterrupt_4(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr, uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag);
static void lpddr4_checkctlinterrupt_3(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr, uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag);
static void lpddr4_checkctlinterrupt_2(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr, uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag);
static void lpddr4_ackctlinterrupt_4(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr);
static void lpddr4_ackctlinterrupt_3(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr);
static void lpddr4_ackctlinterrupt_2(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr);


uint32_t lpddr4_enablepiinitiator(const lpddr4_privatedata *pD)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_SET(LPDDR4__PI_NORMAL_LVL_SEQ__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_NORMAL_LVL_SEQ__REG)));
	CPS_REG_WRITE((&(ctlRegBase->LPDDR4__PI_NORMAL_LVL_SEQ__REG)), regVal);
	regVal = CPS_FLD_SET(LPDDR4__PI_INIT_LVL_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_INIT_LVL_EN__REG)));
	CPS_REG_WRITE((&(ctlRegBase->LPDDR4__PI_INIT_LVL_EN__REG)), regVal);
	return result;
}

uint32_t lpddr4_getctlinterruptmask(const lpddr4_privatedata *pD, uint64_t *mask)
{
	uint32_t result = 0U;

	result = lpddr4_getctlinterruptmaskSF(pD, mask);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		*mask = (uint64_t)(CPS_FLD_READ(LPDDR4__INT_MASK_MASTER__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_MASTER__REG))));
	}
	return result;
}

uint32_t lpddr4_setctlinterruptmask(const lpddr4_privatedata *pD, const uint64_t *mask)
{
	uint32_t result;
	uint32_t regVal = 0;
	const uint64_t ui64One = 1ULL;
	const uint32_t ui32IrqCount = (uint32_t)32U;

	result = lpddr4_setctlinterruptmaskSF(pD, mask);
	if ((result == (uint32_t)CDN_EOK) && (ui32IrqCount < 64U)) {
		if (*mask >= (ui64One << ui32IrqCount))
			result = (uint32_t)CDN_EINVAL;
	}

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		regVal = CPS_FLD_WRITE(LPDDR4__INT_MASK_MASTER__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_MASTER__REG)), *mask);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_MASK_MASTER__REG), regVal);
	}
	return result;
}

static void lpddr4_checkctlinterrupt_4(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr,
				       uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag)
{
	if ((intr >= LPDDR4_INTR_INIT_MEM_RESET_DONE) && (intr <= LPDDR4_INTR_INIT_POWER_ON_STATE))
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_INIT__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_INIT__REG)));
	else if ((intr >= LPDDR4_INTR_MRR_ERROR) && (intr <= LPDDR4_INTR_MR_WRITE_DONE))
		*ctlGrpIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_MODE__REG));
	else if (intr == LPDDR4_INTR_BIST_DONE)
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_BIST__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_BIST__REG)));
	else if (intr == LPDDR4_INTR_PARITY_ERROR)
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_PARITY__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_PARITY__REG)));
	else
		*CtlMasterIntFlag = (uint32_t)1U;
}

static void lpddr4_checkctlinterrupt_3(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr,
				       uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag)
{
	if ((intr >= LPDDR4_INTR_FREQ_DFS_REQ_HW_IGNORE) && (intr <= LPDDR4_INTR_FREQ_DFS_SW_DONE))
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_FREQ__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_FREQ__REG)));
	else if ((intr >= LPDDR4_INTR_LP_DONE) && (intr <= LPDDR4_INTR_LP_TIMEOUT))
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_LOWPOWER__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_LOWPOWER__REG)));
	else
		lpddr4_checkctlinterrupt_4(ctlRegBase, intr, ctlGrpIrqStatus, CtlMasterIntFlag);
}

static void lpddr4_checkctlinterrupt_2(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr,
				       uint32_t *ctlGrpIrqStatus, uint32_t *CtlMasterIntFlag)
{
	if (intr <= LPDDR4_INTR_TIMEOUT_AUTO_REFRESH_MAX)
		*ctlGrpIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_TIMEOUT__REG));
	else if ((intr >= LPDDR4_INTR_TRAINING_ZQ_STATUS) && (intr <= LPDDR4_INTR_TRAINING_DQS_OSC_VAR_OUT))
		*ctlGrpIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_TRAINING__REG));
	else if ((intr >= LPDDR4_INTR_USERIF_OUTSIDE_MEM_ACCESS) && (intr <= LPDDR4_INTR_USERIF_INVAL_SETTING))
		*ctlGrpIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_USERIF__REG));
	else if ((intr >= LPDDR4_INTR_MISC_MRR_TRAFFIC) && (intr <= LPDDR4_INTR_MISC_REFRESH_STATUS))
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_MISC__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_MISC__REG)));
	else if ((intr >= LPDDR4_INTR_DFI_UPDATE_ERROR) && (intr <= LPDDR4_INTR_DFI_TIMEOUT))
		*ctlGrpIrqStatus = CPS_FLD_READ(LPDDR4__INT_STATUS_DFI__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_DFI__REG)));
	else
		lpddr4_checkctlinterrupt_3(ctlRegBase, intr, ctlGrpIrqStatus, CtlMasterIntFlag);
}

uint32_t lpddr4_checkctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr, bool *irqStatus)
{
	uint32_t result;
	uint32_t ctlMasterIrqStatus = 0U;
	uint32_t ctlGrpIrqStatus = 0U;
	uint32_t CtlMasterIntFlag = 0U;

	result = LPDDR4_INTR_CheckCtlIntSF(pD, intr, irqStatus);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		ctlMasterIrqStatus = (CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_MASTER__REG)) & (~((uint32_t)1 << 31)));

		lpddr4_checkctlinterrupt_2(ctlRegBase, intr, &ctlGrpIrqStatus, &CtlMasterIntFlag);

		if ((ctlintmap[intr][INT_SHIFT] < WORD_SHIFT) && (ctlintmap[intr][GRP_SHIFT] < WORD_SHIFT)) {
			if ((((ctlMasterIrqStatus >> ctlintmap[intr][GRP_SHIFT]) & BIT_MASK) > 0U) &&
			    (((ctlGrpIrqStatus >> ctlintmap[intr][INT_SHIFT]) & BIT_MASK) > 0U) &&
			    (CtlMasterIntFlag == (uint32_t)0))
				*irqStatus = true;
			else if ((((ctlMasterIrqStatus >> ctlintmap[intr][GRP_SHIFT]) & BIT_MASK) > 0U) &&
				 (CtlMasterIntFlag == (uint32_t)1U))
				*irqStatus = true;
			else
				*irqStatus = false;
		}
	}
	return result;
}

static void lpddr4_ackctlinterrupt_4(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr)
{
	uint32_t regVal = 0;

	if ((intr >= LPDDR4_INTR_MRR_ERROR) && (intr <= LPDDR4_INTR_MR_WRITE_DONE) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_MODE__REG), (uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]);
	} else if ((intr == LPDDR4_INTR_BIST_DONE) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		regVal = CPS_FLD_WRITE(LPDDR4__INT_ACK_BIST__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_ACK_BIST__REG)),
				       (uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_BIST__REG), regVal);
	} else if ((intr == LPDDR4_INTR_PARITY_ERROR) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		regVal = CPS_FLD_WRITE(LPDDR4__INT_ACK_PARITY__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_ACK_PARITY__REG)),
				       (uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_PARITY__REG), regVal);
	} else {
	}
}

static void lpddr4_ackctlinterrupt_3(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr)
{
	uint32_t regVal = 0;

	if ((intr >= LPDDR4_INTR_LP_DONE) && (intr <= LPDDR4_INTR_LP_TIMEOUT) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		regVal = CPS_FLD_WRITE(LPDDR4__INT_ACK_LOWPOWER__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_ACK_LOWPOWER__REG)),
				       (uint32_t)((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_LOWPOWER__REG), regVal);
	} else if ((intr >= LPDDR4_INTR_INIT_MEM_RESET_DONE) && (intr <= LPDDR4_INTR_INIT_POWER_ON_STATE) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		regVal = CPS_FLD_WRITE(LPDDR4__INT_ACK_INIT__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_ACK_INIT__REG)),
				       (uint32_t)((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_INIT__REG), regVal);
	} else {
		lpddr4_ackctlinterrupt_4(ctlRegBase, intr);
	}
}

static void  lpddr4_ackctlinterrupt_2(lpddr4_ctlregs *ctlRegBase, lpddr4_intr_ctlinterrupt intr)
{
	uint32_t regVal = 0;

	if ((intr >= LPDDR4_INTR_DFI_UPDATE_ERROR) && (intr <= LPDDR4_INTR_DFI_TIMEOUT) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_DFI__REG), (uint32_t)((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
	} else if ((intr >= LPDDR4_INTR_FREQ_DFS_REQ_HW_IGNORE) && (intr <= LPDDR4_INTR_FREQ_DFS_SW_DONE) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		regVal = CPS_FLD_WRITE(LPDDR4__INT_ACK_FREQ__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_ACK_FREQ__REG)),
				       (uint32_t)((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_FREQ__REG), regVal);
	} else {
		lpddr4_ackctlinterrupt_3(ctlRegBase, intr);
	}
}

uint32_t lpddr4_ackctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr)
{
	uint32_t result;

	result = LPDDR4_INTR_AckCtlIntSF(pD, intr);
	if ((result == (uint32_t)CDN_EOK) && ((uint32_t)ctlintmap[intr][INT_SHIFT] < WORD_SHIFT)) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		if (intr <= LPDDR4_INTR_TIMEOUT_AUTO_REFRESH_MAX)
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_TIMEOUT__REG), ((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		else if ((intr >= LPDDR4_INTR_TRAINING_ZQ_STATUS) && (intr <= LPDDR4_INTR_TRAINING_DQS_OSC_VAR_OUT))
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_TRAINING__REG), ((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		else if ((intr >= LPDDR4_INTR_USERIF_OUTSIDE_MEM_ACCESS) && (intr <= LPDDR4_INTR_USERIF_INVAL_SETTING))
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_USERIF__REG), ((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		else if ((intr >= LPDDR4_INTR_MISC_MRR_TRAFFIC) && (intr <= LPDDR4_INTR_MISC_REFRESH_STATUS))
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_MISC__REG), ((uint32_t)BIT_MASK << (uint32_t)ctlintmap[intr][INT_SHIFT]));
		else
			lpddr4_ackctlinterrupt_2(ctlRegBase, intr);
	}

	return result;
}

void lpddr4_checkwrlvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_WRLVL_STATUS_OBS_0__REG));
	errBitMask = ((uint32_t)BIT_MASK << (uint32_t)12U);
	for (snum = 0U; snum < DSLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != 0U) {
			debugInfo->wrlvlerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

uint32_t lpddr4_getdebuginitinfo(const lpddr4_privatedata *pD, lpddr4_debuginfo *debugInfo)
{
	uint32_t result = 0U;
	bool errorFound = false;

	result = lpddr4_getdebuginitinfoSF(pD, debugInfo);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		lpddr4_seterrors(ctlRegBase, debugInfo, (uint8_t *)&errorFound);
		lpddr4_setsettings(ctlRegBase, errorFound);
		errorFound = (bool)lpddr4_checklvlerrors(pD, debugInfo, errorFound);
	}

	if (errorFound == (bool)true)
		result = (uint32_t)CDN_EPROTO;

	return result;
}

uint32_t lpddr4_getreducmode(const lpddr4_privatedata *pD, lpddr4_reducmode *mode)
{
	uint32_t result = 0U;

	result = lpddr4_getreducmodeSF(pD, mode);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		if (CPS_FLD_READ(LPDDR4__MEM_DP_REDUCTION__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__MEM_DP_REDUCTION__REG))) == 0U)
			*mode = LPDDR4_REDUC_ON;
		else
			*mode = LPDDR4_REDUC_OFF;
	}
	return result;
}

uint32_t lpddr4_setreducmode(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	result = lpddr4_setreducmodeSF(pD, mode);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		regVal = (uint32_t)CPS_FLD_WRITE(LPDDR4__MEM_DP_REDUCTION__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__MEM_DP_REDUCTION__REG)), *mode);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__MEM_DP_REDUCTION__REG), regVal);
	}
	return result;
}

uint32_t lpddr4_checkmmrreaderror(const lpddr4_privatedata *pD, uint64_t *mmrValue, uint8_t *mrrStatus)
{
	uint32_t lowerData;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
	uint32_t result = (uint32_t)CDN_EOK;

	if (lpddr4_pollctlirq(pD, LPDDR4_INTR_MRR_ERROR, 100) == 0U) {
		*mrrStatus = (uint8_t)CPS_FLD_READ(LPDDR4__MRR_ERROR_STATUS__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__MRR_ERROR_STATUS__REG)));
		*mmrValue = (uint64_t)0;
		result = (uint32_t)CDN_EIO;
	} else {
		*mrrStatus = (uint8_t)0;
		lowerData = CPS_REG_READ(&(ctlRegBase->LPDDR4__PERIPHERAL_MRR_DATA__REG));
		*mmrValue = (uint64_t)((*mmrValue << WORD_SHIFT) | lowerData);
		result = lpddr4_ackctlinterrupt(pD, LPDDR4_INTR_MR_READ_DONE);
	}
	return result;
}

#ifdef REG_WRITE_VERIF

uint32_t lpddr4_getdslicemask(uint32_t dSliceNum, uint32_t arrayOffset)
{
	uint32_t rwMask = 0U;

	switch (dSliceNum) {
	case 0:
		if (arrayOffset < DSLICE0_REG_COUNT)
			rwMask = g_lpddr4_data_slice_0_rw_mask[arrayOffset];
		break;
	default:
		if (arrayOffset < DSLICE1_REG_COUNT)
			rwMask = g_lpddr4_data_slice_1_rw_mask[arrayOffset];
		break;
	}
	return rwMask;
}
#endif

uint32_t lpddr4_geteccenable(const lpddr4_privatedata *pD, lpddr4_eccenable *eccParam)
{
	uint32_t result = 0U;

	result = lpddr4_geteccenableSF(pD, eccParam);
	if (result == (uint32_t)CDN_EOK) {
		*eccParam = LPDDR4_ECC_DISABLED;
		result = (uint32_t)CDN_EOPNOTSUPP;
	}

	return result;
}
uint32_t lpddr4_seteccenable(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam)
{
	uint32_t result = 0U;

	result = lpddr4_seteccenableSF(pD, eccParam);
	if (result == (uint32_t)CDN_EOK)
		result = (uint32_t)CDN_EOPNOTSUPP;

	return result;
}
