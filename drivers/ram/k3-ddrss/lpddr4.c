/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */

#include <cdn_errno.h>

#include "cps_drv_lpddr4.h"
#include "lpddr4_if.h"
#include "lpddr4.h"
#include "lpddr4_structs_if.h"

#ifndef LPDDR4_CUSTOM_TIMEOUT_DELAY
#define LPDDR4_CUSTOM_TIMEOUT_DELAY 100000000U
#endif

#ifndef LPDDR4_CPS_NS_DELAY_TIME
#define LPDDR4_CPS_NS_DELAY_TIME 10000000U
#endif

static uint32_t lpddr4_pollphyindepirq(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt irqBit, uint32_t delay);
static uint32_t lpddr4_pollandackirq(const lpddr4_privatedata *pD);
static uint32_t lpddr4_startsequencecontroller(const lpddr4_privatedata *pD);
static uint32_t lpddr4_writemmrregister(const lpddr4_privatedata *pD, uint32_t writeModeRegVal);
static void lpddr4_checkcatrainingerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr);
static void lpddr4_checkgatelvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr);
static void lpddr4_checkreadlvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr);
static void lpddr4_checkdqtrainingerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr);
static uint8_t lpddr4_seterror(volatile uint32_t *reg, uint32_t errBitMask, uint8_t *errFoundPtr, const uint32_t errorInfoBits);
static void lpddr4_setphysnapsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound);
static void lpddr4_setphyadrsnapsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound);
static void readpdwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrlongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrlonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrdpshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrdplongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void readsrdplonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles);
static void lpddr4_readlpiwakeuptime(lpddr4_ctlregs *ctlRegBase, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, uint32_t *cycles);
static void writepdwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrlongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrlonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrdpshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrdplongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void writesrdplonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles);
static void lpddr4_writelpiwakeuptime(lpddr4_ctlregs *ctlRegBase, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles);
static void lpddr4_updatefsp2refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max);
static void lpddr4_updatefsp1refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max);
static void lpddr4_updatefsp0refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max);
#ifdef REG_WRITE_VERIF
static uint32_t lpddr4_getphyrwmask(uint32_t regOffset);
static uint32_t lpddr4_verifyregwrite(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t regValue);
#endif

uint32_t lpddr4_pollctlirq(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt irqBit, uint32_t delay)
{
	uint32_t result = 0U;
	uint32_t timeout = 0U;
	bool irqStatus = false;

	do {
		if (++timeout == delay) {
			result = (uint32_t)CDN_EIO;
			break;
		}
		cps_delayns(LPDDR4_CPS_NS_DELAY_TIME);
		result = lpddr4_checkctlinterrupt(pD, irqBit, &irqStatus);
	} while ((irqStatus == (bool)false) && (result == (uint32_t)CDN_EOK));

	return result;
}

static uint32_t lpddr4_pollphyindepirq(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt irqBit, uint32_t delay)
{
	uint32_t result = 0U;
	uint32_t timeout = 0U;
	bool irqStatus = false;

	do {
		if (++timeout == delay) {
			result = (uint32_t)CDN_EIO;
			break;
		}
		cps_delayns(LPDDR4_CPS_NS_DELAY_TIME);
		result = lpddr4_checkphyindepinterrupt(pD, irqBit, &irqStatus);
	} while ((irqStatus == (bool)false) && (result == (uint32_t)CDN_EOK));

	return result;
}

static uint32_t lpddr4_pollandackirq(const lpddr4_privatedata *pD)
{
	uint32_t result = 0U;

	result = lpddr4_pollphyindepirq(pD, LPDDR4_INTR_PHY_INDEP_INIT_DONE_BIT, LPDDR4_CUSTOM_TIMEOUT_DELAY);

	if (result == (uint32_t)CDN_EOK)
		result = lpddr4_ackphyindepinterrupt(pD, LPDDR4_INTR_PHY_INDEP_INIT_DONE_BIT);
	if (result == (uint32_t)CDN_EOK)
		result = lpddr4_pollctlirq(pD, LPDDR4_INTR_MC_INIT_DONE, LPDDR4_CUSTOM_TIMEOUT_DELAY);
	if (result == (uint32_t)CDN_EOK)
		result = lpddr4_ackctlinterrupt(pD, LPDDR4_INTR_MC_INIT_DONE);
	return result;
}

