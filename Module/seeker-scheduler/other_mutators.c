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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "state.h"
#include "stats.h"
#include "other_mutators.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* state.c: total states for each cpu */
extern unsigned int total_states;

/* main.c: value returned by num_online_cpus() */
extern int total_online_cpus;

/* state.c: current state of each cpu */
extern int cur_cpu_state[NR_CPUS];

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* Selected states for cpus */
static int new_cpu_state[NR_CPUS];

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * ondemand - The mutator called every mutator interval.
 * @Side effects - Changes cur_cpu_state and states field during which the states
 * 		   will be inconsistent. 
 *
 * Chooses the layout based on load of processors.
 ********************************************************************************/
void ondemand(void)
{
	int load;
	int i;

	for (i = 0; i < total_online_cpus; i++) {
		load = get_cpu_load(i);
		if( load > LOAD_0_875 ) {
			new_cpu_state[i] = total_states-1;
		} else {
			new_cpu_state[i] = 0;
		}
		if(cur_cpu_state[i] != new_cpu_state[i]){
			set_freq(i,new_cpu_state[i]);
		}
	}
}

/********************************************************************************
 * conservative - The mutator called every mutator interval.
 * @Side effects - Changes cur_cpu_state and states field during which the states
 * 		   will be inconsistent. 
 *
 * Chooses the layout based on load of processors.
 ********************************************************************************/
void conservative(void)
{
	int load;
	int i;

	for (i = 0; i < total_online_cpus; i++) {
		load = get_cpu_load(i);
		if( load > LOAD_0_875 ) {
			new_cpu_state[i] = cur_cpu_state[i] == (total_states -1) ? 
				total_states-1 : 
				cur_cpu_state[i]+1;
		} else {
			new_cpu_state[i] = cur_cpu_state[i] == 0 ?
				0 :
				cur_cpu_state[i] - 1;
		}
		if(cur_cpu_state[i] != new_cpu_state[i]){
			set_freq(i,new_cpu_state[i]);
		}
	}
}

