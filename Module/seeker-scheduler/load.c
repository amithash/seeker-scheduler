/******************************************************************************\
 * FILE: load.c
 * DESCRIPTION: Provides functions to determine load in terms of active time.
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

#include <linux/kernel_stat.h>
#include <linux/percpu.h>
#include <linux/jiffies.h>
#include <linux/time.h>

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* Hold the last idle time to compute the idle time from last */
struct idle_info_t {
	unsigned int prev_idle_time;
};

/* we want idle_info to be per-cpu */
static DEFINE_PER_CPU(struct idle_info_t, idle_info);

/* Easy define to access idle_info of a particular cpu. return's a pointer */
#define CPU_INFO(cpu) (&per_cpu(idle_info,(cpu)))

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: The mutator interval in seconds  */
extern int change_interval;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * init_idle_logger - Initialize each cpu's idle_time
 * @Side Effects - cpu_info.prev_idle_time is set.
 *
 * Initialize this subsystem. 
 ********************************************************************************/
void init_idle_logger(void)
{
	int i;
	int cpus = num_online_cpus();
	for (i = 0; i < cpus; i++) {
		CPU_INFO(i)->prev_idle_time =
		    kstat_cpu(i).cpustat.idle + kstat_cpu(i).cpustat.iowait;
	}
}

/********************************************************************************
 * get_cpu_load - Get load of system from last time 
 * @cpu - The cpu for which load is requested.
 * @return - Load in "load units" for cpu "cpu".
 * @Side Effects - The idle_time for cpu's is updated to the current value. 
 *
 * Return the idle time as a load (Fixed point number) from the last time 
 * the same was requested for this CPU.
 ********************************************************************************/
unsigned int get_cpu_load(int cpu)
{
	unsigned total_time;
	unsigned int cur_idle_time =
	    kstat_cpu(cpu).cpustat.idle + kstat_cpu(cpu).cpustat.iowait;
		unsigned int this_time = cur_idle_time - CPU_INFO(cpu)->prev_idle_time;
	CPU_INFO(cpu)->prev_idle_time = cur_idle_time;
	total_time = msecs_to_jiffies(change_interval);

	if (this_time >= total_time)
		return 0;

	return (8 * (total_time - this_time)) / total_time;
}
