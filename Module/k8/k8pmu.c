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

#include "k8pmu.h"

MODULE_LICENSE("GPL");
//#define KBUILD_BASENAME "coscheduler"
//#define KBUILD_MODNAME "k8pmu"

evtsel_t evtsel[NR_CPUS][NUM_EVTSEL] = {
	{
		{0,0,0,0,0,0,0,0,0,0,0x00,PERF_EVTSEL0}, /*0*/
		{0,0,0,0,0,0,0,0,0,0,0x00,PERF_EVTSEL1}, /*1*/
		{0,0,0,0,0,0,0,0,0,0,0x00,PERF_EVTSEL2}, /*2*/
		{0,0,0,0,0,0,0,0,0,0,0x00,PERF_EVTSEL3}  /*3*/
	}
};

counter_t counters[NR_CPUS][NUM_COUNTERS] = {
	{
		{0, 0, PERF_CTR0, 0, 0, 0}, /* 0 */
		{0, 0, PERF_CTR1, 1, 0, 0}, /* 1 */
		{0, 0, PERF_CTR2, 2, 0, 0}, /* 2 */
		{0, 0, PERF_CTR3, 3, 0, 0} /* 3 */
	}
};

char *evtsel_names[NUM_EVTSEL] = {
  "MSR_K7_EVNTSEL0",
  "MSR_K7_EVNTSEL1",
  "MSR_K7_EVNTSEL2",
  "MSR_K7_EVNTSEL3",
};

char *counter_names[NUM_COUNTERS] = {
  "MSR_K7_PERFCTR0", 
  "MSR_K7_PERFCTR1",
  "MSR_K7_PERFCTR2",
  "MSR_K7_PERFCTR3",
};

//read the evtsel reg and return the low 32 bits
//the high are reserved anyway
inline u32 evtsel_read(u32 evtsel_num){
	u32 low, high;
	int cpu_id = smp_processor_id();
	rdmsr(evtsel[cpu_id][evtsel_num].addr, low, high);
	return low;
}
EXPORT_SYMBOL_GPL(evtsel_read);


//clears the event select registers
inline void evtsel_clear(u32 evtsel_num){
	u32 low, high;
	int cpu_id = smp_processor_id();
	rdmsr(evtsel[cpu_id][evtsel_num].addr, low, high);
	low &= EVTSEL_RESERVED_BITS;
	wrmsr(evtsel[cpu_id][evtsel_num].addr, low, high);
}
EXPORT_SYMBOL_GPL(evtsel_clear);

//write to the respective evtsel register
inline void evtsel_write(u32 evtsel_num)
{
	u32 low, high;
	int cpu = smp_processor_id();
	evtsel_t *cur_evtsel = &(evtsel[cpu][evtsel_num]);

	rdmsr(cur_evtsel->addr, low, high);
	low &= EVTSEL_RESERVED_BITS;

	low = (cur_evtsel->ev_select << 0)
		| (cur_evtsel->ev_mask << 8)
		| (cur_evtsel->usr_flag << 16)
		| (cur_evtsel->os_flag << 17)
		| (cur_evtsel->edge << 18)
		| (cur_evtsel->pc_flag << 19)
		| (cur_evtsel->int_flag << 20)
		| (cur_evtsel->enabled << 22)
		| (cur_evtsel->inv_flag << 23)
		| (cur_evtsel->cnt_mask << 24);

	wrmsr(cur_evtsel->addr, low, high);
}
EXPORT_SYMBOL_GPL(evtsel_write);

//must be called using on_each_cpu
inline void counter_clear(u32 counter){
	int cpu_id;
	cpu_id = smp_processor_id();
	if(cpu_id < NR_CPUS){
		wrmsr(counters[cpu_id][counter].addr, 0, 0);
		wrmsr(counters[cpu_id][counter].addr, 0, 0);
	}
}
EXPORT_SYMBOL_GPL(counter_clear);

//must be called using on_each_cpu
void counter_read(void){
	u32 low, high;
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if(cpu_id < NR_CPUS){
		/* this is the "full" read of the full 48bits */
		for(i=0;i<NUM_COUNTERS;i++){
			rdmsr(counters[cpu_id][i].addr, low, high);
			counters[cpu_id][i].high = high;
      			counters[cpu_id][i].low = low;
		}
	}
}
EXPORT_SYMBOL_GPL(counter_read);

//use this to get the counter data
u64 get_counter_data(u32 counter, u32 cpu_id){
	u64 counter_val = (u64)counters[cpu_id][counter].low;
	counter_val = counter_val | ((u64)counters[cpu_id][counter].high << 32);
	return counter_val;
}
EXPORT_SYMBOL_GPL(get_counter_data);

//must be called using on_each_cpu
inline void counter_disable(int counter) {
	int cpu_id = smp_processor_id(); 
	if(cpu_id < NR_CPUS){
		evtsel_clear(counter);
		counter_clear(counter);
		evtsel[cpu_id][counter].enabled = 0;
		evtsel_write(counter);
	}
}
EXPORT_SYMBOL_GPL(counter_disable);

//must be called using on_each_cpu
int counter_enable(u32 event_num, u32 ev_mask, u32 os){
	u32 i;
	int counter_num = -1;
	int cpu_id = smp_processor_id();
	if(cpu_id < NR_CPUS){  
		for(i=0;i<NUM_COUNTERS;i++){
			if(counters[cpu_id][i].enabled == 0){
				counter_num = i;
				break;
			}
		}
		if(counter_num >=0){
			evtsel_clear(counter_num);
			counter_clear(counter_num);
			counters[cpu_id][counter_num].enabled = 1;
			counters[cpu_id][counter_num].event_num = event_num;

			//counfigure the event sel reg
			evtsel[cpu_id][counter_num].ev_select = event_num;
			evtsel[cpu_id][counter_num].ev_mask = ev_mask;
			evtsel[cpu_id][counter_num].usr_flag = 1;
			evtsel[cpu_id][counter_num].os_flag = os;
			evtsel[cpu_id][counter_num].pc_flag = 1;
			evtsel[cpu_id][counter_num].int_flag = 0;
			evtsel[cpu_id][counter_num].inv_flag = 0;
			evtsel[cpu_id][counter_num].cnt_mask = 0;
			evtsel[cpu_id][counter_num].enabled = 1;
			evtsel[cpu_id][counter_num].edge = 0;	
			evtsel_write(counter_num);
			return counter_num;
		} 
		else {
			return -1;
		} 
	}
	else{
		return -1;
	}
}
EXPORT_SYMBOL_GPL(counter_enable);

//must be called from on_each_cpu
inline void pmu_init_msrs(void){
	int i;
	int cpu = smp_processor_id();
	if(cpu != 0){
		for(i=0;i<NUM_COUNTERS;i++){
			counters[cpu][i] = counters[0][i];
			evtsel[cpu][i] = evtsel[0][i];
		}
	}
  
	for(i = 0; i < NUM_EVTSEL; i++) {
		evtsel_clear(i);
	}

	for(i = 0; i < NUM_COUNTERS; i++) {
		counter_disable(i);
		counter_clear(i);
	}
}
EXPORT_SYMBOL_GPL(pmu_init_msrs);

//must be called from on_each_cpu
static int pmu_init(void){
	pmu_init_msrs();
	return 0;
}

static void pmu_exit(void){
	int i,j;
	for(i=0;i<NR_CPUS;i++){
		for(j=0;j<NUM_COUNTERS;j++){
			counters[i][j].enabled = 0;
		}
	}
	;
}

module_init(pmu_init);
module_exit(pmu_exit);

