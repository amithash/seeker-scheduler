
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>

#include <seeker.h>

#include "pmu_int.h"

extern evtsel_t evtsel[NR_CPUS][NUM_COUNTERS];
extern counter_t counters[NR_CPUS][NUM_COUNTERS];
extern cleared_t cleared[NR_CPUS][NUM_COUNTERS];

//read the evtsel reg and return the low 32 bits
//the high are reserved anyway
inline u32 evtsel_read(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	rdmsr(evtsel[0][evtsel_num].addr, low, high);
	return low;
#else
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(evtsel_read);

//clears the event select registers
inline void evtsel_clear(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	rdmsr(evtsel[0][evtsel_num].addr, low, high);
	low &= EVTSEL_RESERVED_BITS;
	high &= EVTSEL_RESERVED_BITS_HIGH;
	wrmsr(evtsel[0][evtsel_num].addr, low, high);
#endif
}

EXPORT_SYMBOL_GPL(evtsel_clear);

//write to the respective evtsel register
inline void evtsel_write(u32 evtsel_num)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	int cpu = smp_processor_id();
	if (likely(cpu < NR_CPUS)) {
		evtsel_t *cur_evtsel = &(evtsel[cpu][evtsel_num]);

		rdmsr(cur_evtsel->addr, low, high);
		low &= EVTSEL_RESERVED_BITS;

		low |= ((cur_evtsel->ev_select & BITS(8)) << 0) /*bits 0-7 */
		    | (cur_evtsel->ev_mask << 8) /* bits 8-15 */
		    | (cur_evtsel->usr_flag << 16) /* bit 16 */
		    | (cur_evtsel->os_flag << 17) /* bit 17 */
		    | (cur_evtsel->edge << 18) /* bit 18 */
		    | (cur_evtsel->pc_flag << 19) /* bit 19 */
		    | (cur_evtsel->int_flag << 20) /* bit 20 */
		    | (cur_evtsel->enabled << 22) /* bit 22 */
		    | (cur_evtsel->inv_flag << 23) /* bit 23 */
		    | (cur_evtsel->cnt_mask << 24); /* bits 24-31 */

		high &= EVTSEL_RESERVED_BITS_HIGH;
#if defined(ARCH_K8) || defined(ARCH_K10)
		high |= ((cur_evtsel->ev_select >> 8) << 0) /* bits 0(32)-3(35) */
			| (cur_evtsel->go << 8) /* bit 8(40) */
			| (cur_evtsel->ho << 9); /* bit 9(41) */
#endif

		wrmsr(cur_evtsel->addr, low, high);
	}
#endif
}

EXPORT_SYMBOL_GPL(evtsel_write);

//must be called using ON_EACH_CPU
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

//must be called using ON_EACH_CPU
void counter_read(void)
{
#if NUM_COUNTERS > 0
	u32 low, high;
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		/* this is the "full" read of the full 48bits */
		for (i = 0; i < NUM_COUNTERS; i++) {
			rdmsr(counters[cpu_id][i].addr, low, high);
			counters[cpu_id][i].high = high;
			counters[cpu_id][i].low = low;
		}
	}
#endif
}

EXPORT_SYMBOL_GPL(counter_read);

//use this to get the counter data
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

//must be called using ON_EACH_CPU
inline void counter_disable(int counter)
{
#if NUM_COUNTERS > 0
	int cpu_id = smp_processor_id();
	if (unlikely(counter >= NUM_COUNTERS || cpu_id >= NR_CPUS))
		return;

	evtsel_clear(counter);
	counter_clear(counter);
	evtsel[cpu_id][counter].enabled = 0;
	counters[cpu_id][counter].enabled = 0;
	evtsel_write(counter);
#endif
}

EXPORT_SYMBOL_GPL(counter_disable);

//must be called using ON_EACH_CPU
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