static uint32_t lpddr4_startsequencecontroller(const lpddr4_privatedata *pD)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
	lpddr4_infotype infoType;

	regVal = CPS_FLD_SET(LPDDR4__PI_START__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_START__REG)));
	CPS_REG_WRITE((&(ctlRegBase->LPDDR4__PI_START__REG)), regVal);

	regVal = CPS_FLD_SET(LPDDR4__START__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__START__REG)));
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__START__REG), regVal);

	if (pD->infohandler != (lpddr4_infocallback)NULL) {
		infoType = LPDDR4_DRV_SOC_PLL_UPDATE;
		pD->infohandler(pD, infoType);
	}

	result = lpddr4_pollandackirq(pD);

	return result;
}

volatile uint32_t *lpddr4_addoffset(volatile uint32_t *addr, uint32_t regOffset)
{
	volatile uint32_t *local_addr = addr;
	volatile uint32_t *regAddr = &local_addr[regOffset];

	return regAddr;
}

uint32_t lpddr4_probe(const lpddr4_config *config, uint16_t *configSize)
{
	uint32_t result;

	result = (uint32_t)(lpddr4_probeSF(config, configSize));
	if (result == (uint32_t)CDN_EOK)
		*configSize = (uint16_t)(sizeof(lpddr4_privatedata));
	return result;
}

uint32_t lpddr4_init(lpddr4_privatedata *pD, const lpddr4_config *cfg)
{
	uint32_t result = 0U;

	result = lpddr4_initSF(pD, cfg);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)cfg->ctlbase;
		pD->ctlbase = ctlRegBase;
		pD->infohandler = (lpddr4_infocallback)cfg->infohandler;
		pD->ctlinterrupthandler = (lpddr4_ctlcallback)cfg->ctlinterrupthandler;
		pD->phyindepinterrupthandler = (lpddr4_phyindepcallback)cfg->phyindepinterrupthandler;
	}
	return result;
}

uint32_t lpddr4_start(const lpddr4_privatedata *pD)
{
	uint32_t result = 0U;

	result = lpddr4_startSF(pD);
	if (result == (uint32_t)CDN_EOK) {
		result = lpddr4_enablepiinitiator(pD);
		result = lpddr4_startsequencecontroller(pD);
	}
	return result;
}

