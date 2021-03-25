/******************************************************************************
 * Copyright (C) 2014-2018 Cadence Design Systems, Inc.
 * All rights reserved worldwide
 *
 * The material contained herein is the proprietary and confidential
 * information of Cadence or its licensors, and is supplied subject to, and may
 * be used only by Cadence's customer in accordance with a previously executed
 * license and maintenance agreement between Cadence and that customer.
 *
 ******************************************************************************
 * cps.c
 *
 * Sample implementation of Cadence Platform Services for a bare-metal
 * system
 ******************************************************************************
 */


#include "cps.h"
#include <string.h>
#define _weak __attribute__((weak))


/* see cps.h */
_weak uint32_t cps_readreg32(volatile uint32_t* address) {
    return *address;
}

/* see cps.h */
_weak void cps_writereg32(volatile uint32_t* address, uint32_t value) {
    *address = value;
}

/* see cps.h */
_weak uint8_t cps_uncachedread8(volatile uint8_t* address) {
    return *address;
}

/* see cps.h */
_weak uint16_t cps_uncachedread16(volatile uint16_t* address) {
    return *address;
}

/* see cps.h */
_weak uint32_t cps_uncachedread32(volatile uint32_t* address) {
    return *address;
}

/* see cps.h */
_weak uint64_t cps_uncachedread64(volatile uint64_t* address) {
    return *address;
}

/* see cps.h */
_weak uint64_t cps_readreg64(volatile uint64_t* address) {
    return *address;
}

/* see cps.h */
_weak void cps_uncachedwrite8(volatile uint8_t* address, uint8_t value) {
    *address = value;
}

/* see cps.h */
_weak void cps_uncachedwrite16(volatile uint16_t* address, uint16_t value) {
    *address = value;
}

/* see cps.h */
_weak void cps_uncachedwrite32(volatile uint32_t* address, uint32_t value) {
    *address = value;
}

/* see cps.h */
_weak void cps_uncachedwrite64(volatile uint64_t* address, uint64_t value) {
    *address = value;
}

/* see cps.h */
_weak void cps_writereg64(volatile uint64_t* address, uint64_t value) {
    *address = value;
}

/* see cps.h */
_weak void cps_writephysaddress32(volatile uint32_t* location, uint32_t addrValue) {
    *location = addrValue;
}

/* see cps.h */
_weak void cps_buffercopy(volatile uint8_t *dst, volatile const uint8_t *src, uint32_t size) {
    memcpy((void*)dst, (void*)src, size);
}

/* see cps.h */
_weak void cps_cacheinvalidate(void* address, size_t size, uintptr_t devInfo) {
    (void) address;
    (void) size;
    (void) devInfo;
}

/* see cps.h */
_weak void cps_cacheflush(void* address, size_t size, uintptr_t devInfo) {
    (void) address;
    (void) size;
    (void) devInfo;
}

/* see cps.h */
_weak void cps_delayns(uint32_t ns) {
    (void) ns;
}

/* see cps.h */
_weak void cps_memorybarrier(void) {

}

/* see cps.h */
_weak void cps_memorybarrierwrite(void) {

}

/* see cps.h */
_weak void cps_memorybarrierread(void) {

}
