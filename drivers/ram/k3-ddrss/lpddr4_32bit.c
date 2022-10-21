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

static void lpddr4_setrxoffseterror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errorFound);

uint32_t lpddr4_enablepiinitiator(const lpddr4_privatedata *pD)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_SET(LPDDR4__PI_INIT_LVL_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_INIT_LVL_EN__REG)));
	regVal = CPS_FLD_SET(LPDDR4__PI_NORMAL_LVL_SEQ__FLD, regVal);
	CPS_REG_WRITE((&(ctlRegBase->LPDDR4__PI_INIT_LVL_EN__REG)), regVal);
	return result;
}

uint32_t lpddr4_getctlinterruptmask(const lpddr4_privatedata *pD, uint64_t *mask)
{
	uint32_t result = 0U;
	uint32_t lowerMask = 0U;

	result = lpddr4_getctlinterruptmaskSF(pD, mask);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		lowerMask = (uint32_t)(CPS_FLD_READ(LPDDR4__INT_MASK_0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_0__REG))));
		*mask = (uint64_t)(CPS_FLD_READ(LPDDR4__INT_MASK_1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_1__REG))));
		*mask = (uint64_t)((*mask << WORD_SHIFT) | lowerMask);
	}
	return result;
}

uint32_t lpddr4_setctlinterruptmask(const lpddr4_privatedata *pD, const uint64_t *mask)
{
	uint32_t result;
	uint32_t regVal = 0;
	const uint64_t ui64One = 1ULL;
	const uint32_t ui32IrqCount = (uint32_t)LPDDR4_INTR_LOR_BITS + 1U;

	result = lpddr4_setctlinterruptmaskSF(pD, mask);
	if ((result == (uint32_t)CDN_EOK) && (ui32IrqCount < 64U)) {
		if (*mask >= (ui64One << ui32IrqCount))
			result = (uint32_t)CDN_EINVAL;
	}

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		regVal = (uint32_t)(*mask & WORD_MASK);
		regVal = CPS_FLD_WRITE(LPDDR4__INT_MASK_0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_0__REG)), regVal);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_MASK_0__REG), regVal);

		regVal = (uint32_t)((*mask >> WORD_SHIFT) & WORD_MASK);
		regVal = CPS_FLD_WRITE(LPDDR4__INT_MASK_1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_MASK_1__REG)), regVal);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_MASK_1__REG), regVal);
	}
	return result;
}

uint32_t lpddr4_checkctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr, bool *irqStatus)
{
	uint32_t result;
	uint32_t ctlIrqStatus = 0;
	uint32_t fieldShift = 0;

	result = LPDDR4_INTR_CheckCtlIntSF(pD, intr, irqStatus);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		if ((uint32_t)intr >= (uint32_t)WORD_SHIFT) {
			ctlIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_1__REG));
			fieldShift = (uint32_t)intr - ((uint32_t)WORD_SHIFT);
		} else {
			ctlIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__INT_STATUS_0__REG));
			fieldShift = (uint32_t)intr;
		}

		if (fieldShift < WORD_SHIFT) {
			if (((ctlIrqStatus >> fieldShift) & BIT_MASK) > 0U)
				*irqStatus = true;
			else
				*irqStatus = false;
		}
	}
	return result;
}

uint32_t lpddr4_ackctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr)
{
	uint32_t result = 0;
	uint32_t regVal = 0;
	uint32_t localInterrupt = (uint32_t)intr;

	result = LPDDR4_INTR_AckCtlIntSF(pD, intr);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		if (localInterrupt > WORD_SHIFT) {
			localInterrupt = (localInterrupt - (uint32_t)WORD_SHIFT);
			regVal = ((uint32_t)BIT_MASK << localInterrupt);
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_1__REG), regVal);
		} else {
			regVal = ((uint32_t)BIT_MASK << localInterrupt);
			CPS_REG_WRITE(&(ctlRegBase->LPDDR4__INT_ACK_0__REG), regVal);
		}
	}

	return result;
}