uint32_t lpddr4_readreg(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t *regValue)
{
	uint32_t result = 0U;

	result = lpddr4_readregSF(pD, cpp, regValue);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		if (cpp == LPDDR4_CTL_REGS) {
			if (regOffset >= LPDDR4_INTR_CTL_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				*regValue = CPS_REG_READ(lpddr4_addoffset(&(ctlRegBase->DENALI_CTL_0), regOffset));
		} else if (cpp == LPDDR4_PHY_REGS) {
			if (regOffset >= LPDDR4_INTR_PHY_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				*regValue = CPS_REG_READ(lpddr4_addoffset(&(ctlRegBase->DENALI_PHY_0), regOffset));

		} else {
			if (regOffset >= LPDDR4_INTR_PHY_INDEP_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				*regValue = CPS_REG_READ(lpddr4_addoffset(&(ctlRegBase->DENALI_PI_0), regOffset));
		}
	}
	return result;
}

#ifdef REG_WRITE_VERIF

static uint32_t lpddr4_getphyrwmask(uint32_t regOffset)
{
	uint32_t rwMask = 0U;
	uint32_t arrayOffset = 0U;
	uint32_t sliceNum, sliceOffset = 0U;

	for (sliceNum = (uint32_t)0U; sliceNum <= (DSLICE_NUM + ASLICE_NUM); sliceNum++) {
		sliceOffset = sliceOffset + (uint32_t)SLICE_WIDTH;
		if (regOffset < sliceOffset)
			break;
	}
	arrayOffset = regOffset - (sliceOffset - (uint32_t)SLICE_WIDTH);

	if (sliceNum < DSLICE_NUM) {
		rwMask = lpddr4_getdslicemask(sliceNum, arrayOffset);
	} else {
		if (sliceNum == DSLICE_NUM) {
			if (arrayOffset < ASLICE0_REG_COUNT)
				rwMask = g_lpddr4_address_slice_0_rw_mask[arrayOffset];
		} else {
			if (arrayOffset < PHY_CORE_REG_COUNT)
				rwMask = g_lpddr4_phy_core_rw_mask[arrayOffset];
		}
	}
	return rwMask;
}

static uint32_t lpddr4_verifyregwrite(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t regValue)
{
	uint32_t result = (uint32_t)CDN_EOK;
	uint32_t regReadVal = 0U;
	uint32_t rwMask = 0U;

	result = lpddr4_readreg(pD, cpp, regOffset, &regReadVal);

	if (result == (uint32_t)CDN_EOK) {
		switch (cpp) {
		case LPDDR4_PHY_INDEP_REGS:
			rwMask = g_lpddr4_pi_rw_mask[regOffset];
			break;
		case LPDDR4_PHY_REGS:
			rwMask = lpddr4_getphyrwmask(regOffset);
			break;
		default:
			rwMask = g_lpddr4_ddr_controller_rw_mask[regOffset];
			break;
		}

		if ((rwMask & regReadVal) != (regValue & rwMask))
			result = CDN_EIO;
	}
	return result;
}
#endif

uint32_t lpddr4_writereg(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t regValue)
{
	uint32_t result = 0U;

	result = lpddr4_writeregSF(pD, cpp);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		if (cpp == LPDDR4_CTL_REGS) {
			if (regOffset >= LPDDR4_INTR_CTL_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				CPS_REG_WRITE(lpddr4_addoffset(&(ctlRegBase->DENALI_CTL_0), regOffset), regValue);
		} else if (cpp == LPDDR4_PHY_REGS) {
			if (regOffset >= LPDDR4_INTR_PHY_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				CPS_REG_WRITE(lpddr4_addoffset(&(ctlRegBase->DENALI_PHY_0), regOffset), regValue);
		} else {
			if (regOffset >= LPDDR4_INTR_PHY_INDEP_REG_COUNT)
				result = (uint32_t)CDN_EINVAL;
			else
				CPS_REG_WRITE(lpddr4_addoffset(&(ctlRegBase->DENALI_PI_0), regOffset), regValue);
		}
	}
#ifdef REG_WRITE_VERIF
	if (result == (uint32_t)CDN_EOK)
		result = lpddr4_verifyregwrite(pD, cpp, regOffset, regValue);

#endif

	return result;
}

uint32_t lpddr4_getmmrregister(const lpddr4_privatedata *pD, uint32_t readModeRegVal, uint64_t *mmrValue, uint8_t *mmrStatus)
{
	uint32_t result = 0U;
	uint32_t tDelay = 1000U;
	uint32_t regVal = 0U;

	result = lpddr4_getmmrregisterSF(pD, mmrValue, mmrStatus);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		regVal = CPS_FLD_WRITE(LPDDR4__READ_MODEREG__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__READ_MODEREG__REG)), readModeRegVal);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__READ_MODEREG__REG), regVal);

		result = lpddr4_pollctlirq(pD, LPDDR4_INTR_MR_READ_DONE, tDelay);
	}
	if (result == (uint32_t)CDN_EOK)
		result = lpddr4_checkmmrreaderror(pD, mmrValue, mmrStatus);
	return result;
}

static uint32_t lpddr4_writemmrregister(const lpddr4_privatedata *pD, uint32_t writeModeRegVal)
{
	uint32_t result = (uint32_t)CDN_EOK;
	uint32_t tDelay = 1000U;
	uint32_t regVal = 0U;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_WRITE(LPDDR4__WRITE_MODEREG__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__WRITE_MODEREG__REG)), writeModeRegVal);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__WRITE_MODEREG__REG), regVal);

	result = lpddr4_pollctlirq(pD, LPDDR4_INTR_MR_WRITE_DONE, tDelay);

	return result;
}

