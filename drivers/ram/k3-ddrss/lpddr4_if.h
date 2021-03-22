/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */


#ifndef LPDDR4_IF_H
#define LPDDR4_IF_H

#include "cdn_stdtypes.h"
#ifdef CONFIG_K3_AM64_DDRSS
#include <lpddr4_16bit_if.h>
#else
#include <lpddr4_32bit_if.h>
#endif


typedef struct lpddr4_config_s lpddr4_config;
typedef struct lpddr4_privatedata_s lpddr4_privatedata;
typedef struct lpddr4_debuginfo_s lpddr4_debuginfo;
typedef struct lpddr4_fspmoderegs_s lpddr4_fspmoderegs;


typedef enum {
	LPDDR4_CTL_REGS		= 0U,
	LPDDR4_PHY_REGS		= 1U,
	LPDDR4_PHY_INDEP_REGS	= 2U
} lpddr4_regblock;

typedef enum {
	LPDDR4_DRV_NONE			= 0U,
	LPDDR4_DRV_SOC_PLL_UPDATE	= 1U
} lpddr4_infotype;

typedef enum {
	LPDDR4_LPI_PD_WAKEUP_FN				= 0U,
	LPDDR4_LPI_SR_SHORT_WAKEUP_FN			= 1U,
	LPDDR4_LPI_SR_LONG_WAKEUP_FN			= 2U,
	LPDDR4_LPI_SR_LONG_MCCLK_GATE_WAKEUP_FN		= 3U,
	LPDDR4_LPI_SRPD_SHORT_WAKEUP_FN			= 4U,
	LPDDR4_LPI_SRPD_LONG_WAKEUP_FN			= 5U,
	LPDDR4_LPI_SRPD_LONG_MCCLK_GATE_WAKEUP_FN	= 6U
} lpddr4_lpiwakeupparam;

typedef enum {
	LPDDR4_REDUC_ON		= 0U,
	LPDDR4_REDUC_OFF	= 1U
} lpddr4_reducmode;

typedef enum {
	LPDDR4_ECC_DISABLED		= 0U,
	LPDDR4_ECC_ENABLED		= 1U,
	LPDDR4_ECC_ERR_DETECT		= 2U,
	LPDDR4_ECC_ERR_DETECT_CORRECT	= 3U
} lpddr4_eccenable;

typedef enum {
	LPDDR4_DBI_RD_ON	= 0U,
	LPDDR4_DBI_RD_OFF	= 1U,
	LPDDR4_DBI_WR_ON	= 2U,
	LPDDR4_DBI_WR_OFF	= 3U
} lpddr4_dbimode;

typedef enum {
	LPDDR4_FSP_0	= 0U,
	LPDDR4_FSP_1	= 1U,
	LPDDR4_FSP_2	= 2U
} lpddr4_ctlfspnum;

typedef void (*lpddr4_infocallback)(const lpddr4_privatedata *pD, lpddr4_infotype infoType);

typedef void (*lpddr4_ctlcallback)(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt ctlInterrupt, uint8_t chipSelect);

typedef void (*lpddr4_phyindepcallback)(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt phyIndepInterrupt, uint8_t chipSelect);




uint32_t lpddr4_probe(const lpddr4_config *config, uint16_t *configSize);

uint32_t lpddr4_init(lpddr4_privatedata *pD, const lpddr4_config *cfg);

uint32_t lpddr4_start(const lpddr4_privatedata *pD);

uint32_t lpddr4_readreg(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t *regValue);

uint32_t lpddr4_writereg(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t regValue);

uint32_t lpddr4_getmmrregister(const lpddr4_privatedata *pD, uint32_t readModeRegVal, uint64_t *mmrValue, uint8_t *mmrStatus);

uint32_t lpddr4_setmmrregister(const lpddr4_privatedata *pD, uint32_t writeModeRegVal, uint8_t *mrwStatus);

uint32_t lpddr4_writectlconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_writephyconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_writephyindepconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_readctlconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_readphyconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_readphyindepconfig(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

uint32_t lpddr4_getctlinterruptmask(const lpddr4_privatedata *pD, uint64_t *mask);

uint32_t lpddr4_setctlinterruptmask(const lpddr4_privatedata *pD, const uint64_t *mask);

uint32_t lpddr4_checkctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr, bool *irqStatus);

uint32_t lpddr4_ackctlinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr);

uint32_t lpddr4_getphyindepinterruptmask(const lpddr4_privatedata *pD, uint32_t *mask);

uint32_t lpddr4_setphyindepinterruptmask(const lpddr4_privatedata *pD, const uint32_t *mask);

uint32_t lpddr4_checkphyindepinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr, bool *irqStatus);

uint32_t lpddr4_ackphyindepinterrupt(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr);

uint32_t lpddr4_getdebuginitinfo(const lpddr4_privatedata *pD, lpddr4_debuginfo *debugInfo);

uint32_t lpddr4_getlpiwakeuptime(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, uint32_t *cycles);

uint32_t lpddr4_setlpiwakeuptime(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles);

uint32_t lpddr4_geteccenable(const lpddr4_privatedata *pD, lpddr4_eccenable *eccParam);

uint32_t lpddr4_seteccenable(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam);

uint32_t lpddr4_getreducmode(const lpddr4_privatedata *pD, lpddr4_reducmode *mode);

uint32_t lpddr4_setreducmode(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode);

uint32_t lpddr4_getdbireadmode(const lpddr4_privatedata *pD, bool *on_off);

uint32_t lpddr4_getdbiwritemode(const lpddr4_privatedata *pD, bool *on_off);

uint32_t lpddr4_setdbimode(const lpddr4_privatedata *pD, const lpddr4_dbimode *mode);

uint32_t lpddr4_getrefreshrate(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, uint32_t *tref, uint32_t *tras_max);

uint32_t lpddr4_setrefreshrate(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max);

uint32_t lpddr4_refreshperchipselect(const lpddr4_privatedata *pD, const uint32_t trefInterval);



#endif  /* LPDDR4_IF_H */
