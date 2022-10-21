/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */

#ifndef lpddr4_obj_IF_H
#define lpddr4_obj_IF_H

#include "lpddr4_if.h"


typedef struct lpddr4_obj_s {
	uint32_t (*probe)(const lpddr4_config *config, uint16_t *configSize);

	uint32_t (*init)(lpddr4_privatedata *pD, const lpddr4_config *cfg);

	uint32_t (*start)(const lpddr4_privatedata *pD);

	uint32_t (*readreg)(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t *regValue);

	uint32_t (*writereg)(const lpddr4_privatedata *pD, lpddr4_regblock cpp, uint32_t regOffset, uint32_t regValue);

	uint32_t (*getmmrregister)(const lpddr4_privatedata *pD, uint32_t readModeRegVal, uint64_t *mmrValue, uint8_t *mmrStatus);

	uint32_t (*setmmrregister)(const lpddr4_privatedata *pD, uint32_t writeModeRegVal, uint8_t *mrwStatus);

	uint32_t (*writectlconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*writephyconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*writephyindepconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*readctlconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*readphyconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*readphyindepconfig)(const lpddr4_privatedata *pD, uint32_t regValues[], uint16_t regNum[], uint16_t regCount);

	uint32_t (*getctlinterruptmask)(const lpddr4_privatedata *pD, uint64_t *mask);

	uint32_t (*setctlinterruptmask)(const lpddr4_privatedata *pD, const uint64_t *mask);

	uint32_t (*checkctlinterrupt)(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr, bool *irqStatus);

	uint32_t (*ackctlinterrupt)(const lpddr4_privatedata *pD, lpddr4_intr_ctlinterrupt intr);

	uint32_t (*getphyindepinterruptmask)(const lpddr4_privatedata *pD, uint32_t *mask);

	uint32_t (*setphyindepinterruptmask)(const lpddr4_privatedata *pD, const uint32_t *mask);

	uint32_t (*checkphyindepinterrupt)(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr, bool *irqStatus);

	uint32_t (*ackphyindepinterrupt)(const lpddr4_privatedata *pD, lpddr4_intr_phyindepinterrupt intr);

	uint32_t (*getdebuginitinfo)(const lpddr4_privatedata *pD, lpddr4_debuginfo *debugInfo);

	uint32_t (*getlpiwakeuptime)(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, uint32_t *cycles);

	uint32_t (*setlpiwakeuptime)(const lpddr4_privatedata *pD, const lpddr4_lpiwakeupparam *lpiWakeUpParam, const lpddr4_ctlfspnum *fspNum, const uint32_t *cycles);

	uint32_t (*geteccenable)(const lpddr4_privatedata *pD, lpddr4_eccenable *eccParam);

	uint32_t (*seteccenable)(const lpddr4_privatedata *pD, const lpddr4_eccenable *eccParam);

	uint32_t (*getreducmode)(const lpddr4_privatedata *pD, lpddr4_reducmode *mode);

	uint32_t (*setreducmode)(const lpddr4_privatedata *pD, const lpddr4_reducmode *mode);

	uint32_t (*getdbireadmode)(const lpddr4_privatedata *pD, bool *on_off);

	uint32_t (*getdbiwritemode)(const lpddr4_privatedata *pD, bool *on_off);

	uint32_t (*setdbimode)(const lpddr4_privatedata *pD, const lpddr4_dbimode *mode);

	uint32_t (*getrefreshrate)(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, uint32_t *tref, uint32_t *tras_max);

	uint32_t (*setrefreshrate)(const lpddr4_privatedata *pD, const lpddr4_ctlfspnum *fspNum, const uint32_t *tref, const uint32_t *tras_max);

	uint32_t (*refreshperchipselect)(const lpddr4_privatedata *pD, const uint32_t trefInterval);
} lpddr4_obj;

extern lpddr4_obj *lpddr4_getinstance(void);



#endif  /* lpddr4_obj_IF_H */