uint32_t lpddr4_setmmrregister(const lpddr4_privatedata *pD, uint32_t writeModeRegVal, uint8_t *mrwStatus)
{
	uint32_t result = 0U;

	result = lpddr4_setmmrregisterSF(pD, mrwStatus);
	if (result == (uint32_t)CDN_EOK) {
		result = lpddr4_writemmrregister(pD, writeModeRegVal);

		if (result == (uint32_t)CDN_EOK)
			result = lpddr4_ackctlinterrupt(pD, LPDDR4_INTR_MR_WRITE_DONE);
		if (result == (uint32_t)CDN_EOK) {
			lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
			*mrwStatus = (uint8_t)CPS_FLD_READ(LPDDR4__MRW_STATUS__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__MRW_STATUS__REG)));
			if ((*mrwStatus) != 0U)
				result = (uint32_t)CDN_EIO;
		}
	}

#ifdef ASILC
#endif

	return result;
}

uint32_t lpddr4_writectlconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_writectlconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;

	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_writereg(pD, LPDDR4_CTL_REGS, (uint32_t)regNum[aIndex],
							   (uint32_t)regValues[aIndex]);
	}
	return result;
}

uint32_t lpddr4_writephyindepconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_writephyindepconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;
	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_writereg(pD, LPDDR4_PHY_INDEP_REGS, (uint32_t)regNum[aIndex],
							   (uint32_t)regValues[aIndex]);
	}
	return result;
}

uint32_t lpddr4_writephyconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_writephyconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;
	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_writereg(pD, LPDDR4_PHY_REGS, (uint32_t)regNum[aIndex],
							   (uint32_t)regValues[aIndex]);
	}
	return result;
}

uint32_t lpddr4_readctlconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_readctlconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;
	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_readreg(pD, LPDDR4_CTL_REGS, (uint32_t)regNum[aIndex],
							  (uint32_t *)(&regValues[aIndex]));
	}
	return result;
}

uint32_t lpddr4_readphyindepconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_readphyindepconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;
	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_readreg(pD, LPDDR4_PHY_INDEP_REGS, (uint32_t)regNum[aIndex],
							  (uint32_t *)(&regValues[aIndex]));
	}
	return result;
}

uint32_t lpddr4_readphyconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount)
{
	uint32_t result;
	uint32_t aIndex;

	result = lpddr4_readphyconfigSF(pD);
	if ((regValues == (uint32_t *)NULL) || (regNum == (uint16_t *)NULL))
		result = CDN_EINVAL;
	if (result == (uint32_t)CDN_EOK) {
		for (aIndex = 0; aIndex < regCount; aIndex++)
			result = (uint32_t)lpddr4_readreg(pD, LPDDR4_PHY_REGS, (uint32_t)regNum[aIndex],
							  (uint32_t *)(&regValues[aIndex]));
	}
	return result;
}

uint32_t lpddr4_getphyindepinterruptmask(const lpddr4_privatedata *pD, uint32_t *mask)
{
	uint32_t result;

	result = LPDDR4_GetPhyIndepInterruptMSF(pD, mask);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		*mask = CPS_FLD_READ(LPDDR4__PI_INT_MASK__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_INT_MASK__REG)));
	}
	return result;
}

uint32_t lpddr4_setphyindepinterruptmask(const lpddr4_privatedata *pD, const uint32_t *mask)
{
	uint32_t result;
	uint32_t regVal = 0;
	const uint32_t ui32IrqCount = (uint32_t)LPDDR4_INTR_PHY_INDEP_DLL_LOCK_STATE_CHANGE_BIT + 1U;

	result = LPDDR4_SetPhyIndepInterruptMSF(pD, mask);
	if ((result == (uint32_t)CDN_EOK) && (ui32IrqCount < WORD_SHIFT)) {
		if (*mask >= (1U << ui32IrqCount))
			result = (uint32_t)CDN_EINVAL;
	}
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		regVal = CPS_FLD_WRITE(LPDDR4__PI_INT_MASK__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_INT_MASK__REG)), *mask);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__PI_INT_MASK__REG), regVal);
	}
	return result;
}

uint32_t lpddr4_checkphyindepinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr, bool *irqStatus)
{
	uint32_t result = 0;
	uint32_t phyIndepIrqStatus = 0;

	result = LPDDR4_INTR_CheckPhyIndepIntSF(pD, intr, irqStatus);
	if ((result == (uint32_t)CDN_EOK) && ((uint32_t)intr < WORD_SHIFT)) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		phyIndepIrqStatus = CPS_REG_READ(&(ctlRegBase->LPDDR4__PI_INT_STATUS__REG));
		*irqStatus = (bool)(((phyIndepIrqStatus >> (uint32_t)intr) & BIT_MASK) > 0U);
	}
	return result;
}

