
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the " 
		   "fixed performance monitoring counters");

fixctrl_t fcontrol[NR_CPUS] = {
	{0,0,0,0,0,0,0,0,0}
};

fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{
		#if NUM_FIXED_COUNTERS > 0
		{0, 0, MSR_PERF_FIXED_CTR0} /* 0 */
		#endif
		#if NUM_FIXED_COUNTERS > 1
		,{0, 0, MSR_PERF_FIXED_CTR1} /* 1 */
		#endif
		#if NUM_FIXED_COUNTERS > 2
		,{0, 0, MSR_PERF_FIXED_CTR2}  /* 2 */
		#endif
	}
};

char *fcounter_names[NUM_FIXED_COUNTERS] = {
	#if NUM_FIXED_COUNTERS > 0
	"FIXED_CTR0"
	#endif
	#if NUM_FIXED_COUNTERS > 1
	,"FIXED_CTR1"
	#endif
	#if NUM_FIXED_COUNTERS > 2
	,"FIXED_CTR2"
	#endif
};

fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{
		#if NUM_FIXED_COUNTERS > 0
		{0,0,0} 
		#endif
		#if NUM_FIXED_COUNTERS > 1
		,{0,0,0}
		#endif
		#if NUM_FIXED_COUNTERS > 2
		,{0,0,0}
		#endif
	}
};

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

// Read the fixed counter control register.
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


//clears the fixed counter control registers.
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

//write to the fixed counter control registers.
inline void control_write(void)
{
	#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		fixctrl_t *cur_control = &(fcontrol[cpu_id]);
	
		rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
		low &= FIXSEL_RESERVED_BITS;
	
		low = 	  (cur_control->os0 << 0)
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

//must be called using on_each_cpu
inline void fcounter_clear(u32 counter)
{
	#if NUM_FIXED_COUNTERS > 0
	int cpu_id;
	cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		wrmsr(fcounters[cpu_id][counter].addr, fcleared[cpu_id][counter].low, fcleared[cpu_id][counter].high);
		wrmsr(fcounters[cpu_id][counter].addr, fcleared[cpu_id][counter].low, fcleared[cpu_id][counter].high);
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounter_clear);

//must be called using on_each_cpu
void fcounter_read(void)
{
	#if NUM_FIXED_COUNTERS > 0
	u32 low, high;
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		/* this is the "full" read of the full 64bits */
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			rdmsr(fcounters[cpu_id][i].addr, low, high);
			fcounters[cpu_id][i].high = high;
			fcounters[cpu_id][i].low = low;
		}
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounter_read);

//use this to get the counter data
u64 get_fcounter_data(u32 counter, u32 cpu_id)
{
	#if NUM_FIXED_COUNTERS > 0
	u64 counter_val;
	if(likely(counter < NUM_FIXED_COUNTERS && cpu_id < NR_CPUS)){
		counter_val = (u64)fcounters[cpu_id][counter].low;
		counter_val = counter_val | ((u64)fcounters[cpu_id][counter].high << 32);
		return counter_val - fcleared[cpu_id][counter].all;
	}
	else{
		return -1;
	}
	#else
	return 0;
	#endif
}
EXPORT_SYMBOL_GPL(get_fcounter_data);

//must be called using on_each_cpu
inline void fcounters_disable(void)
{
	#if NUM_FIXED_COUNTERS > 0
	int i;
	int cpu_id = smp_processor_id(); 
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			fcounter_clear(i);
		}
		control_clear();			
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounters_disable);

//must be called using on_each_cpu
void fcounters_enable(u32 os)
{
	#if NUM_FIXED_COUNTERS > 0
	u32 i;
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
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

//must be called from on_each_cpu
void fpmu_init_msrs(void)
{
	#if NUM_FIXED_COUNTERS > 0
	int i;
	int cpu_id = get_cpu();
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			if(cpu_id != 0){
				fcounters[cpu_id][i] = fcounters[0][i];
				fcleared[cpu_id][i] = fcleared[0][i];
			}
			fcounter_clear(i);
		}
		if(cpu_id != 0){
			fcontrol[cpu_id] = fcontrol[0];
		}
		control_clear();
		fcounters_disable();
	}
	put_cpu();
	#endif
}
EXPORT_SYMBOL_GPL(fpmu_init_msrs);

//must be called from on_each_cpu
static int __init fpmu_init(void)
{
	#if NUM_FIXED_COUNTERS > 0
	if(on_each_cpu((void *)fpmu_init_msrs,NULL,1,1) < 0){
		error("Could not enable anything, panicing and exiting");
		return -ENODEV;
	}
	#endif
	return 0;
}

static void __exit fpmu_exit(void)
{
	;
}

module_init(fpmu_init);
module_exit(fpmu_exit);

