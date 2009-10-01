/******************************************************************************\
 * FILE: mutate_common.c
 * DESCRIPTION: Common mutator functions dealing with timers and calls the
 * the appropiate functions.
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

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <seeker.h>
#include "mutate.h"
#include "nrtasks.h"

/*******************************************************************************\
 * 			Function Declarations 					*
\*******************************************************************************/

static void state_change(struct work_struct *w);

void ondemand(void);
void conservative(void);
void greedy_delta(int dt);
void mem_dynamic_prog(int dt);

/********************************************************************************
 * 			Global Data structures 					*
 ********************************************************************************/

/* The mutataor's work struct */
static DECLARE_DELAYED_WORK(state_work, state_change);

/* Mutator interval time in jiffies */
static u64 interval_jiffies;

/* Timer started flag */
static int timer_started = 0;

/* Proc info for each cpu */
struct proc_info info[NR_CPUS];

/* Mutator interval */
u64 interval_count;

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

extern int mutation_method;

extern int delta;

extern int change_interval;

extern int total_online_cpus;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * state_change - work callback 
 * @w - the work struct calling this function.
 * @return - None
 * @Side Effect - Calls mutator which does its thing, and schedules itself. 
 *
 * The work routine responsible to call the mutator
 *******************************************************************************/
static void state_change(struct work_struct *w)
{
	debug("State change now @ %ld", jiffies);
	switch (mutation_method) {
	case ONDEMAND_MUTATOR:
		ondemand();
		break;
	case CONSERVATIVE_MUTATOR:
		conservative();
		break;
	case GREEDY_DELTA_MUTATOR:
		greedy_delta(delta);
		break;
	case MEM_DYNAMIC_PROG_MUTATOR:
		mem_dynamic_prog(delta);
		break;
	default:
		error("Unknown mutator");
		timer_started = 0;	/* do not run again */
	}
	if (timer_started) {
		schedule_delayed_work(&state_work, interval_jiffies);
	}
}

/********************************************************************************
 * init_mutator - initialize mutator local structures.
 * @Side Effects - For each cpu, sleep_time is set to 0 and is set to be awake. 
 *
 * Inits mutator structures. 
 ********************************************************************************/
void init_mutator(void)
{
	int i;

	if (mutation_method == STATIC_MUTATOR) {
		/* copy states */
		/* or calls init_states */
		return;
	}

	for (i = 0; i < NR_CPUS; i++) {
		info[i].sleep_time = 0;
		info[i].awake_time = 1;
		info[i].awake = 1;
	}
	interval_jiffies = (change_interval * HZ) / 1000;
	if (interval_jiffies < 1) {
		warn("change_interval=%dms makes the interval lower"
		     "than the scheduling quanta. adjusting it to equal"
		     "to the quanta = %dms", change_interval, (1000 / HZ));
		interval_jiffies = 1;
		change_interval = 1000 / HZ;
	}

	timer_started = 1;
	init_timer_deferrable(&state_work.timer);
	schedule_delayed_work(&state_work, interval_jiffies);
	info("Started Timer");
}

/********************************************************************************
 * exit_mutator - cleans up and exits the mutator.
 * @Side Effects - the work timer is cancled.
 *
 * Clean up and exit the mutator.
 ********************************************************************************/
void exit_mutator(void)
{
	if (timer_started) {
		timer_started = 0;
		cancel_delayed_work(&state_work);
	}
}

/********************************************************************************
 * wake_up_procs - wake up total_demand processors, if asleep.
 * @req_cpus - total processors required.
 *
 * first count the number of awake processors, if greater than or equal to
 * the required (total_demand) then return.
 * Else, iteratively wake up the proc with min sleep_time 
 ********************************************************************************/
void wake_up_procs(int req_cpus)
{
	int awake_total = 0;
	unsigned int min_sleep_time;
	unsigned int wake_up_proc;
	int i, j;
	/* First count awake processors */
	for (i = 0; i < total_online_cpus; i++) {
		awake_total += info[i].awake;
	}
	if (awake_total >= req_cpus) {
		return;
	}
	for (i = 0; i < req_cpus; i++) {
		awake_total = 0;
		min_sleep_time = UINT_MAX;
		wake_up_proc = UINT_MAX;
		for (j = 0; j < total_online_cpus && awake_total < req_cpus;
		     j++) {
			if (info[j].sleep_time == 0) {
				awake_total++;
				continue;
			}
			if (info[j].sleep_time < min_sleep_time) {
				wake_up_proc = j;
				min_sleep_time = info[j].sleep_time;
			}
		}
		if (wake_up_proc < total_online_cpus) {
			awake_total++;
			info[wake_up_proc].sleep_time = 0;
			info[wake_up_proc].awake = 1;
		}
		if (awake_total >= req_cpus)
			break;
	}
}

/********************************************************************************
 * retire_procs - mark cpus not required to sleep.
 * @req_cpus - cpus required.
 * @put_to_sleep - (Out) Array of cpus which will be marked 1 if the cpu has 
 * 		   to sleep.
 * @cpu_awake_proxy - Array of real cpu order. 
 ********************************************************************************/
void retire_procs(int req_cpus, int *put_to_sleep, int *cpu_awake_proxy)
{
	int awake_total = 0;
	int i, j;
	/* First count awake processors */
	for (i = 0; i < total_online_cpus; i++) {
		awake_total += info[i].awake;
	}
	if (awake_total <= req_cpus) {
		return;
	}
	for (i = 0; i < total_online_cpus; i++) {
		/* Choose an awake processor with no tasks on it */
		if (info[i].awake == 1 && get_cpu_tasks(i) == 0) {
			put_to_sleep[i] = 1;
			info[i].awake = 0;
			info[i].sleep_time = 1;
			info[i].awake_time = 0;
			awake_total--;
		} else {
			put_to_sleep[i] = 0;
		}
		/* Stop when all required procs are asleep */
		if (awake_total <= req_cpus) {
			break;
		}
	}
	/* Now create a proxy of awake processors 
	 * creating an illusion that 0-x are awake */
	for (i = 0, j = 0; i < total_online_cpus; i++) {
		if (info[i].awake == 1) {
			cpu_awake_proxy[j] = i;
			j++;
		}
	}
}