uint32_t lpddr4_ackphyindepinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	result = LPDDR4_INTR_AckPhyIndepIntSF(pD, intr);
	if ((result == (uint32_t)CDN_EOK) && ((uint32_t)intr < WORD_SHIFT)) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		regVal = ((uint32_t)BIT_MASK << (uint32_t)intr);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__PI_INT_ACK__REG), regVal);
	}

	return result;
}

static void lpddr4_checkcatrainingerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_ADR_CALVL_OBS1_0__REG));
	errBitMask = (CA_TRAIN_RL) | (NIBBLE_MASK);
	for (snum = 0U; snum < ASLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != CA_TRAIN_RL) {
			debugInfo->catraingerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

static void lpddr4_checkgatelvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_GTLVL_STATUS_OBS_0__REG));
	errBitMask = GATE_LVL_ERROR_FIELDS;
	for (snum = (uint32_t)0U; snum < DSLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != 0U) {
			debugInfo->gatelvlerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

static void lpddr4_checkreadlvlerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_RDLVL_STATUS_OBS_0__REG));
	errBitMask = READ_LVL_ERROR_FIELDS;
	for (snum = (uint32_t)0U; snum < DSLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != 0U) {
			debugInfo->readlvlerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

static void lpddr4_checkdqtrainingerror(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, bool *errFoundPtr)
{
	uint32_t regVal;
	uint32_t errBitMask = 0U;
	uint32_t snum;
	volatile uint32_t *regAddress;

	regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__PHY_WDQLVL_STATUS_OBS_0__REG));
	errBitMask = DQ_LVL_STATUS;
	for (snum = (uint32_t)0U; snum < DSLICE_NUM; snum++) {
		regVal = CPS_REG_READ(regAddress);
		if ((regVal & errBitMask) != 0U) {
			debugInfo->dqtrainingerror = CDN_TRUE;
			*errFoundPtr = true;
		}
		regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
	}
}

bool lpddr4_checklvlerrors(const lpddr4_privatedata *pD, lpddr4_debuginfo *debugInfo, bool errFound)
{
	bool localErrFound = errFound;

	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	if (localErrFound == (bool)false)
		lpddr4_checkcatrainingerror(ctlRegBase, debugInfo, &localErrFound);

	if (localErrFound == (bool)false)
		lpddr4_checkwrlvlerror(ctlRegBase, debugInfo, &localErrFound);

	if (localErrFound == (bool)false)
		lpddr4_checkgatelvlerror(ctlRegBase, debugInfo, &localErrFound);

	if (localErrFound == (bool)false)
		lpddr4_checkreadlvlerror(ctlRegBase, debugInfo, &localErrFound);

	if (localErrFound == (bool)false)
		lpddr4_checkdqtrainingerror(ctlRegBase, debugInfo, &localErrFound);
	return localErrFound;
}

static uint8_t lpddr4_seterror(volatile uint32_t *reg, uint32_t errBitMask, uint8_t *errFoundPtr, const uint32_t errorInfoBits)
{
	uint32_t regVal = 0U;

	regVal = CPS_REG_READ(reg);
	if ((regVal & errBitMask) != errorInfoBits)
		*errFoundPtr = CDN_TRUE;
	return *errFoundPtr;
}

