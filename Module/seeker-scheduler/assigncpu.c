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
#include <asm/irq.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>

#include <seeker.h>

#include "assigncpu.h"
#include "sched_debug.h"
#include "state.h"
#include "seeker_cpufreq.h"
#include "debug.h"
#include "tsc_intf.h"
#include "nrtasks.h"
#include "migrate.h"

#ifdef ADAPTIVE_LADDER_SCHEDULING
#if defined(SEEKER_PLUGIN_PATCH) && SEEKER_PLUGIN_PATCH < 2
#warning "An older version of schedmod patch is used. use the ver2 variant."
#endif
#endif

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* state.c: description of each state */
extern struct state_desc states[MAX_STATES];

/* state.c: total states supproted on system */
extern int total_states;

/* mutate.c: current mutater interval */
extern u64 interval_count;

/* state.c: current state of cpus */
extern int cur_cpu_state[NR_CPUS];

/* main.c: flag indicating a static layout */
extern int static_layout;

/* state.c: the lowest cpu state */
extern int low_state;

/* state.c: The highest cpu state */
extern int high_state;

/* state.c: state right at the middle */
extern int mid_state;

/* main.c: flag requesting scheduler be disabled */
extern int disable_scheduling;

/* states.c: states seq_lock */
extern seqlock_t states_seq_lock;

/* main.c: mask of all allowed/online cpus */
extern cpumask_t total_online_mask;

/* main.c: base_state provided by the user */
extern int base_state;


/********************************************************************************
 * 				Local Macros					*
 ********************************************************************************/

/* macro to perform saturating increment with an exclusive limit */
#define sat_inc(state,ex_limit) ((state) < ex_limit-1 ? (state)+1 : ex_limit-1)

/* macro to performa a saturating decrement with an inclusive limit */
#define sat_dec(state,inc_limit) ((state) > inc_limit ? (state)-1 : inc_limit)

/* macro to do a sat sum on integers (including negative nums)
 * with exclusive high and inclusive low
 */
#define sat_sum(a,b,inc_low, ex_high) (((a) + (b)) >= (ex_high) ? (ex_high)-1 : (((a)+(b)) < (inc_low) ? (inc_low) : ((a)+(b))))


/* IPC of 8 Corrospondents to IPC of 1.0. */
#define IPC(inst,cy) div(((inst)<<3),(cy))
#define IPC_0_000 0
#define IPC_0_125 1
#define IPC_0_250 2
#define IPC_0_375 3
#define IPC_0_500 4
#define IPC_0_625 5
#define IPC_0_750 6
#define IPC_0_875 7
#define IPC_1_000 8

/* The HIGH IPC Threshold */
#define IPC_HIGH IPC_0_750

/* The LOW IPC Threshold */
#define IPC_LOW  IPC_0_500

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * state_free - Is state free?
 * @state - State to check.
 * @Return - 1 if state is free, 0 otherwise.
 *
 * Returns 1 if state is free to execute on. 
 ********************************************************************************/
inline int state_free(int old, int state)
{
	if(old == state && states[state].cpus > 0 && 
		get_state_tasks_exself(state) < states[state].cpus)
		return 1;
	if(states[state].cpus > 0 && get_state_tasks(state) < states[state].cpus)
		return 1;

	return 0;
}

/********************************************************************************
 * lowest_loaded_state - Returns the state which is least loaded.
 * @Return - The state which is loaded the least.
 *
 * Search for a state which is the least loaded. 
 ********************************************************************************/
int lowest_loaded_state(void)
{
	int min_load = 100000;
	int min_state = 0;
	int min_found = 0;
	int this_load;
	int i;

	for(i=0;i<total_states;i++){
		if(states[i].cpus == 0)
			continue;

		this_load = get_state_tasks(i) - states[i].cpus;
		if(min_found == 0){
			min_load = this_load;
			min_state = i;
			min_found = 1;
			continue;
		}
		if(this_load < min_load){
			min_load = this_load;
			min_state = i;
		} 
	}
	if(min_found == 0)
		return -1;

	return min_state;
}
		

