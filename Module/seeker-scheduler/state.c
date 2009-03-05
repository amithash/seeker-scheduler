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
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <seeker.h>

#include "state.h"
#include "seeker_cpufreq.h"
#include "assigncpu.h"
#include "mutate.h"
#include "debug.h"

/* Define the logger interval */
#define LOGGER_INTERVAL (HZ >> 8)

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
extern int static_layout[NR_CPUS];

/* main.c: Contains the number of cpu's static layout */
extern int static_layout_length;

#ifdef DEBUG
/* assigncpu.c: the debug string */
extern char debug_string[1024];
#endif
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
unsigned long long current_jiffies[NR_CPUS] = {0};

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
 * states_copy - copy states element by element 
 * @dest - The destination states struct
 * @src - Source struct
 * @Side Effects - None
 *
 * Copy from src to dest element by element. 
 ********************************************************************************/
void states_copy(struct state_desc *dest, struct state_desc *src)
{
	dest->state = src->state;
	dest->cpumask = src->cpumask;
	dest->cpus = src->cpus;
	dest->demand = src->demand;
}

/********************************************************************************
 * log_cpu_state - log the state of cpu. 
 * @cpu - the cpu for which state has to be logged.
 *
 * log with debug the states transition.
 ********************************************************************************/
void log_cpu_state(int cpu)
{
	struct debug_block *p = NULL;
	p = get_debug();
	if(p){
		p->entry.type = DEBUG_STATE;
		p->entry.u.state.cpu = cpu;
		p->entry.u.state.state = cur_cpu_state[cpu];
		p->entry.u.state.residency_time = (jiffies - current_jiffies[cpu]) * 1000;
		p->entry.u.state.residency_time = do_div(p->entry.u.state.residency_time,HZ);
	}
	current_jiffies[cpu] = jiffies;
	put_debug(p);
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
	info("State of cpu %d changed to %d",cpu,state);
	if(cur_cpu_state[cpu] != state){
		write_seqlock(&states_seq_lock);
		cpu_set(cpu,states[state].cpumask);
		cpu_clear(cpu,states[cur_cpu_state[cpu]].cpumask);
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

	assigncpu_debug_print();

	for (i=0; i<total_online_cpus; i++){
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
	if(state_logger_started)
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
	if(!state_logger_started)
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
int init_cpu_states(unsigned int how)
{
	int i;

	seqlock_init(&states_seq_lock);

	if(register_scpufreq_user(&seeker_scheduler_user)){
		error("Registering with seeker_cpufreq failed.");
		return -1;
	}

	for(i = 0; i < total_online_cpus; i++) {
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

	for (i = 0; i < total_states; i++) {
		states[i].state = i;
		states[i].cpus = 0;
		states[i].demand = 0;
		states[i].usage = 0;
		cpus_clear(states[i].cpumask);
	}

	switch (how) {
	case ALL_HIGH:
		info("All states starting of high");
		states[total_states - 1].cpus = total_online_cpus;
		for (i = 0; i < total_online_cpus; i++) {
			cpu_set(i, states[total_states - 1].cpumask);
			set_freq(i, max_state_possible[i]-1);
		}
		break;
	case ALL_LOW:
		info("All states starting off low");
		states[0].cpus = total_online_cpus;
		for (i = 0; i < total_online_cpus; i++) {
			cpu_set(i, states[0].cpumask);
			set_freq(i,0);
		}
		break;
	case BALANCE:
		info("Half the cpus are starting of high, while the other half low");
		states[total_states - 1].cpus = total_online_cpus >> 1;
		states[0].cpus = total_online_cpus - (total_online_cpus >> 1);
		for (i = 0; i < states[total_states - 1].cpus; i++) {
			cpu_set(i, states[0].cpumask);
			set_freq(i, 0);
		}
		for (; i < total_online_cpus; i++) {
			cpu_set(i, states[total_states - 1].cpumask);
			set_freq(i, max_state_possible[i]-1);
		}
		break;
	case STATIC_LAYOUT:
		info("A statc layout was chosen");
		for (i = 0; i < static_layout_length && i < total_online_cpus;
		     i++) {
			if (static_layout[i] < 0)
				static_layout[i] = 0;
			if (static_layout[i] >= total_states)
				static_layout[i] = total_states - 1;
			set_freq(i, static_layout[i]);
			cpu_set(i, states[static_layout[i]].cpumask);
			states[static_layout[i]].cpus++;
		}
		for (i = static_layout_length; i < total_online_cpus; i++) {
			set_freq(i, 0);
			cpu_set(i, states[0].cpumask);
			states[0].cpus++;
		}
		break;
	case NO_CHANGE:
	default:
		info("No change is done. The current freq is read");
		for (i = 0; i < total_online_cpus; i++) {
			unsigned int this_freq = get_freq(i);
			if (this_freq >= total_states) {
				set_freq(i, 0);
				this_freq = 0;
				warn("Freq state for cpu %d was not initialized and hence set to 0", i);
			}
			cpu_set(i, states[this_freq].cpumask);
			states[this_freq].cpus++;
		}
		break;
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