void lpddr4_seterrors(lpddr4_ctlregs *ctlRegBase, lpddr4_debuginfo *debugInfo, uint8_t *errFoundPtr)
{
	uint32_t errBitMask = (BIT_MASK << 0x1U) | (BIT_MASK);

	debugInfo->pllerror = lpddr4_seterror(&(ctlRegBase->LPDDR4__PHY_PLL_OBS_0__REG),
					      errBitMask, errFoundPtr, PLL_READY);
	if (*errFoundPtr == CDN_FALSE)
		debugInfo->pllerror = lpddr4_seterror(&(ctlRegBase->LPDDR4__PHY_PLL_OBS_1__REG),
						      errBitMask, errFoundPtr, PLL_READY);

	if (*errFoundPtr == CDN_FALSE)
		debugInfo->iocaliberror = lpddr4_seterror(&(ctlRegBase->LPDDR4__PHY_CAL_RESULT_OBS_0__REG),
							  IO_CALIB_DONE, errFoundPtr, IO_CALIB_DONE);
	if (*errFoundPtr == CDN_FALSE)
		debugInfo->iocaliberror = lpddr4_seterror(&(ctlRegBase->LPDDR4__PHY_CAL_RESULT2_OBS_0__REG),
							  IO_CALIB_DONE, errFoundPtr, IO_CALIB_DONE);
	if (*errFoundPtr == CDN_FALSE)
		debugInfo->iocaliberror = lpddr4_seterror(&(ctlRegBase->LPDDR4__PHY_CAL_RESULT3_OBS_0__REG),
							  IO_CALIB_FIELD, errFoundPtr, IO_CALIB_STATE);
}

static void lpddr4_setphysnapsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound)
{
	uint32_t snum = 0U;
	volatile uint32_t *regAddress;
	uint32_t regVal = 0U;

	if (errorFound == (bool)false) {
		regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__SC_PHY_SNAP_OBS_REGS_0__REG));
		for (snum = (uint32_t)0U; snum < DSLICE_NUM; snum++) {
			regVal = CPS_FLD_SET(LPDDR4__SC_PHY_SNAP_OBS_REGS_0__FLD, CPS_REG_READ(regAddress));
			CPS_REG_WRITE(regAddress, regVal);
			regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
		}
	}
}

static void lpddr4_setphyadrsnapsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound)
{
	uint32_t snum = 0U;
	volatile uint32_t *regAddress;
	uint32_t regVal = 0U;

	if (errorFound == (bool)false) {
		regAddress = (volatile uint32_t *)(&(ctlRegBase->LPDDR4__SC_PHY_ADR_SNAP_OBS_REGS_0__REG));
		for (snum = (uint32_t)0U; snum < ASLICE_NUM; snum++) {
			regVal = CPS_FLD_SET(LPDDR4__SC_PHY_ADR_SNAP_OBS_REGS_0__FLD, CPS_REG_READ(regAddress));
			CPS_REG_WRITE(regAddress, regVal);
			regAddress = lpddr4_addoffset(regAddress, (uint32_t)SLICE_WIDTH);
		}
	}
}

void lpddr4_setsettings(lpddr4_ctlregs *ctlRegBase, const bool errorFound)
{
	lpddr4_setphysnapsettings(ctlRegBase, errorFound);
	lpddr4_setphyadrsnapsettings(ctlRegBase, errorFound);
}

static void readpdwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_PD_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_PD_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_PD_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F2__REG)));
}

static void readsrshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_SHORT_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_SHORT_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_SHORT_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F2__REG)));
}

static void readsrlongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F2__REG)));
}

static void readsrlonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F2__REG)));
}

static void readsrdpshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F2__REG)));
}

static void readsrdplongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F2__REG)));
}

static void readsrdplonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, uint32_t *cycles)
{
	if (*fspNum == LPDDR4_FSP_0)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F0__REG)));
	else if (*fspNum == LPDDR4_FSP_1)
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F1__REG)));
	else
		*cycles = CPS_FLD_READ(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F2__REG)));

}

static void lpddr4_readlpiwakeuptime(lpddr4_ctlregs *ctlRegBase, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, uint32_t *cycles)
{
	if (*lpiWakeUpParam == LPDDR4_LPI_PD_WAKEUP_FN)
		readpdwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_SHORT_WAKEUP_FN)
		readsrshortwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_LONG_WAKEUP_FN)
		readsrlongwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_LONG_MCCLK_GATE_WAKEUP_FN)
		readsrlonggatewakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SRPD_SHORT_WAKEUP_FN)
		readsrdpshortwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SRPD_LONG_WAKEUP_FN)
		readsrdplongwakeup(fspNum, ctlRegBase, cycles);
	else
		readsrdplonggatewakeup(fspNum, ctlRegBase, cycles);
}