/********************************************************************************
 * get_lower_state - Get a state lower than current state.
 * @state - The current state
 * @Return - An avaliable state preferrably lower than state
 * @Side Effects - None
 *
 * Returns the closest avaliable state lower than 'state'. If none are found, 
 * return 'state' or a higher state closest to 'state' whatever is avaliable
 ********************************************************************************/
inline int get_lower_state(int state)
{
	int new_state;
	int i;
	new_state = sat_dec(state, 0);
	for (i = new_state; i >= 0; i--) {
		if (state_free(state,i))
			return i;
	}
	if (state_free(state,state))
		return state;
	for (i = state + 1; i < total_states; i++) {
		if (state_free(state,i))
			return i;
	}
	return lowest_loaded_state();
}

/********************************************************************************
 * get_higher_state - Get a state higher than current state.
 * @state - The current state
 * @Return - An avaliable state preferrably higher than state
 * @Side Effects - None
 *
 * Returns the closest avaliable state higher than 'state'. If none are found, 
 * return 'state' or a lower state closest to 'state' whatever is avaliable.
 ********************************************************************************/
inline int get_higher_state(int state)
{
	int new_state;
	int i;
	new_state = sat_inc(state, total_states);
	for (i = new_state; i < total_states; i++) {
		if (state_free(state,i))
			return i;
	}
	if (state_free(state,state))
		return state;
	for (i = state - 1; i >= 0; i--) {
		if (state_free(state,i))
			return i;
	}
	return lowest_loaded_state();
}

/********************************************************************************
 * get_closest_state - Get a state closest to the current state.
 * @state - The current state
 * @Return - An avaliable state closest to 'state'
 * @Side Effects - None
 *
 * Returns 'state' if avaliable, or a state closest to 'state'
 ********************************************************************************/
inline int get_closest_state(int state)
{
	int state1, state2;
	int ret_state;
	if (state_free(state,state))
		return state;
	state1 = get_lower_state(state);
	state2 = get_higher_state(state);
	if(state1 == -1 && state2 == -1)
		return lowest_loaded_state();
	if(state1 == -1)
		return state2;
	if(state2 == -1)
		return state1;
	ret_state = ABS(state - state1) < ABS(state - state2) ? state1 : state2;

	return ret_state;
}

/********************************************************************************
 * put_mask_from_stats - The fate of "ts" shall be decided!
 * @ts - The task 
 * @Side Effect - ts-> is evaluated if it has executed enough instructions
 *
 * Evaluates IPC and decides what state it needs and appropiately assigns
 * the cpus_allowed element. 
 ********************************************************************************/
