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

#include <linux/kernel_stat.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>
#include "state.h"

extern int total_online_cpus;

extern struct state_desc states[MAX_STATES];

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

int get_tasks_load(void)
{
	int total = 0;
	int i;
	for(i = 0; i < total_online_cpus; i++) {
#ifdef SEEKER_PLUGIN_PATCH
		total += get_cpu_nr_running(i);
#endif
	}
	return total > total_online_cpus ? total_online_cpus : total;
}

int get_state_tasks(int state)
{
	int i; 
	int tasks = 0;
	cpumask_t mask = states[state].cpumask;
	for_each_cpu_mask(i,mask){
#ifdef SEEKER_PLUGIN_PATCH
		tasks += get_cpu_nr_running(i);
#endif
	}
	return tasks;
}

int get_state_tasks_exself(int state)
{
	int i; 
	int tasks = 0;
	cpumask_t mask = states[state].cpumask;
	for_each_cpu_mask(i,mask){
#ifdef SEEKER_PLUGIN_PATCH
		tasks += get_cpu_nr_running(i);
#endif
	}
	if(tasks)
		tasks--;
	return tasks;
}

