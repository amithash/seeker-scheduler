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

#include "pmu.h"

extern evtsel_t  evtsel[NR_CPUS][NUM_COUNTERS];
extern counter_t counters[NR_CPUS][NUM_COUNTERS];
extern cleared_t cleared[NR_CPUS][NUM_COUNTERS];

int pmu_configure_interrupt(int ctr, u32 low, u32 high)
{
	#if NUM_COUNTERS > 0
	int ret = 0;
	int cpu = smp_processor_id();
	if(likely(ctr < NUM_COUNTERS)){
		cleared[cpu][ctr].low = low;
		cleared[cpu][ctr].high = high;
		cleared[cpu][ctr].all = (u64)low | (((u64)high) << 32);
	}
	else{
		ret =  -1;
	}
	return -1;
	#else
	return 0;
	#endif
}
EXPORT_SYMBOL_GPL(pmu_configure_interrupt);

int pmu_enable_interrupt(int ctr)
{
	#if NUM_COUNTERS > 0
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS && ctr < NUM_COUNTERS)){
		evtsel[cpu_id][ctr].int_flag = 1;
		evtsel_write(ctr);
	}
	else{
		return -1;
	}
	return 0;
	#else
	return -1;
	#endif
}	
EXPORT_SYMBOL_GPL(pmu_enable_interrupt);

int pmu_disable_interrupt(int ctr)
{
	#if NUM_COUNTERS > 0
	int cpu;
	cpu = smp_processor_id();
	if(likely(ctr < NUM_COUNTERS && cpu < NR_CPUS)){
		cleared[cpu][ctr].low = 0;
		cleared[cpu][ctr].high = 0;
		cleared[cpu][ctr].all = 0;
		evtsel[cpu][ctr].int_flag = 0;
		evtsel_write(ctr);
	}
	else{
		return -1;
	}
	return 0;
	#else
	return -1;
	#endif
}	
EXPORT_SYMBOL_GPL(pmu_disable_interrupt);

int pmu_is_interrupt(int ctr)
{
	#if NUM_COUNTERS > 0

#if defined(ARCH_C2D)
	u32 ret = 0;
	u32 low,high;

	/* Unlike the C2D, the AMD Archs do not
	 * have a way of indicating ovf status or
	 * control. And hence just return success
	 * (1)
	 */

	if(unlikely(ctr >= NUM_COUNTERS))
		return -1;
	rdmsr(MSR_PERF_GLOBAL_STATUS,low,high);
	switch(ctr){
		#if NUM_COUNTERS > 0
		case 0:
			ret = low & CTR0_OVERFLOW_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 1
		case 1:
			ret = low & CTR1_OVERFLOW_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 2
		case 2:
			ret = low & CTR2_OVERFLOW_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 3
		case 3:
			ret = low & CTR3_OVERFLOW_MASK;
			break;
		#endif
		default:
			ret = -1;
			break;
	}
	return ret;
#	else
	return 0;
#	endif
#else
	return -1;
#endif
}
EXPORT_SYMBOL_GPL(pmu_is_interrupt);

int pmu_clear_ovf_status(int ctr)
{
	int ret = 0;
	#if NUM_COUNTERS > 0 
	#if defined(ARCH_C2D)
	u32 low,high;
	if(unlikely(ctr > NUM_COUNTERS))
		return -1;
	/* Unlike the C2D, the AMD Archs do not
	 * have a way of indicating ovf status or
	 * control. And hence just return success
	 * (0)
	 */
	rdmsr(MSR_PERF_GLOBAL_OVF_CTRL,low,high);
	switch(ctr){
		#if NUM_COUNTERS > 0
		case 0:
			low &= CTR0_OVERFLOW_CLEAR_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 1
		case 1:
			low &= CTR1_OVERFLOW_CLEAR_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 2
		case 2:
			low &= CTR2_OVERFLOW_CLEAR_MASK;
			break;
		#endif
		#if NUM_COUNTERS > 3
		case 3:
			low &= CTR3_OVERFLOW_CLEAR_MASK;
			break;
		#endif
		default:
			ret = -1;
			break;
	}
	if(likely(ret != -1)){
		wrmsr(MSR_PERF_GLOBAL_OVF_CTRL,low,high);
	}
	#else
	return 0;
	#endif
	return ret;
#endif

}
EXPORT_SYMBOL_GPL(pmu_clear_ovf_status);
