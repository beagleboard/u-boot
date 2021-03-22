/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */




#ifndef LPDDR4_SANITY_H
#define LPDDR4_SANITY_H

#include "cdn_errno.h"
#include "cdn_stdtypes.h"
#include "lpddr4_if.h"
#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t lpddr4_configsf(const lpddr4_config *obj);
static inline uint32_t lpddr4_privatedatasf(const lpddr4_privatedata *obj);

static inline uint32_t lpddr4_sanityfunction1(const lpddr4_config *config, const uint16_t *configSize);
static inline uint32_t lpddr4_sanityfunction2(const lpddr4_privatedata *pD, const lpddr4_config *cfg);
static inline uint32_t lpddr4_sanityfunction3(const lpddr4_privatedata *pD);
static inline uint32_t lpddr4_sanityfunction4(const lpddr4_privatedata *pD, const lpddr4_regblock cpp, const uint32_t *regValue);
static inline uint32_t lpddr4_sanityfunction5(const lpddr4_privatedata *pD, const lpddr4_regblock cpp);
static inline uint32_t lpddr4_sanityfunction6(const lpddr4_privatedata *pD, const uint64_t *mmrValue, const uint8_t *mmrStatus);
static inline uint32_t lpddr4_sanityfunction7(const lpddr4_privatedata *pD, const uint8_t *mrwStatus);
static inline uint32_t lpddr4_sanityfunction14(const lpddr4_privatedata *pD, const uint64_t *mask);
static inline uint32_t lpddr4_sanityfunction15(const lpddr4_privatedata *pD, const uint64_t *mask);
static inline uint32_t lpddr4_sanityfunction16(const lpddr4_privatedata *pD, const uint32_t *mask);
static inline uint32_t lpddr4_sanityfunction18(const lpddr4_privatedata *pD, const lpddr4_debuginfo *debugInfo);
static inline uint32_t lpddr4_sanityfunction19(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles);
static inline uint32_t lpddr4_sanityfunction21(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam);
static inline uint32_t lpddr4_sanityfunction22(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam);
static inline uint32_t lpddr4_sanityfunction23(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode);
static inline uint32_t lpddr4_sanityfunction24(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode);
static inline uint32_t lpddr4_sanityfunction25(const lpddr4_privatedata *pD, const bool *on_off);
static inline uint32_t lpddr4_sanityfunction27(const lpddr4_privatedata *pD, const lpddr4_dbimode *mode);
static inline uint32_t lpddr4_sanityfunction28(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max);
static inline uint32_t lpddr4_sanityfunction29(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max);

#define lpddr4_probeSF lpddr4_sanityfunction1
#define lpddr4_initSF lpddr4_sanityfunction2
#define lpddr4_startSF lpddr4_sanityfunction3
#define lpddr4_readregSF lpddr4_sanityfunction4
#define lpddr4_writeregSF lpddr4_sanityfunction5
#define lpddr4_getmmrregisterSF lpddr4_sanityfunction6
#define lpddr4_setmmrregisterSF lpddr4_sanityfunction7
#define lpddr4_writectlconfigSF lpddr4_sanityfunction3
#define lpddr4_writephyconfigSF lpddr4_sanityfunction3
#define lpddr4_writephyindepconfigSF lpddr4_sanityfunction3
#define lpddr4_readctlconfigSF lpddr4_sanityfunction3
#define lpddr4_readphyconfigSF lpddr4_sanityfunction3
#define lpddr4_readphyindepconfigSF lpddr4_sanityfunction3
#define lpddr4_getctlinterruptmaskSF lpddr4_sanityfunction14
#define lpddr4_setctlinterruptmaskSF lpddr4_sanityfunction15
#define LPDDR4_GetPhyIndepInterruptMSF lpddr4_sanityfunction16
#define LPDDR4_SetPhyIndepInterruptMSF lpddr4_sanityfunction16
#define lpddr4_getdebuginitinfoSF lpddr4_sanityfunction18
#define lpddr4_getlpiwakeuptimeSF lpddr4_sanityfunction19
#define lpddr4_setlpiwakeuptimeSF lpddr4_sanityfunction19
#define lpddr4_geteccenableSF lpddr4_sanityfunction21
#define lpddr4_seteccenableSF lpddr4_sanityfunction22
#define lpddr4_getreducmodeSF lpddr4_sanityfunction23
#define lpddr4_setreducmodeSF lpddr4_sanityfunction24
#define lpddr4_getdbireadmodeSF lpddr4_sanityfunction25
#define lpddr4_getdbiwritemodeSF lpddr4_sanityfunction25
#define lpddr4_setdbimodeSF lpddr4_sanityfunction27
#define lpddr4_getrefreshrateSF lpddr4_sanityfunction28
#define lpddr4_setrefreshrateSF lpddr4_sanityfunction29
#define lpddr4_refreshperchipselectSF lpddr4_sanityfunction3

static inline uint32_t lpddr4_configsf(const lpddr4_config *obj)
{
	uint32_t ret = 0;

	if (obj == NULL)
		ret = CDN_EINVAL;

	return ret;
}

static inline uint32_t lpddr4_privatedatasf(const lpddr4_privatedata *obj)
{
	uint32_t ret = 0;

	if (obj == NULL)
		ret = CDN_EINVAL;

	return ret;
}