uint32_t lpddr4_getlpiwakeuptime(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, uint32_t *cycles)
{
	uint32_t result = 0U;

	result = lpddr4_getlpiwakeuptimeSF(pD, lpiWakeUpParam, fspNum, cycles);
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		lpddr4_readlpiwakeuptime(ctlRegBase, lpiWakeUpParam, fspNum, cycles);
	}
	return result;
}

static void writepdwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_PD_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_PD_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_PD_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_PD_WAKEUP_F2__REG), regVal);
	}
}

static void writesrshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_SHORT_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_SHORT_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_SHORT_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_SHORT_WAKEUP_F2__REG), regVal);
	}
}

static void writesrlongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_WAKEUP_F2__REG), regVal);
	}
}

static void writesrlonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SR_LONG_MCCLK_GATE_WAKEUP_F2__REG), regVal);
	}
}

static void writesrdpshortwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_SHORT_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_SHORT_WAKEUP_F2__REG), regVal);
	}
}

static void writesrdplongwakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_WAKEUP_F2__REG), regVal);
	}
}
static void writesrdplonggatewakeup(const lpddr4_ctlfspnum *fspNum, lpddr4_ctlregs *ctlRegBase, const uint32_t *cycles)
{
	uint32_t regVal = 0U;

	if (*fspNum == LPDDR4_FSP_0) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F0__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F0__REG), regVal);
	} else if (*fspNum == LPDDR4_FSP_1) {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F1__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F1__REG), regVal);
	} else {
		regVal = CPS_FLD_WRITE(LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F2__REG)), *cycles);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_F2__REG), regVal);
	}
}

static void lpddr4_writelpiwakeuptime(lpddr4_ctlregs *ctlRegBase, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles)
{
	if (*lpiWakeUpParam == LPDDR4_LPI_PD_WAKEUP_FN)
		writepdwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_SHORT_WAKEUP_FN)
		writesrshortwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_LONG_WAKEUP_FN)
		writesrlongwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SR_LONG_MCCLK_GATE_WAKEUP_FN)
		writesrlonggatewakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SRPD_SHORT_WAKEUP_FN)
		writesrdpshortwakeup(fspNum, ctlRegBase, cycles);
	else if (*lpiWakeUpParam == LPDDR4_LPI_SRPD_LONG_WAKEUP_FN)
		writesrdplongwakeup(fspNum, ctlRegBase, cycles);
	else
		writesrdplonggatewakeup(fspNum, ctlRegBase, cycles);
}

uint32_t lpddr4_setlpiwakeuptime(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles)
{
	uint32_t result = 0U;

	result = lpddr4_setlpiwakeuptimeSF(pD, lpiWakeUpParam, fspNum, cycles);
	if (result == (uint32_t)CDN_EOK) {
		if (*cycles > NIBBLE_MASK)
			result = (uint32_t)CDN_EINVAL;
	}
	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		lpddr4_writelpiwakeuptime(ctlRegBase, lpiWakeUpParam, fspNum, cycles);
	}
	return result;
}

uint32_t lpddr4_getdbireadmode(const lpddr4_privatedata *pD, bool *on_off)
{
	uint32_t result = 0U;

	result = lpddr4_getdbireadmodeSF(pD, on_off);

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		if (CPS_FLD_READ(LPDDR4__RD_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__RD_DBI_EN__REG))) == 0U)
			*on_off = false;
		else
			*on_off = true;
	}
	return result;
}

uint32_t lpddr4_getdbiwritemode(const lpddr4_privatedata *pD, bool *on_off)
{
	uint32_t result = 0U;

	result = lpddr4_getdbireadmodeSF(pD, on_off);

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		if (CPS_FLD_READ(LPDDR4__WR_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__WR_DBI_EN__REG))) == 0U)
			*on_off = false;
		else
			*on_off = true;
	}
	return result;
}

