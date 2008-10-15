
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


#ifndef _FPMU_H_
#define _FPMU_H_

#include <asm/types.h>
#include <fpmu_public.h>

/********** Constants ********************************************************/
#ifdef ARCH_C2D
#	define FIXSEL_RESERVED_BITS 0xFFFFF444
#	define FIXED_CTR0_OVERFLOW_MASK 0x00000001
#	define FIXED_CTR1_OVERFLOW_MASK 0x00000002
#	define FIXED_CTR2_OVERFLOW_MASK 0x00000004
#	define FIXED_CTR0_OVERFLOW_CLEAR_MASK 0xFFFFFFFE
#	define FIXED_CTR1_OVERFLOW_CLEAR_MASK 0xFFFFFFFD
#	define FIXED_CTR2_OVERFLOW_CLEAR_MASK 0xFFFFFFFB
#elif defined(ARCH_K8) || defined(ARCH_K10)
#else
#	error "Architecture Not supported"
#endif

/********* MSR's *************************************************************/

#if defined(ARCH_C2D)
#	define MSR_PERF_FIXED_CTR0 		0x00000309
#	define MSR_PERF_FIXED_CTR1 		0x0000030A
#	define MSR_PERF_FIXED_CTR2 		0x0000030B
#	define MSR_PERF_FIXED_CTR_CTRL 	0x0000038D
#	define MSR_PERF_GLOBAL_STATUS 		0x0000038E
#	define MSR_PERF_GLOBAL_CTRL		0x0000038F
#	define MSR_PERF_GLOBAL_OVF_CTRL	0x00000390
#endif

/********** Structure Definitions ********************************************/
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

typedef struct {
	u32 low;
	u32 high;
	u64 all;
}fcleared_t;

typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
} fcounter_t;

/********* Extern Vars *******************************************************/
extern fixctrl_t fcontrol[NR_CPUS];
extern fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS];
extern char* fcounter_names[NUM_FIXED_COUNTERS];
extern fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS];

/********** Function Prototypes **********************************************/

void fpmu_init_msrs(void);

inline u32 control_read(void);
inline void control_clear(void);
inline void control_write(void);

void fcounter_clear(u32 counter);
void fcounter_read(void);
u64 get_fcounter_data(u32 counter, u32 cpu_id);
void fcounters_disable(void);
void fcounters_enable(u32 os);

int fpmu_configure_interrupt(int ctr, u32 low, u32 high);
int fpmu_enable_interrupt(int ctr);
int fpmu_disable_interrupt(int ctr);
int fpmu_clear_ovf_status(int ctr);
int fpmu_is_interrupt(int ctr);

#endif