void put_mask_from_stats(struct task_struct *ts)
{
	int new_state = -1;
	struct debug_block *p = NULL;
	short ipc = 0;
	int state = 0;
	int this_cpu;
	int state_req = 0;
	unsigned seq;
	int old_state;
	u64 tasks_interval = 0;
	cpumask_t mask = CPU_MASK_NONE;

	#ifdef SCHED_DEBUG
	int i;
	#endif

	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */
	this_cpu = smp_processor_id();

	if (TS_MEMBER(ts, inst) < INST_THRESHOLD)
		return;
	tasks_interval = TS_MEMBER(ts, interval);
	old_state = TS_MEMBER(ts,cpustate);

	#ifdef SCHED_DEBUG
	for(i=0;i<total_states;i++){
		if(states[i].cpus > 0){
			assigncpu_debug("%d:%d",i,get_state_tasks(i));
		}
	}
	#endif

	do {
		seq = read_seqbegin(&states_seq_lock);

		state = cur_cpu_state[this_cpu];

		switch (TS_MEMBER(ts, fixed_state)) {
		case 0:
			new_state = get_closest_state(low_state);
			state_req = low_state;
			break;
		case 1:
			new_state = get_closest_state(mid_state);
			state_req = mid_state;
			break;
		case 2:
			new_state = get_closest_state(high_state);
			state_req = high_state;
			break;
		default:
			ipc = IPC(TS_MEMBER(ts, inst), TS_MEMBER(ts, re_cy));
#if SCHEDULER_TYPE == LADDER_SCHEDULING
			/*up */
			if (ipc >= IPC_HIGH) {
				new_state = get_higher_state(state);
				state_req = sat_inc(state, total_states);
			} else if (ipc <= IPC_LOW) {
				new_state = get_lower_state(state);
				state_req = sat_dec(state, 0);
			} else {
				state_req = state;
				new_state = get_closest_state(state);
			}
#elif SCHEDULER_TYPE == ADAPTIVE_LADDER_SCHEDULING
#warning "Using adaptive ladder scheduling"
			if (ipc >= IPC_HIGH) {
				new_state = get_higher_state(state);
				if(TS_MEMBER(ts,hist_step) >= 0){
					state_req = sat_sum(state, TS_MEMBER(ts, hist_step) ,0, total_states);
					TS_MEMBER(ts,hist_step) = sat_inc(TS_MEMBER(ts,hist_step),total_states);
				} else {
					state_req = state;
					TS_MEMBER(ts,hist_step) = 0;
				}

			} else if (ipc <= IPC_LOW) {
				new_state = get_lower_state(state);
				if(TS_MEMBER(ts, hist_step) <= 0){
					state_req = sat_sum(state, TS_MEMBER(ts, hist_step), 0, total_states);
					TS_MEMBER(ts,hist_step) = sat_dec(TS_MEMBER(ts,hist_step), (-1*(int)total_states));
				} else {
					state_req = state;
					TS_MEMBER(ts, hist_step) = 0;
				}
			} else {
				state_req = state;
				new_state = get_closest_state(state);
				TS_MEMBER(ts, hist_step) = 0;
			}
#elif SCHEDULER_TYPE == SELECT_SCHEDULING

#endif
		}
		if (new_state >= 0 && new_state < total_states)
			mask = states[new_state].cpumask;
		#if defined(DEBUG) && DEBUG == 2
		else{
			assigncpu_debug("Negative state for %s",ts->comm);
		}
		#endif

	} while (read_seqretry(&states_seq_lock, seq));

	hint_inc(state_req);


	/* Push statastics to the debug buffer if enabled */
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_SCH;
		p->entry.u.sch.interval = TS_MEMBER(ts, interval);
		p->entry.u.sch.inst = TS_MEMBER(ts, inst);
		p->entry.u.sch.ipc = ipc;
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.state_req = state_req;
		p->entry.u.sch.state_given = new_state;
		p->entry.u.sch.cpu = this_cpu;
		p->entry.u.sch.state = state;
		p->entry.u.sch.cycles = TS_MEMBER(ts, ref_cy);
	}
	put_debug(p);

	/* Start over. Forget the IPC... */
	TS_MEMBER(ts, interval) = interval_count;
	TS_MEMBER(ts, inst) = 0;
	TS_MEMBER(ts, ref_cy) = 0;
	TS_MEMBER(ts, re_cy) = 0;
	TS_MEMBER(ts, cpustate) = new_state;

	/* What the duche? as stewie says it */
	if (cpus_empty(mask)) {
		assigncpu_debug("Empty mask for %s",ts->comm);
		return;
	}

	/* Assign only if we have not disabled scheduling 
	 * NOTE: Of course, we do not need to execute this
	 * function, but this is done to have the same 
	 * overhead
	 */
	if(disable_scheduling == 0){
		put_work(ts,mask);
	}
}

/********************************************************************************
 * initial_mask - initialize mask for the starting task.
 * @ts - the task which is about to start.
 *
 * Initialize the task which is about to start.
 ********************************************************************************/
void initial_mask(struct task_struct *ts)
{
	int state = base_state;
	cpumask_t mask = CPU_MASK_NONE;
	unsigned seq;
	do {
		seq = read_seqbegin(&states_seq_lock);
		state = get_closest_state(base_state);
		if (state >= 0 && state < total_states)
			mask = states[state].cpumask;
	} while (read_seqretry(&states_seq_lock, seq));
#ifdef HIST_BASED_SCHEDULING
	TS_MEMBER(ts, hist_step) = 0;
#endif
	if(cpus_empty(mask)){
		mask = total_online_mask;
		TS_MEMBER(ts, cpustate) = cur_cpu_state[smp_processor_id()];
	} else {
		TS_MEMBER(ts, cpustate) = state;
	}
	put_work(ts,mask);

}


