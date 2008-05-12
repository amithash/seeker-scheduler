/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
 *                                                                        *
 * This file is part of Seeker                                            *
 *                                                                        *
 * Seeker is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>
#include <linux/percpu.h>

#include "c2dfpmu.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the fixed performance monitoring counters of the Core2Duo");

fixctrl_t fcontrol[NR_CPUS] = {
	{0,0,0,0,0,0,0,0,0}
};

fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{
		{0, 0, MSR_PERF_FIXED_CTR0}, /* 0 */
		{0, 0, MSR_PERF_FIXED_CTR1}, /* 1 */
		{0, 0, MSR_PERF_FIXED_CTR2}  /* 2 */
	}
};

char *fcounter_names[NUM_FIXED_COUNTERS] = {
	"FIXED_CTR0", 
	"FIXED_CTR1",
	"FIXED_CTR2"
};

cleared_t cleared[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{{0,0,0}, {0,0,0}, {0,0,0}}
};

// Read the fixed counter control register.
inline u32 control_read(void) {
	u32 low, high;
	rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
	return low;
}
EXPORT_SYMBOL_GPL(control_read);

int fpmu_configure_interrupt(int ctr, u32 low, u32 high){
	int ret = 0;
	int cpu = smp_processor_id();
	if(likely(ctr < NUM_FIXED_COUNTERS)){
		cleared[cpu][ctr].low = low;
		cleared[cpu][ctr].high = high;
		cleared[cpu][ctr].all = (u64)low | (((u64)high) << 32);
	}
	else{
		ret =  -1;
	}
	return -1;
}
EXPORT_SYMBOL_GPL(fpmu_configure_interrupt);

int fpmu_enable_interrupt(int ctr){
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

int fpmu_disable_interrupt(int ctr){
	int cpu = smp_processor_id();
	int ret = 0;
	if(likely(ctr < NUM_FIXED_COUNTERS)){
		cleared[cpu][ctr].low = 0;
		cleared[cpu][ctr].high = 0;
		cleared[cpu][ctr].all = 0;
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
	return ret;
}	
EXPORT_SYMBOL_GPL(fpmu_disable_interrupt);

int fpmu_is_interrupt(int ctr){
	u32 low,high;
	u32 ret = 0;
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
	return ret;
}
EXPORT_SYMBOL_GPL(fpmu_is_interrupt);

int fpmu_clear_ovf_status(int ctr){
	u32 low,high;
	int ret = 0;
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
	return ret;
}
EXPORT_SYMBOL_GPL(fpmu_clear_ovf_status);

//clears the fixed counter control registers.
inline void control_clear(void){
	u32 low, high;
	rdmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
	low &= FIXSEL_RESERVED_BITS;
	wrmsr(MSR_PERF_FIXED_CTR_CTRL, low, high);
}
EXPORT_SYMBOL_GPL(control_clear);

//write to the fixed counter control registers.
inline void control_write(void){
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
}
EXPORT_SYMBOL_GPL(control_write);

//must be called using on_each_cpu
inline void fcounter_clear(u32 counter){
	int cpu_id;
	cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		wrmsr(fcounters[cpu_id][counter].addr, cleared[cpu_id][counter].low, cleared[cpu_id][counter].high);
		wrmsr(fcounters[cpu_id][counter].addr, cleared[cpu_id][counter].low, cleared[cpu_id][counter].high);
	}
}
EXPORT_SYMBOL_GPL(fcounter_clear);

//must be called using on_each_cpu
void fcounter_read(void){
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
}
EXPORT_SYMBOL_GPL(fcounter_read);

//use this to get the counter data
u64 get_fcounter_data(u32 counter, u32 cpu_id){
	u64 counter_val;
	if(likely(counter < NUM_FIXED_COUNTERS && cpu_id < NR_CPUS)){
		counter_val = (u64)fcounters[cpu_id][counter].low;
		counter_val = counter_val | ((u64)fcounters[cpu_id][counter].high << 32);
		return counter_val - cleared[cpu_id][counter].all;
	}
	else{
		return -1;
	}
}
EXPORT_SYMBOL_GPL(get_fcounter_data);

//must be called using on_each_cpu
inline void fcounters_disable(void){
	int i;
	int cpu_id = smp_processor_id(); 
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			fcounter_clear(i);
		}
		control_clear();			
	}
}
EXPORT_SYMBOL_GPL(fcounters_disable);

//must be called using on_each_cpu
void fcounters_enable(u32 os) {
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
}
EXPORT_SYMBOL_GPL(fcounters_enable);

//must be called from on_each_cpu
inline void fpmu_init_msrs(void) {
	int i;
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			if(cpu_id != 0){
				fcounters[cpu_id][i] = fcounters[0][i];
				cleared[cpu_id][i] = cleared[0][i];
			}
			fcounter_clear(i);
		}
		if(cpu_id != 0){
			fcontrol[cpu_id] = fcontrol[0];
		}
		control_clear();
		fcounters_disable();
	}
}
EXPORT_SYMBOL_GPL(fpmu_init_msrs);

//must be called from on_each_cpu
static int __init fpmu_init(void){
	fpmu_init_msrs();
	return 0;
}

static void __exit fpmu_exit(void){
	;
}

module_init(fpmu_init);
module_exit(fpmu_exit);

