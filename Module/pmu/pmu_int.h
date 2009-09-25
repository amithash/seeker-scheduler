/******************************************************************************\
 * FILE: pmu_int.h
 * DESCRIPTION: The internal header for PMU. Must NOT be included by any other
 * module or user space.
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 * Copyright 2006 Tipp Mosely                                                  *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

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

#define		EVTSEL_RESERVED_BITS		BITS_AT(1,21)
#define		EVTSEL_RESERVED_BITS_HIGH	(~(0))
#define		CTR0_OVERFLOW_MASK		BITS_AT(1,0)
#define		CTR1_OVERFLOW_MASK		BITS_AT(1,1)
#define		CTR0_OVERFLOW_CLEAR_MASK	(~BITS_AT(1,0))
#define		CTR1_OVERFLOW_CLEAR_MASK	(~BITS_AT(1,1))
#define		FIXSEL_RESERVED_BITS		(~(BITS_AT(2,0) | BITS_AT(3,3) \
						| BITS_AT(3,7) | BITS_AT(1,11)))
#define		FIXED_CTR0_OVERFLOW_MASK	BITS_AT(1,0)
#define		FIXED_CTR1_OVERFLOW_MASK	BITS_AT(1,1)
#define		FIXED_CTR2_OVERFLOW_MASK	BITS_AT(1,2)
#define		FIXED_CTR0_OVERFLOW_CLEAR_MASK	(~BITS_AT(1,0))
#define		FIXED_CTR1_OVERFLOW_CLEAR_MASK	(~BITS_AT(1,1))
#define		FIXED_CTR2_OVERFLOW_CLEAR_MASK	(~BITS_AT(1,2))

#elif defined(ARCH_K8) || defined(ARCH_K10)

#define		EVTSEL_RESERVED_BITS		BITS_AT(1,21)
#define		EVTSEL_RESERVED_BITS_HIGH	(BITS_AT(4,4) | BITS_AT(22,10))

#else

#error "Architecture not supported."

#endif

/********************************************************************************
 * 			Register Address Constants				*
 ********************************************************************************/

#if defined(ARCH_C2D)

#define		EVTSEL0 			0x00000186
#define		EVTSEL1 			0x00000187
#define		PMC0				0x000000C1
#define		PMC1				0x000000C2
#define		MSR_PERF_GLOBAL_STATUS		0x0000038E
#define		MSR_PERF_GLOBAL_CTRL		0x0000038F
#define		MSR_PERF_GLOBAL_OVF_CTRL	0x00000390
#define		MSR_PERF_FIXED_CTR0 		0x00000309
#define		MSR_PERF_FIXED_CTR1 		0x0000030A
#define		MSR_PERF_FIXED_CTR2 		0x0000030B
#define		MSR_PERF_FIXED_CTR_CTRL 	0x0000038D
#define		MSR_PERF_GLOBAL_STATUS 		0x0000038E
#define		MSR_PERF_GLOBAL_CTRL		0x0000038F
#define		MSR_PERF_GLOBAL_OVF_CTRL	0x00000390

#elif defined(ARCH_K8) || defined(ARCH_K10)

#define		EVTSEL0 			0xC0010000
#define		EVTSEL1 			0xC0010001
#define		EVTSEL2 			0xC0010002
#define		EVTSEL3 			0xC0010003
#define		PMC0				0xC0010004
#define		PMC1				0xC0010005
#define		PMC2				0xC0010006
#define		PMC3				0xC0010007

#else

#error "Architecture Not supported"

#endif

/********************************************************************************
 * 			Register Bit Locations					*
 ********************************************************************************/
#if defined(ARCH_C2D)

#define		SHIFT_EVT_SEL		0	/* bits 0-7 */
#define		LOW_BITS_EVT_SEL	8	/* low 8 bits */
#define		SHIFT_EVT_MASK		8	/* bits 8-15 */
#define		SHIFT_USR_FLAG		16	/* bit 16 */
#define		SHIFT_OS_FLAG		17	/* bit 17 */
#define		SHIFT_EDGE_FLAG		18	/* bit 18 */
#define		SHIFT_PC_FLAG		19	/* bit 19 */
#define		SHIFT_INT_FLAG		20	/* bit 20 */
#define		SHIFT_ENABLED		22	/* bit 22 */
#define		SHIFT_INV_FLAG		23	/* bit 23 */
#define		SHIFT_CNT_MASK		24	/* bits 24-31 */

#define		SHIFT_FIX_OS0		0	/* bit 0 */
#define		SHIFT_FIX_USR0		1	/* bit 1 */
#define		SHIFT_FIX_PMI0		3	/* bit 3 */
#define		SHIFT_FIX_OS1		4	/* bit 4 */
#define		SHIFT_FIX_USR1		5	/* bit 5 */
#define		SHIFT_FIX_PMI1		7	/* bit 7 */
#define		SHIFT_FIX_OS2		8	/* bit 8 */
#define		SHIFT_FIX_USR2		9	/* bit 9 */
#define		SHIFT_FIX_PMI2		11	/* bit 11 */

#elif defined(ARCH_K8) || defined(ARCH_K10)

#define		SHIFT_EVT_SEL		0	/* bits 0-7 */
#define		LOW_BITS_EVT_SEL	8	/* low 8 bits */
#define		SHIFT_EVT_MASK		8	/* bits 8-15 */
#define		SHIFT_USR_FLAG		16	/* bit 16 */
#define		SHIFT_OS_FLAG		17	/* bit 17 */
#define		SHIFT_EDGE_FLAG		18	/* bit 18 */
#define		SHIFT_PC_FLAG		19	/* bit 19 */
#define		SHIFT_INT_FLAG		20	/* bit 20 */
#define		SHIFT_ENABLED		22	/* bit 22 */
#define		SHIFT_INV_FLAG		23	/* bit 23 */
#define		SHIFT_CNT_MASK		24	/* bits 24-31 */
#define		SHIFT_HIGH_EV_SEL	(32-32)	/* bits 32-35 */
#define		SHIFT_HIGH_GO_FLAG	(40-32)	/* bit 40 */
#define		SHIFT_HIGH_HO_FLAG	(41-32)	/* bit 41 */

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

/* fixed counter select register elements */
typedef struct {
	u32 pmi0:1;
	u32 os0:1;
	u32 usr0:1;
	u32 pmi1:1;
	u32 os1:1;
	u32 usr1:1;
	u32 pmi2:1;
	u32 os2:1;
	u32 usr2:1;
} fixctrl_t;


/* Cleared value */
typedef struct {
	u32 low;
	u32 high;
	u64 all;
} cleared_t;

/* Cleared value */
typedef struct {
	u32 low;
	u32 high;
	u64 all;
} fcleared_t;

/* Counter discreption */
typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
	u32 event;
	u32 mask;
	u32 enabled;
} counter_t;

/* Counter discreption */
typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
} fcounter_t;


/********************************************************************************
 * 			PMU Internal API 					*
 ********************************************************************************/
#include <pmu.h>

inline u32 evtsel_read(u32 evtsel_num);
inline void evtsel_clear(u32 evtsel_num);
inline void evtsel_write(u32 evtsel_num);
inline u32 control_read(void);
inline void control_clear(void);
inline void control_write(void);

#endif

