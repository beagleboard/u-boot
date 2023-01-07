/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */


#ifndef INCLUDE_CDN_LOG_H

#define INCLUDE_CDN_LOG_H

#include "cdn_assert.h"
#include "cdn_inttypes.h"



#ifdef DEBUG
#if DEBUG
#define CFP_DBG_MSG 1
#endif
#endif

#define CLIENT_MSG         0x01000000

#define DBG_GEN_MSG        0xFFFFFFFF

#define DBG_CRIT 0
#define DBG_WARN 5
#define DBG_FYI 10
#define DBG_HIVERB 100
#define DBG_INFLOOP 200

#ifdef _HAVE_DBG_LOG_INT_
uint32_t g_dbg_enable_log = 0;
#else
extern uint32_t g_dbg_enable_log;
#endif

#ifdef _HAVE_DBG_LOG_INT_
uint32_t g_dbg_log_lvl = DBG_CRIT;
uint32_t g_dbg_log_cnt = 0;
uint32_t g_dbg_state = 0;
#else
extern uint32_t g_dbg_log_lvl;
extern uint32_t g_dbg_log_cnt;
extern uint32_t g_dbg_state;
#endif





#ifdef DEBUG
extern void DbgPrint(const char *fmt, ...);
#define cDbgMsg(_t, _x, ...) (((_x) == 0) || \
			      (((_t) & g_dbg_enable_log) && ((_x) <= g_dbg_log_lvl)) ? \
			      DbgPrint(__VA_ARGS__) : 0)
#else
#define cDbgMsg(_t, _x, ...)
#endif // DEBUG

#ifdef CFP_DBG_MSG
#define DbgMsg(t, x, ...)  cDbgMsg((t), (x), __VA_ARGS__)
#else
#define DbgMsg(t, x, ...)
#endif


#define DEBUG_PREFIX "[%-20.20s %4d %4" PRId32 "]-"

// ******** Default vDbgMsg ********
#  define vDbgMsg(log_lvl, module, msg, ...)    DbgMsg((log_lvl), (module), (DEBUG_PREFIX msg), __func__, \
						       __LINE__, g_dbg_log_cnt++, __VA_ARGS__)

// ******** Default cvDbgMsg ********
#  define cvDbgMsg(log_lvl, module, msg, ...)   cDbgMsg((log_lvl), (module), (DEBUG_PREFIX msg), __func__, \
							__LINE__, g_dbg_log_cnt++, __VA_ARGS__)

// ******** Default cvDbgMsg ********
#  define evDbgMsg(log_lvl, module, msg, ...)   { cDbgMsg((log_lvl), (module), (DEBUG_PREFIX msg), __func__,         \
							  __LINE__, g_dbg_log_cnt++, __VA_ARGS__); \
						  assert(0); }

#define DbgMsgSetLvl(x) (g_dbg_log_lvl = (x))
#define DbgMsgEnableModule(x) (g_dbg_enable_log |= (x))
#define DbgMsgDisableModule(x) (g_dbg_enable_log &= ~((uint32_t)(x)))
#define DbgMsgClearAll(_x) (g_dbg_enable_log = (_x))

#define SetDbgState(_x) (g_dbg_state = (_x))
#define GetDbgState       (g_dbg_state)


#endif // INCLUDE_CDN_LOG_H
