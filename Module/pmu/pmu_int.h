
/*****************************************************
 * Copyright 2008 Amithash Prasad                    *
 *                                                   *
 * This file is part of Seeker                       *
 *                                                   *
 * Seeker is free software: you can redistribute     *
 * it and/or modify it under the terms of the        *
 * GNU General Public License as published by        *
 * the Free Software Foundation, either version      *
 * 3 of the License, or (at your option) any         *
 * later version.                                    *
 *                                                   *
 * This program is distributed in the hope that      *
 * it will be useful, but WITHOUT ANY WARRANTY;      *
 * without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE. See the GNU General Public License       *
 * for more details.                                 *
 *                                                   *
 * You should have received a copy of the GNU        *
 * General Public License along with this program.   *
 * If not, see <http://www.gnu.org/licenses/>.       *
 *****************************************************/

#ifndef _PMU_INT_H_
#define _PMU_INT_H_

#include <asm/types.h>
#include <pmu_public.h>

/********************************************************************************
 * 				Useful Macros					*
 ********************************************************************************/

#define BITS(n) ((1 << (n)) - 1)
#define BITS_AT(nbits,at) (BITS(nbits) << (at))


/********************************************************************************
 * 			Register Mask Constants					*
 ********************************************************************************/

#if defined(ARCH_C2D)
#	define EVTSEL_RESERVED_BITS BITS_AT(1,21)
#	define EVTSEL_RESERVED_BITS_HIGH (~(0))
#	define CTR0_OVERFLOW_MASK BITS_AT(1,0)
#	define CTR1_OVERFLOW_MASK BITS_AT(1,1)
#	define CTR0_OVERFLOW_CLEAR_MASK (~BITS_AT(1,0))
#	define CTR1_OVERFLOW_CLEAR_MASK (~BITS_AT(1,1))
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define EVTSEL_RESERVED_BITS BITS_AT(1,21)
#	define EVTSEL_RESERVED_BITS_HIGH (BITS_AT(4,4) | BITS_AT(22,10))
#else
#error "Architecture not supported."
#endif

/********************************************************************************
 * 			Register Address Constants				*
 ********************************************************************************/

#if defined(ARCH_C2D)
#	define EVTSEL0 			0x00000186
#	define EVTSEL1 			0x00000187
#	define PMC0 			0x000000C1
#	define PMC1 			0x000000C2
#	define MSR_PERF_GLOBAL_STATUS 	0x0000038E
#	define MSR_PERF_GLOBAL_CTRL	0x0000038F
#	define MSR_PERF_GLOBAL_OVF_CTRL	0x00000390
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define EVTSEL0 			0xC0010000
#	define EVTSEL1 			0xC0010001
#	define EVTSEL2 			0xC0010002
#	define EVTSEL3 			0xC0010003
#	define PMC0 			0xC0010004
#	define PMC1 			0xC0010005
#	define PMC2 			0xC0010006
#	define PMC3 			0xC0010007
#else
#error "Architecture Not supported"
#endif

/********************************************************************************
 * 				Prototypes 					*
 ********************************************************************************/

/* evtsel register elements */
typedef struct {
#if defined(ARCH_K8) || defined(ARCH_K10)
	u32 ev_select:12;
#else
	u32 ev_select:8;
#endif
	u32 ev_mask:8;
	u32 usr_flag:1;
	u32 os_flag:1;
	u32 edge:1;
	u32 pc_flag:1;
	u32 int_flag:1;
	u32 rsvd:1;
	u32 enabled:1;
	u32 inv_flag:1;
	u32 cnt_mask:8;
#if defined(ARCH_K8) || defined(ARCH_K10)
	u32 ho:1;
	u32 go:1;
#endif
	u32 addr;
} evtsel_t;

/* Cleared value */
typedef struct {
	u32 low;
	u32 high;
	u64 all;
} cleared_t;

/* Counter discreption */
typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
	u32 event;
	u32 mask;
	u32 enabled;
} counter_t;

/********************************************************************************
 * 			PMU Internal API 					*
 ********************************************************************************/
#include <pmu.h>

inline u32 evtsel_read(u32 evtsel_num);
inline void evtsel_clear(u32 evtsel_num);
inline void evtsel_write(u32 evtsel_num);

#endif

