
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

#ifndef _FPMU_INT_H_
#define _FPMU_INT_H_

#include <asm/types.h>
#include <fpmu_public.h>

/********************************************************************************
 * 				Useful Macros					*
 ********************************************************************************/

#define BITS(n) ((1 << (n)) - 1)
#define BITS_AT(nbits,at) (BITS(nbits) << (at))

/********************************************************************************
 * 			Register Mask Constants					*
 ********************************************************************************/

#ifdef ARCH_C2D
#	define FIXSEL_RESERVED_BITS \
	(~(BITS_AT(2,0) | BITS_AT(3,3) | BITS_AT(3,7) | BITS_AT(1,11)))
#	define FIXED_CTR0_OVERFLOW_MASK BITS_AT(1,0)
#	define FIXED_CTR1_OVERFLOW_MASK BITS_AT(1,1)
#	define FIXED_CTR2_OVERFLOW_MASK BITS_AT(1,2)
#	define FIXED_CTR0_OVERFLOW_CLEAR_MASK (~BITS_AT(1,0))
#	define FIXED_CTR1_OVERFLOW_CLEAR_MASK (~BITS_AT(1,1))
#	define FIXED_CTR2_OVERFLOW_CLEAR_MASK (~BITS_AT(1,2))
#elif defined(ARCH_K8) || defined(ARCH_K10)
#else
#	error "Architecture Not supported"
#endif

/********************************************************************************
 * 			Register Address Constants				*
 ********************************************************************************/

#if defined(ARCH_C2D)
#	define MSR_PERF_FIXED_CTR0 		0x00000309
#	define MSR_PERF_FIXED_CTR1 		0x0000030A
#	define MSR_PERF_FIXED_CTR2 		0x0000030B
#	define MSR_PERF_FIXED_CTR_CTRL 		0x0000038D
#	define MSR_PERF_GLOBAL_STATUS 		0x0000038E
#	define MSR_PERF_GLOBAL_CTRL		0x0000038F
#	define MSR_PERF_GLOBAL_OVF_CTRL		0x00000390
#endif

/********************************************************************************
 * 				Prototypes 					*
 ********************************************************************************/

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
} fcleared_t;

/* Counter discreption */
typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
} fcounter_t;

/********************************************************************************
 * 			FPMU Internal API 					*
 ********************************************************************************/

inline u32 control_read(void);
inline void control_clear(void);
inline void control_write(void);

#include <fpmu.h>

#endif