uint32_t lpddr4_setdbimode(const lpddr4_privatedata *pD, const lpddr4_dbimode *mode)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	result = lpddr4_setdbimodeSF(pD, mode);

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		if (*mode == LPDDR4_DBI_RD_ON)
			regVal = CPS_FLD_WRITE(LPDDR4__RD_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__RD_DBI_EN__REG)), 1U);
		else if (*mode == LPDDR4_DBI_RD_OFF)
			regVal = CPS_FLD_WRITE(LPDDR4__RD_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__RD_DBI_EN__REG)), 0U);
		else if (*mode == LPDDR4_DBI_WR_ON)
			regVal = CPS_FLD_WRITE(LPDDR4__WR_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__WR_DBI_EN__REG)), 1U);
		else
			regVal = CPS_FLD_WRITE(LPDDR4__WR_DBI_EN__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__WR_DBI_EN__REG)), 0U);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__RD_DBI_EN__REG), regVal);
	}
	return result;
}

uint32_t lpddr4_getrefreshrate(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, uint32_t *tref, uint32_t *tras_max)
{
	uint32_t result = 0U;

	result = lpddr4_getrefreshrateSF(pD, fspNum, tref, tras_max);

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

		switch (*fspNum) {
		case LPDDR4_FSP_2:
			*tref = CPS_FLD_READ(LPDDR4__TREF_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F2__REG)));
			*tras_max = CPS_FLD_READ(LPDDR4__TRAS_MAX_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F2__REG)));
			break;
		case LPDDR4_FSP_1:
			*tref = CPS_FLD_READ(LPDDR4__TREF_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F1__REG)));
			*tras_max = CPS_FLD_READ(LPDDR4__TRAS_MAX_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F1__REG)));
			break;
		default:
			*tref = CPS_FLD_READ(LPDDR4__TREF_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F0__REG)));
			*tras_max = CPS_FLD_READ(LPDDR4__TRAS_MAX_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F0__REG)));
			break;
		}
	}
	return result;
}

static void lpddr4_updatefsp2refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t regVal = 0U;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_WRITE(LPDDR4__TREF_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F2__REG)), *tref);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TREF_F2__REG), regVal);
	regVal = CPS_FLD_WRITE(LPDDR4__TRAS_MAX_F2__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F2__REG)), *tras_max);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TRAS_MAX_F2__REG), regVal);
}

static void lpddr4_updatefsp1refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t regVal = 0U;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_WRITE(LPDDR4__TREF_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F1__REG)), *tref);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TREF_F1__REG), regVal);
	regVal = CPS_FLD_WRITE(LPDDR4__TRAS_MAX_F1__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F1__REG)), *tras_max);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TRAS_MAX_F1__REG), regVal);;
}

static void lpddr4_updatefsp0refrateparams(const lpddr4_privatedata *pD, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t regVal = 0U;
	lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;

	regVal = CPS_FLD_WRITE(LPDDR4__TREF_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_F0__REG)), *tref);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TREF_F0__REG), regVal);
	regVal = CPS_FLD_WRITE(LPDDR4__TRAS_MAX_F0__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TRAS_MAX_F0__REG)), *tras_max);
	CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TRAS_MAX_F0__REG), regVal);
}

uint32_t lpddr4_setrefreshrate(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t result = 0U;

	result = lpddr4_setrefreshrateSF(pD, fspNum, tref, tras_max);

	if (result == (uint32_t)CDN_EOK) {
		switch (*fspNum) {
		case LPDDR4_FSP_2:
			lpddr4_updatefsp2refrateparams(pD, tref, tras_max);
			break;
		case LPDDR4_FSP_1:
			lpddr4_updatefsp1refrateparams(pD, tref, tras_max);
			break;
		default:
			lpddr4_updatefsp0refrateparams(pD, tref, tras_max);
			break;
		}
	}
	return result;
}

uint32_t lpddr4_refreshperchipselect(const lpddr4_privatedata *pD, const uint32_t trefInterval)
{
	uint32_t result = 0U;
	uint32_t regVal = 0U;

	result = lpddr4_refreshperchipselectSF(pD);

	if (result == (uint32_t)CDN_EOK) {
		lpddr4_ctlregs *ctlRegBase = (lpddr4_ctlregs *)pD->ctlbase;
		regVal = CPS_FLD_WRITE(LPDDR4__TREF_INTERVAL__FLD, CPS_REG_READ(&(ctlRegBase->LPDDR4__TREF_INTERVAL__REG)), trefInterval);
		CPS_REG_WRITE(&(ctlRegBase->LPDDR4__TREF_INTERVAL__REG), regVal);
	}
	return result;
}
