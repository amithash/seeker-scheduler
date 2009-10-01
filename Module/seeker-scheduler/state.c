/******************************************************************************\
 * FILE: state.c
 * DESCRIPTION: Provides interfaces which provides views and manipulation
 * functions for performance states. This is also responsible to talk to
 * and listen to seeker-cpufreq. All the mutators do is set_freq. This takes
 * care of the rest.
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

#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <seeker.h>

#include "state.h"
#include "seeker_cpufreq.h"
#include "mutate.h"
#include "log.h"

/* Define the logger interval */
#define LOGGER_INTERVAL (HZ >> 8)

#define BOUNDS(val,l,h)		((val) < (l) 	?	\
				(l)		:	\
				((val) >= (h)	?	\
				 (h) - 1	:	\
				 (val)		)	)

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

int seeker_cpufreq_inform(int cpu, int state);
static void state_logger(struct work_struct *w);

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: value returned by num_online_cpus() */
extern int total_online_cpus;

/* main.c: contains the static layout if requested */
extern int init_layout[NR_CPUS];

/* main.c: Contains the number of cpu's static layout */
extern int init_layout_length;

/* main.c: contains the home state for mutator */
extern int base_state;

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
DEFINE_SEQLOCK(states_seq_lock);

/* Highest state */
int high_state;

/* Lowest state */
int low_state;

/* State at the mid */
int mid_state;

/* scpufreq user structure */
struct scpufreq_user seeker_scheduler_user = {
	.inform = &seeker_cpufreq_inform,
};

/* holds the jiffies of the last time data was read. */
unsigned long long current_jiffies[NR_CPUS] = { 0 };

/* Indicates that the state logger is running */
int state_logger_started = 0;

/* states logger work */
static DECLARE_DELAYED_WORK(state_logger_work, state_logger);

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
	atomic_inc(&(states[state].demand));
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
	atomic_dec(&(states[state].demand));
}

/********************************************************************************
 * hint_get - Get the current value of hint
 * @state - the state who's hint is required.
 *
 * Get the current value of hint.
 ********************************************************************************/
int hint_get(int state)
{
	return atomic_read(&(states[state].demand));
}

/********************************************************************************
 * hint_clear - Set the value of hint to 0.
 * @state - The state whose hint has to be cleared.
 *
 * Set the current value of hint to 0 for state `state`.
 ********************************************************************************/
void hint_clear(int state)
{
	atomic_set(&(states[state].demand), 0);
}

/********************************************************************************
 * log_cpu_state - log the state of cpu. 
 * @cpu - the cpu for which state has to be logged.
 *
 * log with debug the states transition.
 ********************************************************************************/
void log_cpu_state(int cpu)
{
	struct log_block *p = NULL;
	if (jiffies == current_jiffies[cpu])
		return;

	p = get_log();
	if (p) {
		p->entry.type = LOG_STATE;
		p->entry.u.state.cpu = cpu;
		p->entry.u.state.state = cur_cpu_state[cpu];
		p->entry.u.state.residency_time =
		    (jiffies - current_jiffies[cpu]) * 1000;
		p->entry.u.state.residency_time =
		    p->entry.u.state.residency_time / HZ;
	}
	current_jiffies[cpu] = jiffies;
	put_log(p);
}

/********************************************************************************
 * seeker_cpufreq_inform - The inform callback function for seeker_cpufreq,
 * @cpu - The cpu for which frequency has changed.
 * @state - the new state of cpu `cpu`.
 * @Side Effects - states, and cur_cpu_state data structures are changed
 *                 to reflect this change.
 *
 * This allows seeker_cpufreq to inform us if someone other than us
 * changed the frequency of a particular cpu. 
 ********************************************************************************/
