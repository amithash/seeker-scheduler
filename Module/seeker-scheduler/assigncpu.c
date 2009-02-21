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

#include <seeker.h>

#include "assigncpu.h"
#include "state.h"
#include "seeker_cpufreq.h"
#include "debug.h"
#include "tsc_intf.h"

/* Works for a max of 32 processors */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

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
#define IPC_HIGH IPC_0_875

/* The LOW IPC Threshold */
#define IPC_LOW  IPC_0_625

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

/* main.c: cycles run by a task on every cpu */
extern unsigned long long task_cycles[NR_CPUS];

#ifdef DEBUG
/* main.c: Debugging count of the number of times schedules was called */
extern unsigned int total_schedules;

/* main.c: counts the error condition negative new states */
extern unsigned int negative_newstates;

/* main.c: counts the error condition empty masks */
extern unsigned int mask_empty_cond;
#endif

/********************************************************************************
 * 				Local Macros					*
 ********************************************************************************/

/* macro to perform saturating increment with an exclusive limit */
#define sat_inc(state,ex_limit) ((state) < ex_limit-1 ? (state)+1 : ex_limit-1)

/* macro to performa a saturating decrement with an inclusive limit */
#define sat_dec(state,inc_limit) ((state) > inc_limit ? (state)-1 : inc_limit)

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

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
		if (states[i].cpus > 0)
			return i;
	}
	if (states[state].cpus > 0)
		return state;
	for (i = state + 1; i < total_states; i++) {
		if (states[i].cpus > 0)
			return i;
	}
	return -1;
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
		if (states[i].cpus > 0)
			return i;
	}
	if (states[state].cpus > 0)
		return state;
	for (i = state - 1; i >= 0; i--) {
		if (states[i].cpus > 0)
			return i;
	}
	return -1;
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
	if (states[state].cpus > 0)
		return state;
	state1 = get_lower_state(state);
	state2 = get_higher_state(state);
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
	u64 tasks_interval = 0;
	cpumask_t mask = CPU_MASK_NONE;
	unsigned long long cy;

	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */

	if (TS_MEMBER(ts, inst) < INST_THRESHOLD)
		return;
	tasks_interval = TS_MEMBER(ts, interval);

	this_cpu = smp_processor_id();

	cy = task_cycles[this_cpu] + get_tsc_cycles();
	task_cycles[this_cpu] = 0;

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
		}
		if (new_state >= 0 && new_state < total_states)
			mask = states[new_state].cpumask;
#ifdef DEBUG
		else
			negative_newstates++;
#endif

	} while (read_seqretry(&states_seq_lock, seq));

	hint_inc(state_req);

	/* What the duche? as stewie says it */
	if (cpus_empty(mask)) {
#ifdef DEBUG
		mask_empty_cond++;
#endif
		return;
	}

	/* Assign only if we have not disabled scheduling */
	if(!disable_scheduling)
		ts->cpus_allowed = mask;

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
		p->entry.u.sch.cycles = cy;
	}
	put_debug(p);

	/* Start over. Forget the IPC... */
	TS_MEMBER(ts, interval) = interval_count;
	TS_MEMBER(ts, inst) = 0;
	TS_MEMBER(ts, ref_cy) = 0;
	TS_MEMBER(ts, re_cy) = 0;
	TS_MEMBER(ts, cpustate) = new_state;

#ifdef DEBUG
	total_schedules++;
#endif
}
