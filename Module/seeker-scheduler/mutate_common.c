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
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <seeker.h>
#include "mutate.h"

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

static void state_change(struct work_struct *w);

void ondemand(void);
void conservative(void);
void greedy_delta(int dt);
void mem_dynamic_prog(int dt);

/********************************************************************************
 * 			Global Datastructures 					*
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
	switch(mutation_method){
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
			timer_started = 0; /* do not run again */
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

	if(mutation_method == STATIC_MUTATOR){
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
	if(interval_jiffies < 1){
		warn("change_interval=%dms makes the interval lower"
			"than the scheduling quanta. adjusting it to equal"
			"to the quanta = %dms",change_interval,(1000/HZ));
		interval_jiffies = 1;
		change_interval = 1000 / HZ;
	}

	timer_started = 1;
	init_timer_deferrable(&state_work.timer);
	schedule_delayed_work(&state_work, interval_jiffies);
	info("Started Timer");
}

void exit_mutator(void)
{
	if (timer_started) {
		timer_started = 0;
		cancel_delayed_work(&state_work);
	}
}


