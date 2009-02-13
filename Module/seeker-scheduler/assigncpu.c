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
#define IPC_HIGH IPC_1_000

/* The LOW IPC Threshold */
#define IPC_LOW  IPC_0_750

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

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

#define higher(state) ((state) < total_states-1 ? (state)+1 : total_states-1)
#define lower(state) ((state) > 0 ? (state)-1 : 0)

int get_lower_state(int state)
{
	int new_state;
	int i;
	new_state = lower(state);
	for(i=new_state;i>=0;i--){
		if(states[i].cpus > 0)
			return i;
	}
	if(states[state].cpus > 0)
		return state;
	for (i = state + 1; i < total_states; i++){
		if (states[i].cpus > 0)
			return i;
	}
	return -1;
}


int get_higher_state(int state)
{
	int new_state;
	int i;
	new_state = higher(state);
	for(i=new_state;i<total_states;i++){
		if(states[i].cpus > 0)
			return i;
	}
	if(states[state].cpus > 0)
		return state;
	for(i=state-1;i>=0;i--){
		if(states[i].cpus > 0)
			return i;
	}
	return -1;
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
	unsigned int seq;
	u64 tasks_interval = 0;
	cpumask_t mask = CPU_MASK_NONE;

	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */

	if (TS_MEMBER(ts,inst) < INST_THRESHOLD)
		return;
	tasks_interval = TS_MEMBER(ts,interval);

	this_cpu = smp_processor_id();

	do{
		seq = read_seqbegin(&states_seq_lock);
		switch (TS_MEMBER(ts,fixed_state)) {
		case 0:
			state_req = state = low_state;
			break;
		case 1:
			state_req = state = mid_state;
			break;
		case 2:
			state_req = state = high_state;
			break;
		default:
			ipc = IPC(TS_MEMBER(ts,inst), TS_MEMBER(ts,re_cy));
			state = cur_cpu_state[this_cpu];
			/*up */
			if (ipc >= IPC_HIGH) {
				new_state = get_higher_state(state);
				state_req = higher(state);
			} else if (ipc <= IPC_LOW) {
				new_state = get_lower_state(state);
				state_req = lower(state);
			} else {
				state_req = state;
				if(states[state].cpus > 0){
					new_state = state;
				} else {
					int new_state1 = get_higher_state(state);
					int new_state2 = get_lower_state(state);
					new_state = abs(state-new_state1) < abs(state-new_state2) ? new_state1 : new_state1;		
				}
			}
		}
		mask = states[new_state].cpumask;
	} while(read_seqretry(&states_seq_lock,seq));

	hint_inc(state_req);

	if(cpus_empty(mask))
		return;
	set_cpus_allowed_ptr(ts,&mask);

	/* Push statastics to the debug buffer if enabled */
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_SCH;
		p->entry.u.sch.interval = TS_MEMBER(ts,interval);
		p->entry.u.sch.inst = TS_MEMBER(ts,inst);
		p->entry.u.sch.ipc = ipc;
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.state_req = state_req;
		p->entry.u.sch.state_given = new_state;
		p->entry.u.sch.cpu = this_cpu;
	}
	put_debug(p);
#ifdef SEEKER_PLUGIN_PATCH

	/* Start over. Forget the IPC... */
	TS_MEMBER(ts,interval) = interval_count;
	TS_MEMBER(ts,inst) = 0;
	TS_MEMBER(ts,ref_cy) = 0;
	TS_MEMBER(ts,re_cy) = 0;
	TS_MEMBER(ts,cpustate) = new_state;
#endif
}


