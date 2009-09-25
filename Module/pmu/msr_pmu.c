/******************************************************************************\
 * FILE: msr_pmu.c
 * DESCRIPTION: Provides functions to manipulate the PMU MSR's
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>

#include <seeker.h>

#include "pmu_int.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: event select register contents */
extern evtsel_t evtsel[NR_CPUS][NUM_COUNTERS];

/* main.c: counter contents */
extern counter_t counters[NR_CPUS][NUM_COUNTERS];

/* main.c: cleared value of counters */
extern cleared_t cleared[NR_CPUS][NUM_COUNTERS];

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/


/*******************************************************************************
 * evtsel_read - Read and return the low 32 bits of the evtsel reg.
 * @evtsel_num - The counter number
 * @Side Effects - None.
 * @return - The low 32 bits of the evtsel register.
 *
 * Reads and returns the low 32 bits of the evtsel register on the 
 * current cpu corrosponding evtsel number.
 *******************************************************************************/
inline u32 evtsel_read(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	rdmsr(evtsel[smp_processor_id()][evtsel_num].addr, low, high);
	return low;
#else
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(evtsel_read);

/*******************************************************************************
 * evtsel_clear - Clears the evtsel register on _this_ cpu.
 * @evtsel_num - The counter number
 * @Side Effects - None
 *
 * clears the evtsel register on this cpu. 
 *******************************************************************************/
inline void evtsel_clear(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	u32 cpu = smp_processor_id();
	rdmsr(evtsel[cpu][evtsel_num].addr, low, high);
	low &= EVTSEL_RESERVED_BITS;
	high &= EVTSEL_RESERVED_BITS_HIGH;
	wrmsr(evtsel[cpu][evtsel_num].addr, low, high);
#endif
}

EXPORT_SYMBOL_GPL(evtsel_clear);

/*******************************************************************************
 * evtsel_write - Write evtsel discreptions to register on _this_ cpu.
 * @evtsel_num - the counter number.
 * @Side Effects - Counter evtsel_num's configuration is written to register.
 *
 * This writes the configuration for counter evtsel_num in 
 * evtsel[_this_cpu_][evtsel_num] to registers. 
 * NOTE: It assumes that you have the values required _already_ in evtsel 
 * before calling this function. 
 *******************************************************************************/
inline void evtsel_write(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	int cpu = smp_processor_id();
	if (likely(cpu < NR_CPUS)) {
		evtsel_t *cur_evtsel = &(evtsel[cpu][evtsel_num]);

		rdmsr(cur_evtsel->addr, low, high);
		low &= EVTSEL_RESERVED_BITS;

		low |= ((cur_evtsel->ev_select & BITS(LOW_BITS_EVT_SEL)) << SHIFT_EVT_SEL)
		    | (cur_evtsel->ev_mask << SHIFT_EVT_MASK)
		    | (cur_evtsel->usr_flag << SHIFT_USR_FLAG)
		    | (cur_evtsel->os_flag << SHIFT_OS_FLAG)
		    | (cur_evtsel->edge << SHIFT_EDGE_FLAG)
		    | (cur_evtsel->pc_flag << SHIFT_PC_FLAG)
		    | (cur_evtsel->int_flag << SHIFT_INT_FLAG)
		    | (cur_evtsel->enabled << SHIFT_ENABLED)
		    | (cur_evtsel->inv_flag << SHIFT_INV_FLAG)
		    | (cur_evtsel->cnt_mask << SHIFT_CNT_MASK);

		high &= EVTSEL_RESERVED_BITS_HIGH;
#if defined(ARCH_K8) || defined(ARCH_K10)
		high |= ((cur_evtsel->ev_select >> SHIFT_VAL_HIGH_EVT_SEL) << SHIFT_HIGH_EVT_SEL)
			| (cur_evtsel->go << SHIFT_HIGH_GO_FLAG)
			| (cur_evtsel->ho << SHIFT_HIGH_HO_FLAG);
#endif

		wrmsr(cur_evtsel->addr, low, high);
	}
#endif
}

EXPORT_SYMBOL_GPL(evtsel_write);

/*******************************************************************************
 * counter_clear - Clears counter `counter` on _this_ cpu.
 * @counter - The counter to be cleared.
 * 
 * Clears counter `counter` on _this_ cpu. Usually called using on_each_cpu
 * or smp_cal_function_single.
 *******************************************************************************/
inline void counter_clear(u32 counter)
{
#if NUM_COUNTERS > 0
	int cpu_id;
	cpu_id = smp_processor_id();
	if (unlikely(counter >= NUM_COUNTERS)) {
		error("Trying to clear non-existant counter %d", counter);
		return;
	}
	if (likely(cpu_id < NR_CPUS)) {
		wrmsr(counters[cpu_id][counter].addr, 0, 0);
	}
#endif
}

EXPORT_SYMBOL_GPL(counter_clear);

/*******************************************************************************
 * counter_read - Read all configured counters on _this_ cpu.
 * @Side Effects - counters[_this_cpu_] is populated for each counter.
 *
 * Reads all enabled/configured counters on _this_ cpu and place them in
 * counters[_this_cpu_] for each enabled counter. To get the values, use
 * get_counter_data. 
 *******************************************************************************/
void counter_read(void)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		/* this is the "full" read of the full 48bits */
		for (i = 0; i < NUM_COUNTERS; i++) {
			if(counters[cpu_id][i].enabled == 0)
				continue;
			rdmsr(counters[cpu_id][i].addr, low, high);
			counters[cpu_id][i].high = high;
			counters[cpu_id][i].low = low;
		}
	}