void lpddr4_checkwrlvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_WRLVL_ERROR_OBS_0__REG));
	errBitMask = (BIT_MASK << 1) | (BIT_MASK);
	for (snum = 0U; snum < DSLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != 0U) {
			debugInfo->wrlvlerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

static void lpddr4_setrxoffseterror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errorFound)
{
	volatile uint32_t *regAddress;
	uint32_t snum = 0U;
	uint32_t errBitMask = 0U;
	uint32_t regVal = 0U;

	if (*errorFound == (bool)false) {
		regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_RX_CAL_LOCK_OBS_0__REG));
		errBitMask = (RX_CAL_DONE) | (NIBBLE_MASK);
		for (snum = (uint32_t)0U; snum < DSLICE_NUM; snum++) {
			regVal = CPS_FLD_READ(LPDDR4__PHY_RX_CAL_LOCK_OBS_0__FLD, CPS_REG_READ(regAddress));
			if ((regVal & errBitMask) != RX_CAL_DONE) {
				debugInfo->rxoffseterror = (uint8_t)true;
				*errorFound = true;
			}
			regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
		}
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
		lpddr4_setrxoffseterror(ctlRegBase, debugInfo, &errorFound);
		errorFound = (bool)lpddr4_checklvlerrors(pD, debugInfo, errorFound);
	}

	if (errorFound == (bool)true)
		result = (uint32_t)CDN_EPROTO;

	return result;
}

uint32_t lpddr4_geteccenable(const lpddr4_privatedata *pD, lpddr4_eccenable *eccParam)
{
	uint32_t result = 0U;
	uint32_t fldVal = 0U;

	result = lpddr4_geteccenableSF(pD, eccParam);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		fldVal = CPS_FLD_READ(LPDDR4__ECC_ENABLE__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__ECC_ENABLE__REG)));
		switch (fldVal) {
		case 3:
			*eccParam = LPDDR4_ECC_ERR_DETECT_CORRECT;
			break;
		case 2:
			*eccParam = LPDDR4_ECC_ERR_DETECT;
			break;
		case 1:
			*eccParam = LPDDR4_ECC_ENABLED;
			break;
		default:
			*eccParam = LPDDR4_ECC_DISABLED;
			break;
		}
	}
	return result;
}

uint32_t lpddr4_seteccenable(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	result = lpddr4_seteccenableSF(pD, eccParam);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		regVal = CPS_FLD_WRITE(LPDDR4__ECC_ENABLE__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__ECC_ENABLE__REG)), *eccParam);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__ECC_ENABLE__REG), regVal);
	}
	return result;
}

uint32_t lpddr4_getreducmode(const lpddr4_privatedata *pD, lpddr4_reducmode *mode)
{
	uint32_t result = 0U;

	result = lpddr4_getreducmodeSF(pD, mode);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		if (CPS_FLD_READ(LPDDR4__REDUC__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__REDUC__REG))) == 0U)
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
		regVal = (uint32_t)CPS_FLD_WRITE(LPDDR4__REDUC__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__REDUC__REG)), *mode);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__REDUC__REG), regVal);
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
		lowerData = CPS_REG_READ(&(ctlRegBase->LPDDR4__PERIPHERAL_MRR_DATA_0__REG));
		*mmrValue = CPS_REG_READ(&(ctlRegBase->LPDDR4__PERIPHERAL_MRR_DATA_1__REG));
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
	case 1:
		if (arrayOffset < DSLICE1_REG_COUNT)
			rwMask = g_lpddr4_data_slice_1_rw_mask[arrayOffset];
		break;
	case 2:
		if (arrayOffset < DSLICE2_REG_COUNT)
			rwMask = g_lpddr4_data_slice_2_rw_mask[arrayOffset];
		break;
	default:
		if (arrayOffset < DSLICE3_REG_COUNT)
			rwMask = g_lpddr4_data_slice_3_rw_mask[arrayOffset];
		break;
	}
	return rwMask;
}
#endif