static inline uint32_t lpddr4_sanityfunction1(const lpddr4_config *config, const uint16_t *configSize)
{
	uint32_t ret = 0;

	if (configSize == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_configsf(config) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction2(const lpddr4_privatedata *pD, const lpddr4_config *cfg)
{
	uint32_t ret = 0;

	if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_configsf(cfg) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction3(const lpddr4_privatedata *pD)
{
	uint32_t ret = 0;

	if (lpddr4_privatedatasf(pD) == CDN_EINVAL)
		ret = CDN_EINVAL;

	return ret;
}

static inline uint32_t lpddr4_sanityfunction4(const lpddr4_privatedata *pD, const lpddr4_regblock cpp, const uint32_t *regValue)
{
	uint32_t ret = 0;

	if (regValue == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(cpp != LPDDR4_CTL_REGS) &&
		(cpp != LPDDR4_PHY_REGS) &&
		(cpp != LPDDR4_PHY_INDEP_REGS)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction5(const lpddr4_privatedata *pD, const lpddr4_regblock cpp)
{
	uint32_t ret = 0;

	if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(cpp != LPDDR4_CTL_REGS) &&
		(cpp != LPDDR4_PHY_REGS) &&
		(cpp != LPDDR4_PHY_INDEP_REGS)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction6(const lpddr4_privatedata *pD, const uint64_t *mmrValue, const uint8_t *mmrStatus)
{
	uint32_t ret = 0;

	if (mmrValue == NULL) {
		ret = CDN_EINVAL;
	} else if (mmrStatus == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction7(const lpddr4_privatedata *pD, const uint8_t *mrwStatus)
{
	uint32_t ret = 0;

	if (mrwStatus == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction14(const lpddr4_privatedata *pD, const uint64_t *mask)
{
	uint32_t ret = 0;

	if (mask == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction15(const lpddr4_privatedata *pD, const uint64_t *mask)
{
	uint32_t ret = 0;

	if (mask == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction16(const lpddr4_privatedata *pD, const uint32_t *mask)
{
	uint32_t ret = 0;

	if (mask == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction18(const lpddr4_privatedata *pD, const lpddr4_debuginfo *debugInfo)
{
	uint32_t ret = 0;

	if (debugInfo == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction19(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles)
{
	uint32_t ret = 0;

	if (lpiWakeUpParam == NULL) {
		ret = CDN_EINVAL;
	} else if (fspNum == NULL) {
		ret = CDN_EINVAL;
	} else if (cycles == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*lpiWakeUpParam != LPDDR4_LPI_PD_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SR_SHORT_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SR_LONG_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SR_LONG_MCCLK_GATE_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SRPD_SHORT_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SRPD_LONG_WAKEUP_FN) &&
		(*lpiWakeUpParam != LPDDR4_LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_FN)
		) {
		ret = CDN_EINVAL;
	} else if (
		(*fspNum != LPDDR4_FSP_0) &&
		(*fspNum != LPDDR4_FSP_1) &&
		(*fspNum != LPDDR4_FSP_2)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction21(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam)
{
	uint32_t ret = 0;

	if (eccParam == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction22(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam)
{
	uint32_t ret = 0;

	if (eccParam == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*eccParam != LPDDR4_ECC_DISABLED) &&
		(*eccParam != LPDDR4_ECC_ENABLED) &&
		(*eccParam != LPDDR4_ECC_ERR_DETECT) &&
		(*eccParam != LPDDR4_ECC_ERR_DETECT_CORRECT)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction23(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode)
{
	uint32_t ret = 0;

	if (mode == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction24(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode)
{
	uint32_t ret = 0;

	if (mode == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*mode != LPDDR4_REDUC_ON) &&
		(*mode != LPDDR4_REDUC_OFF)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction25(const lpddr4_privatedata *pD, const bool *on_off)
{
	uint32_t ret = 0;

	if (on_off == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction27(const lpddr4_privatedata *pD, const lpddr4_dbimode *mode)
{
	uint32_t ret = 0;

	if (mode == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*mode != LPDDR4_DBI_RD_ON) &&
		(*mode != LPDDR4_DBI_RD_OFF) &&
		(*mode != LPDDR4_DBI_WR_ON) &&
		(*mode != LPDDR4_DBI_WR_OFF)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction28(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t ret = 0;

	if (fspNum == NULL) {
		ret = CDN_EINVAL;
	} else if (tref == NULL) {
		ret = CDN_EINVAL;
	} else if (tras_max == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*fspNum != LPDDR4_FSP_0) &&
		(*fspNum != LPDDR4_FSP_1) &&
		(*fspNum != LPDDR4_FSP_2)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

static inline uint32_t lpddr4_sanityfunction29(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max)
{
	uint32_t ret = 0;

	if (fspNum == NULL) {
		ret = CDN_EINVAL;
	} else if (tref == NULL) {
		ret = CDN_EINVAL;
	} else if (tras_max == NULL) {
		ret = CDN_EINVAL;
	} else if (lpddr4_privatedatasf(pD) == CDN_EINVAL) {
		ret = CDN_EINVAL;
	} else if (
		(*fspNum != LPDDR4_FSP_0) &&
		(*fspNum != LPDDR4_FSP_1) &&
		(*fspNum != LPDDR4_FSP_2)
		) {
		ret = CDN_EINVAL;
	} else {
	}

	return ret;
}

#ifdef __cplusplus
}
#endif


#endif  /* LPDDR4_SANITY_H */