int seeker_cpufreq_inform(int cpu, int state)
{
	debug("State of cpu %d changed to %d", cpu, state);
	if (cur_cpu_state[cpu] != state) {
		write_seqlock(&states_seq_lock);
		cpu_set(cpu, states[state].cpumask);
		cpu_clear(cpu, states[cur_cpu_state[cpu]].cpumask);
		states[state].cpus++;
		states[cur_cpu_state[cpu]].cpus--;
		cur_cpu_state[cpu] = state;
		write_sequnlock(&states_seq_lock);
	}

	log_cpu_state(cpu);

	cur_cpu_state[cpu] = state;

	return 0;
}

/********************************************************************************
 * state_logger - Logs the state of all cpus. started as a delayed work.
 * @w - the work struct responsible, not used.
 *
 * Log states of all cpus. and schedule self for later.
 ********************************************************************************/
void state_logger(struct work_struct *w)
{
	int i;

	for (i = 0; i < total_online_cpus; i++) {
		log_cpu_state(i);
	}

	if (state_logger_started) {
		schedule_delayed_work(&state_logger_work, LOGGER_INTERVAL);
	}

}

/********************************************************************************
 * start_state_logger - Start the state logger task.
 *
 * Start the state logger task. 
 ********************************************************************************/
void start_state_logger(void)
{
	if (state_logger_started)
		return;
	init_timer_deferrable(&state_logger_work.timer);
	schedule_delayed_work(&state_logger_work, LOGGER_INTERVAL);
	state_logger_started = 1;
}

/********************************************************************************
 * stop_state_logger - Stop the state logger task
 *
 * Stop the state logger task. 
 ********************************************************************************/
void stop_state_logger(void)
{
	if (!state_logger_started)
		return;
	state_logger_started = 0;
	cancel_delayed_work(&state_logger_work);
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
int init_cpu_states(void)
{
	int i;

	seqlock_init(&states_seq_lock);

	for (i = 0; i < total_online_cpus; i++) {
		current_jiffies[i] = jiffies;
	}

	for (i = 0; i < total_online_cpus; i++) {
		max_state_possible[i] = get_max_states(i);
		info("Max state for cpu %d = %d", i, max_state_possible[i]);
		if (total_states < max_state_possible[i])
			total_states = max_state_possible[i];
	}
	low_state = 0;
	high_state = total_states - 1;
	mid_state = (total_states >> 1);

	base_state = BOUNDS(base_state, 0, total_states);

	info("Using base state as: %d", base_state);

	for (i = 0; i < total_states; i++) {
		states[i].state = i;
		states[i].cpus = 0;
		hint_clear(i);
		cpus_clear(states[i].cpumask);
	}

	/* init cpu states */
	if (init_layout_length == 0) {
		for (i = 0; i < total_online_cpus; i++) {
			init_layout[i] = 0;
		}
		init_layout_length = total_online_cpus;
	}

	if (init_layout_length > total_online_cpus) {
		init_layout_length = total_online_cpus;
	}

	for (i = 0; i < init_layout_length; i++) {
		init_layout[i] = BOUNDS(init_layout[i], 0, total_states);
		info("init_layout[%d]=%d", i, init_layout[i]);

		set_freq(i, init_layout[i]);
		cpu_set(i, states[init_layout[i]].cpumask);
		states[init_layout[i]].cpus++;
		cur_cpu_state[i] = init_layout[i];
	}

	for (i = init_layout_length; i < total_online_cpus; i++) {
		info("init_layout[%d]=%d", i, 0);
		set_freq(i, 0);
		cpu_set(i, states[0].cpumask);
		states[0].cpus++;
		cur_cpu_state[i] = 0;
	}

	if (register_scpufreq_user(&seeker_scheduler_user)) {
		error("Registering with seeker_cpufreq failed.");
		return -1;
	}

	return 0;
}

/********************************************************************************
 * exit_cpu_states - Clean up states section.
 *
 * Exit and cleanup. Currently the only use is to deregister itself from
 * seeker_cpufreq. 
 ********************************************************************************/
void exit_cpu_states(void)
{
	deregister_scpufreq_user(&seeker_scheduler_user);
}
