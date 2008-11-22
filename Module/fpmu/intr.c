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
#include <linux/percpu.h>

#include <seeker.h>

#include "fpmu.h"

extern fixctrl_t fcontrol[NR_CPUS];
extern fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS];
extern fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS];

int fpmu_configure_interrupt(int ctr, u32 low, u32 high)
{
	int ret = 0;
	int cpu = smp_processor_id();
	if(likely(ctr < NUM_FIXED_COUNTERS)){
		fcleared[cpu][ctr].low = low;
		fcleared[cpu][ctr].high = high;
		fcleared[cpu][ctr].all = (u64)low | (((u64)high) << 32);
	}
	else{
		ret =  -1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(fpmu_configure_interrupt);

int fpmu_enable_interrupt(int ctr)
{
	int ret = 0;
	int cpu_id = smp_processor_id();
	switch(ctr){
		case 0:
			fcontrol[cpu_id].pmi0 = 1;
			break;
		case 1:
			fcontrol[cpu_id].pmi1 = 1;
			break;
		case 2:
			fcontrol[cpu_id].pmi2 = 1;
			break;
		default:
			ret = -1;
			break;
	}
	if(likely(ret != -1)){
		control_write();
	}
	return ret;
}
EXPORT_SYMBOL_GPL(fpmu_enable_interrupt);

int fpmu_disable_interrupt(int ctr)
{
	int ret = 0;
#if NUM_FIXED_COUNTERS > 0
	int cpu = smp_processor_id();
	if(likely(ctr < NUM_FIXED_COUNTERS)){
		fcleared[cpu][ctr].low = 0;
		fcleared[cpu][ctr].high = 0;
		fcleared[cpu][ctr].all = 0;
	}
	else{
		ret =  -1;
	}
	switch(ctr){
		case 0:
			fcontrol[cpu].pmi0 = 0;
			break;
		case 1:
			fcontrol[cpu].pmi1 = 0;
			break;
		case 2:
			fcontrol[cpu].pmi2 = 0;
			break;
		default:
			ret = -1;
			break;
	}
	if(likely(ret != -1)){
		control_write();
	}
#endif
	return ret;
}       
EXPORT_SYMBOL_GPL(fpmu_disable_interrupt);

int fpmu_is_interrupt(int ctr)
{
	u32 ret = 0;
#if NUM_FIXED_COUNTERS > 0
	u32 low,high;
	rdmsr(MSR_PERF_GLOBAL_STATUS,low,high);
	switch(ctr){
		case 0:
			ret = high & FIXED_CTR0_OVERFLOW_MASK;
			break;
		case 1:
			ret = high & FIXED_CTR1_OVERFLOW_MASK;
			break;
		case 2:
			ret = high & FIXED_CTR2_OVERFLOW_MASK;
			break;
		default:
			ret = 0;
			break;
	}
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(fpmu_is_interrupt);


int fpmu_clear_ovf_status(int ctr)
{
	int ret = 0;
#if NUM_FIXED_COUNTERS > 0
	u32 low,high;
	rdmsr(MSR_PERF_GLOBAL_OVF_CTRL,low,high);
	switch(ctr){
		case 0:
			high &= FIXED_CTR0_OVERFLOW_CLEAR_MASK;
			break;
		case 1:
			high &= FIXED_CTR1_OVERFLOW_CLEAR_MASK;
			break;
		case 2:
			high &= FIXED_CTR2_OVERFLOW_CLEAR_MASK;
			break;
		default:
			ret = -1;
			break;
        }
	if(likely(ret != -1)){
		wrmsr(MSR_PERF_GLOBAL_OVF_CTRL,low,high);
	}
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(fpmu_clear_ovf_status);

