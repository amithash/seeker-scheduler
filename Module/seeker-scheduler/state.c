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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>

#include <seeker.h>

#include "state.h"
#include "seeker_cpufreq.h"
#include "assigncpu.h"
#include "mutate.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: value returned by num_online_cpus() */
extern int total_online_cpus;

/* main.c: contains the static layout if requested */
extern int static_layout[NR_CPUS];

/* main.c: Contains the number of cpu's static layout */
extern int static_layout_length;

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* total states of each cpu */
int max_state_possible[NR_CPUS] = { 0 };

/* Total states in the system (Lowest common denominator of all CPU's */
unsigned int total_states = 0;

/* Current state of cpus */
int cur_cpu_state[NR_CPUS] = { 0 };

/* description of each state */
struct state_desc states[MAX_STATES];

/* Seq lock has to be held whenever states are used */
seqlock_t states_seq_lock = SEQLOCK_UNLOCKED;

/* Highest state */
int high_state;

/* Lowest state */
int low_state;

/* State at the mid */
int mid_state;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * hint_inc - increment hint 
 * @state - state who's hint has to be incremented 
 * @Side Effects - None
 *
 * Increment the demand for state "state" 
 ********************************************************************************/
void hint_inc(int state)
{
	states[state].demand++;
}

/********************************************************************************
 * hint_dec - decrement hint
 * @state - state who's hint has to be decremented
 * @Side Effects - None
 *
 * Decrements the demand for state "state"
 ********************************************************************************/
void hint_dec(int state)
{
	states[state].demand--;
}

/********************************************************************************
 * init_cpu_states - initialize the states sub system. 
 * @how - mentions how the states must be initialized.
 * @Side Effects - Initializes states and deems them consistent. 
 * 		   Initializes low,high&mid _state 
 * 		   Initializes the cpus with states decided by the value of "how"
 *
 * Initializes all the required entities of the subsystem.
 ********************************************************************************/
int init_cpu_states(unsigned int how)
{
	int i;
	
	write_seqlock(&states_seq_lock);

	for (i = 0; i < total_online_cpus; i++) {
		max_state_possible[i] = get_max_states(i);
		info("Max state for cpu %d = %d", i, max_state_possible[i]);
		if (total_states < max_state_possible[i])
			total_states = max_state_possible[i];
	}
	low_state = 0;
	high_state = total_states - 1;
	mid_state = (total_states >> 1);

	for (i = 0; i < total_states; i++) {
		states[i].state = i;
		states[i].cpus = 0;
		states[i].demand = 0;
		cpus_clear(states[i].cpumask);
	}

	switch (how) {
	case ALL_HIGH:
		states[total_states - 1].cpus = total_online_cpus;
		for (i = 0; i < total_online_cpus; i++) {
			cpu_set(i, states[total_states - 1].cpumask);
			cur_cpu_state[i] = max_state_possible[i] - 1;
			set_freq(i, cur_cpu_state[i]);
		}
		break;
	case ALL_LOW:
		states[0].cpus = total_online_cpus;
		for (i = 0; i < total_online_cpus; i++) {
			cpu_set(i, states[0].cpumask);
			cur_cpu_state[i] = 0;
			set_freq(i, cur_cpu_state[i]);
		}
		break;
	case BALANCE:
		states[total_states - 1].cpus = total_online_cpus >> 1;
		states[0].cpus = total_online_cpus - (total_online_cpus >> 1);
		for (i = 0; i < states[total_states - 1].cpus; i++) {
			cpu_set(i, states[0].cpumask);
			cur_cpu_state[i] = 0;
			set_freq(i, cur_cpu_state[i]);
		}
		for (; i < total_online_cpus; i++) {
			cpu_set(i, states[total_states - 1].cpumask);
			cur_cpu_state[i] = max_state_possible[i] - 1;
			set_freq(i, cur_cpu_state[i]);
		}
		break;
	case STATIC_LAYOUT:
		for(i = 0; i < static_layout_length && i < total_online_cpus; i++) {
			if(static_layout[i] < 0)
				static_layout[i] = 0;
			if(static_layout[i] >= total_states)
				static_layout[i] = total_states-1;
			set_freq(i,static_layout[i]);
			cur_cpu_state[i] = static_layout[i];
			cpu_set(i, states[static_layout[i]].cpumask);
			states[static_layout[i]].cpus++;
		}
		for(i = static_layout_length; i < total_online_cpus; i++) {
			set_freq(i,0);
			cur_cpu_state[i] = 0;
			cpu_set(i,states[0].cpumask);
			states[0].cpus++;
		}
		break;
	case NO_CHANGE:
	default:
		for (i = 0; i < total_online_cpus; i++) {
			unsigned int this_freq = get_freq(i);
			if (this_freq >= total_states) {
				set_freq(i, 0);
				this_freq = 0;
				warn("Freq state for cpu %d was not initialized and hence set to 0", i);
			}
			cur_cpu_state[i] = this_freq;
			cpu_set(i, states[this_freq].cpumask);
			states[this_freq].cpus++;
		}
		break;
	}

	write_sequnlock(&states_seq_lock);

	return 0;
}

