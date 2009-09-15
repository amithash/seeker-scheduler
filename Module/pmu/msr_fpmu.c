/******************************************************************************\
 * FILE: msr_fpmu.c
 * DESCRIPTION: Provides functions to manipulate the MSR related to fixed
 * PMU
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>
#include <linux/percpu.h>

#include <seeker.h>

#include "pmu_int.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: fpmu control register contents */
extern fixctrl_t fcontrol[NR_CPUS];

/* main.c: counter contents */
extern fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS];

/* main.c: cleared value of counters */
extern fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS];

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * control_read - Read and return the low 32 bits of the fpmu control reg.
 * @Side Effects - None.
 * @return - The low 32 bits of the evtsel register.
 *
 * Reads and returns the low 32 bits of the fpmu control register on the 
 * current cpu.
 *******************************************************************************/
inline u32 control_read(void)
{
#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
	return low;
#else
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(control_read);

/*******************************************************************************
 * control_clear - Clears the fpmu control register on _this_ cpu.
 * @Side Effects - None
 *
 * clears the fpmu control register on this cpu. 
 *******************************************************************************/
inline void control_clear(void)
{
#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
	low &= FIXSEL_RESERVED_BITS;
	wrmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
#endif
}

EXPORT_SYMBOL_GPL(control_clear);

/*******************************************************************************
 * control_write - Write fpmu control discreptions to register on _this_ cpu.
 * @Side Effects - Counter configuration is written to register.
 *
 * This writes the configuration for the fixed counters counter 
 * fcontrol[_this_cpu_] to registers. 
 * NOTE: It assumes that you have the values required _already_ in fcontrol
 * before calling this function. 
 *******************************************************************************/
inline void control_write(void)
{
#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	int cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		fixctrl_t *cur_control = &(fcontrol[cpu_id]);

		rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
		low &= FIXSEL_RESERVED_BITS;

		low = (cur_control->os0 << 0)
		    | (cur_control->usr0 << 1)
		    | (cur_control->pmi0 << 3)
		    | (cur_control->os1 << 4)
		    | (cur_control->usr1 << 5)
		    | (cur_control->pmi1 << 7)
		    | (cur_control->os2 << 8)
		    | (cur_control->usr2 << 9)
		    | (cur_control->pmi2 << 11);

		wrmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
	}
#endif
}

EXPORT_SYMBOL_GPL(control_write);

/*******************************************************************************
 * fcounter_clear - Clears counter `counter` on _this_ cpu.
 * @counter - The counter to be cleared.
 * 
 * Clears counter `counter` on _this_ cpu. Usually called from a function
 * which itself is called using on_each_cpu or smp_cal_function_single.
 *******************************************************************************/
inline void fcounter_clear(u32 counter)
{
#if NUM_FIXED_COUNTERS > 0
	int cpu_id;
	cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		wrmsr(fcounters[cpu_id][counter].addr,
		      fcleared[cpu_id][counter].low,
		      fcleared[cpu_id][counter].high);
		wrmsr(fcounters[cpu_id][counter].addr,
		      fcleared[cpu_id][counter].low,
		      fcleared[cpu_id][counter].high);
	}
#endif
}

EXPORT_SYMBOL_GPL(fcounter_clear);

/*******************************************************************************
 * fcounter_read - Read all fixed counters on _this_ cpu.
 * @Side Effects - fcounters[_this_cpu_] is populated for each counter.
 *
 * Reads all fixed counters on _this_ cpu and place them in fcounters[_this_cpu_]
 * for each fixed counter. To get the values, use get_fcounter_data. 
 *******************************************************************************/
void fcounter_read(void)
{
#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		/* this is the "full" read of the full 64bits */
		for (i = 0; i < NUM_FIXED_COUNTERS; i++) {
			rdmsr(fcounters[cpu_id][i].addr, low, high);
			fcounters[cpu_id][i].high = high;
			fcounters[cpu_id][i].low = low;
		}
	}
#endif
}

EXPORT_SYMBOL_GPL(fcounter_read);

/*******************************************************************************
 * get_fcounter_data - Get read counter values. 
 * @counter - The counter's value required.
 * @cpu_id - The cpu for which the counter value is required.
 * @Side Effects - None.
 *
 * Gets the cached counter value for counter `counter` on cpu `cpu_id`.
 * NOTE: This does _NOT_ read the counter value, but only returns the
 * cached value from a prior call to fcounter_read. 
 *******************************************************************************/
u64 get_fcounter_data(u32 counter, u32 cpu_id)
{
#if NUM_FIXED_COUNTERS > 0
	u64 counter_val;
	if (likely(counter < NUM_FIXED_COUNTERS && cpu_id < NR_CPUS)) {
		counter_val = (u64) fcounters[cpu_id][counter].low;
		counter_val =
		    counter_val | ((u64) fcounters[cpu_id][counter].high << 32);
		return counter_val - fcleared[cpu_id][counter].all;
	} else {
		return -1;
	}
#else
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(get_fcounter_data);

/*******************************************************************************
 * fcounter_disable - Disable all fixed counters on _this_ cpu.
 * 
 * Disable all fixed ounters on _this_ cpu. Typical use is to call this
 * from within a function which itself is called using smp_call_function_single
 * or on_each_cpu. 
 *******************************************************************************/
inline void fcounters_disable(void)
{
#if NUM_FIXED_COUNTERS > 0
	int i;
	int cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		for (i = 0; i < NUM_FIXED_COUNTERS; i++) {
			fcounter_clear(i);
		}
		control_clear();
	}
#endif
}

EXPORT_SYMBOL_GPL(fcounters_disable);

/*******************************************************************************
 * fcounter_enable - Enable all fixed counters on _this_ cpu.
 * @os - If 1, counter will count even in CPL=0 (Kernel Mode). Else counts in
 *       only User mode.
 * @Side Effects - fcounters and fcontrol data structures are initialized.
 *
 * Configures and enables all fixed counters on _this_ cpu. Typical use is to 
 * call this from within another function which itself is called using 
 * smp_call_function_single or on_each_cpu.
 *******************************************************************************/
void fcounters_enable(u32 os)
{
#if NUM_FIXED_COUNTERS > 0
	u32 i;
	int cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		for (i = 0; i < NUM_FIXED_COUNTERS; i++) {
			fcounter_clear(i);
		}
		fcontrol[cpu_id].os0 = os;
		fcontrol[cpu_id].os1 = os;
		fcontrol[cpu_id].os2 = os;
		fcontrol[cpu_id].usr0 = 1;
		fcontrol[cpu_id].usr1 = 1;
		fcontrol[cpu_id].usr2 = 1;
		fcontrol[cpu_id].pmi0 = 0;
		fcontrol[cpu_id].pmi1 = 0;
		fcontrol[cpu_id].pmi2 = 0;
		control_write();
	}
#endif
}

EXPORT_SYMBOL_GPL(fcounters_enable);

