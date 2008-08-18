
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <seeker-headers.h>

#include "seeker.h"
#include "sample.h"
#include "alloc.h"
#include "log.h"
#include "exit.h"


extern int log_events[MAX_COUNTERS_PER_CPU];
extern unsigned int log_ev_masks[MAX_COUNTERS_PER_CPU];
extern int log_num_events;
extern int sample_freq;
extern int os_flag;
extern int pmu_intr;

int cpu_counters[NR_CPUS][MAX_COUNTERS_PER_CPU];
pid_t cpu_pid[NR_CPUS] = {-1};


/*---------------------------------------------------------------------------*
 * Function: clear_counters
 * Descreption: Clears all counters which can be cleared.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void clear_counters(void)
{
	int i;
	int cpu = smp_processor_id();

	/* clear the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		counter_clear(cpu_counters[cpu][i]);
	}
	for(i = 0; i < NUM_FIXED_COUNTERS; i++) {
		fcounter_clear(i);
	}

	/* OTHER COUNTERS SHOULD BE CLEARED HERE (If it makes sense that is) */
}

/*---------------------------------------------------------------------------*
 * Function: do_sample
 * Descreption: The mother load! This is the function which takes samples.
 * 		Called everytime an ISR goes off (Timer/PMU/schedule)
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void do_sample(void) 
{
	int i,j;
	struct log_block *pentry;
	unsigned long long now, period;
	unsigned long long last_ts;
	int cpu = get_cpu();

	read_time_stamp();  
	counter_read();
	fcounter_read();
	if(unlikely(read_temp() == -1)){
		debug("Temperature value not valid, retrying");
		read_temp();
	}

	/* OTHER COUNTER READ CALLS ARE DONE HERE */


	last_ts = get_last_time_stamp(cpu);
	now = get_time_stamp(cpu);
	period = now - last_ts;
	/* create an unlinked block */
	pentry = log_create();
	/* If allocation failed, log and getout! */
	if(!pentry){
		warn("Allocation failed!!! Either you are closing your "
		     "Buffers or something bad is happening");
		goto out;
	}

	pentry->sample.type = SEEKER_SAMPLE;
	pentry->sample.u.seeker_sample.cpu = cpu;
	pentry->sample.u.seeker_sample.pid = cpu_pid[cpu];
	pentry->sample.u.seeker_sample.cycles = period;
	/* log the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		pentry->sample.u.seeker_sample.counters[i] = get_counter_data(cpu_counters[cpu][i], cpu);
		
	}
	/* log the fixed counters */
	for(j=0;j<NUM_FIXED_COUNTERS;j++){
		pentry->sample.u.seeker_sample.counters[i++] = get_fcounter_data(j,cpu);
	}

	pentry->sample.u.seeker_sample.counters[i++] = get_temp(cpu);
	
	/* GETTING DATA FROM OTHER COUNTERS GO HERE */

	log_link(pentry);
  
	out:
	clear_counters();
	put_cpu();
}


/*---------------------------------------------------------------------------*
 * Function: config_counters
 * Descreption: configure all counters on the current cpu. 
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int config_counters(void)
{
	int i;
	int cpu = get_cpu();

	/* enable and configure the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		cpu_counters[cpu][i] =
		counter_enable(log_events[i], log_ev_masks[i], os_flag);
		if(unlikely(cpu_counters[cpu][i] < 0)) {
			error("Could not allocate counter for event %d",log_events[i]);
			return -1;
		}
		printk("%d: Allocated counter %d for %d:%x\n", cpu, cpu_counters[cpu][i],
								log_events[i], log_ev_masks[i]);
	}

	fcounters_enable(os_flag);
	printk("%d: enabled the fixed counters\n",cpu);

	/* ENABLING AND CONFIGURATION OF COUNTERS ARE DONE HERE. 
	 * NOTE: This is different from initialization. This is where they are enabled 
	 * If such a facility is provided by the hardware */

	clear_counters();
	put_cpu();

	return 0;
}



/*---------------------------------------------------------------------------*
 * Function: msrs_init
 * Descreption: Initialize msrs if any for each counter used, on all online cpus
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int msrs_init(void)
{
	/* initialize the pmu counters */
	if(log_num_events > 0){
		if(unlikely(on_each_cpu((void*)pmu_init_msrs,NULL, 1,1) < 0)){
			error("could not initialize counters !");
			return -1;
  		}                   
	}
	/* initialize the fixed pmu counters */
	if(unlikely(on_each_cpu((void*)fpmu_init_msrs,NULL, 1,1) < 0)){
		error("could not initialize fixed counters!");
		return -1;
  	}                   
	/* initialize the temperature sensors */
	if(unlikely(on_each_cpu((void*)therm_init_msrs,NULL, 1,1) < 0)){
		error("could not initialize the temperature sensors!");
		return -1;
  	}
	/* initialize the time stamp counter */
	if(unlikely(on_each_cpu((void*)tsc_init_msrs,NULL, 1,1) < 0)){
		error("could not initialize the time stamp counter!");
		return -1;
  	}

	/* OTHER COUNTER's MSRS CAN BE INITIALIZED HERE. */

	// setup the counters modifications -- needed only for counters with information from seeker 
	// that is currently only for the variable pmu counters.
	if(unlikely(on_each_cpu((void*)config_counters,NULL, 1,1) < 0)){
		error("could not configure counters!");
		return -1;
  	}

	return 0;
}


/*---------------------------------------------------------------------------*
 * Function: do_pid_log
 * Descreption: Called whenever release_thread/sched_exit is executed. Logs the pids.
 * Input Parameters: exiting tasks structure.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void do_pid_log(struct task_struct *p) 
{
	struct log_block *pentry = log_create();
	if(unlikely(!pentry)){
		warn("Allocation failed!!! Either you are closing your "
		      "Buffers or something bad is happening");
		return;
	}
	pentry->sample.type = PIDTAB_ENTRY;
	//pentry->sample.u.pidtab_entry.pid = (u32)(p->pid);
	pentry->sample.u.pidtab_entry.pid = (u32)0;
	pentry->sample.u.pidtab_entry.total_cycles = 0;
	memcpy(&(pentry->sample.u.pidtab_entry.name),
	       p->comm, 
	       sizeof(pentry->sample.u.pidtab_entry.name));
	log_link(pentry);
}