#endif
}

EXPORT_SYMBOL_GPL(counter_read);

/*******************************************************************************
 * get_counter_data - Get read counter values. 
 * @counter - The counter's value required.
 * @cpu_id - The cpu for which the counter value is required.
 * @Side Effects - None.
 *
 * Gets the cached counter value for counter `counter` on cpu `cpu_id`.
 * NOTE: This does _NOT_ read the counter value, but only returns the
 * cached value from a prior call to counter_read. 
 *******************************************************************************/
u64 get_counter_data(u32 counter, u32 cpu_id)
{
#if NUM_COUNTERS > 0
	u64 counter_val;
	if (unlikely(counter >= NUM_COUNTERS))
		return 0;
	counter_val = (u64) counters[cpu_id][counter].low;
	counter_val |= ((u64) counters[cpu_id][counter].high << 32);
	return counter_val;
#else
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(get_counter_data);

/*******************************************************************************
 * counter_disable - Disable a counter on _this_ cpu.
 * @counter - The counter to disable. 
 * 
 * Disable the counter `counter` on _this_ cpu. Typical use is to call this
 * from within a function which itself is called using smp_call_function_single
 * or on_each_cpu. 
 *******************************************************************************/
inline void counter_disable(int counter)
{
#if NUM_COUNTERS > 0
	int cpu_id = smp_processor_id();
	if (unlikely(counter >= NUM_COUNTERS || cpu_id >= NR_CPUS))
		return;

	if(counters[cpu_id][counter].enabled == 0)
		return;

	evtsel_clear(counter);
	counter_clear(counter);
	evtsel[cpu_id][counter].enabled = 0;
	counters[cpu_id][counter].enabled = 0;
	evtsel_write(counter);
#endif
}

EXPORT_SYMBOL_GPL(counter_disable);

/*******************************************************************************
 * counter_enable - Enable a counter on _this_ cpu.
 * @event - The event for the counter to count.
 * @ev_mask - The mask for the event for the counter (Manual, Manual, Manual).
 * @os - If 1, counter will count even in CPL=0 (Kernel Mode). Else counts in
 *       only User mode.
 * @return - The counter number of the counter which was configured with these
 *           details. -1 if a free counter was unavaliable.
 * @Side Effects - counters and evtsel data structures are initialized.
 *
 * Configures and enables a counter on _this_ cpu and returns the counter number
 * -1 if failed. Typical use is to call this from within another function which
 *  itself is called using smp_call_function_single or on_each_cpu.
 *******************************************************************************/
int counter_enable(u32 event, u32 ev_mask, u32 os)
{
#if NUM_COUNTERS > 0
	u32 i;
	int counter_num = -1;
	int cpu_id = smp_processor_id();
	if (unlikely(cpu_id >= NR_CPUS))
		return -1;

	for (i = 0; i < NUM_COUNTERS; i++) {
		if (counters[cpu_id][i].enabled == 0) {
			counter_num = i;
			break;
		}
	}
	if(unlikely(counter_num < 0))
		return -1;

	evtsel_clear(counter_num);
	counter_clear(counter_num);
	counters[cpu_id][counter_num].enabled = 1;
	counters[cpu_id][counter_num].event = event;
	counters[cpu_id][counter_num].mask = ev_mask;
	//counfigure the event sel reg
	evtsel[cpu_id][counter_num].ev_select = event;
	evtsel[cpu_id][counter_num].ev_mask = ev_mask;
	evtsel[cpu_id][counter_num].usr_flag = 1;
	evtsel[cpu_id][counter_num].os_flag = (os & 1);
	evtsel[cpu_id][counter_num].pc_flag = 1;
	evtsel[cpu_id][counter_num].int_flag = 0;
	evtsel[cpu_id][counter_num].inv_flag = 0;
	evtsel[cpu_id][counter_num].cnt_mask = 0;
	evtsel[cpu_id][counter_num].enabled = 1;
	evtsel[cpu_id][counter_num].edge = 0;
	evtsel_write(counter_num);
	return counter_num;
#else
	return -1;
#endif
}

EXPORT_SYMBOL_GPL(counter_enable);

