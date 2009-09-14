/******************************************************************************\
 * FILE: assigncpu.c
 * DESCRIPTION: 
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

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>

#include <seeker.h>

#include "assigncpu.h"
#include "sched_debug.h"
#include "state.h"
#include "seeker_cpufreq.h"
#include "log.h"
#include "tsc_intf.h"
#include "nrtasks.h"
#include "migrate.h"
#include "search_state.h"
#include "pds.h"
#include "ipc.h"

#define FIXED_STATE_LOW		0
#define FIXED_STATE_MID		1
#define FIXED_STATE_HIGH	2

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
 * 				Functions					*
 ********************************************************************************/


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
	struct log_block *p = NULL;
	short ipc = 0;
	int state = 0;
	int this_cpu;
	int state_req = 0;
	unsigned seq;
	int old_state;
	u64 tasks_interval = 0;
	cpumask_t mask = CPU_MASK_NONE;

	#if defined(SCHED_DEBUG)
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
		case FIXED_STATE_LOW:
			new_state = search_state_closest(state,low_state);
			state_req = low_state;
			break;
		case FIXED_STATE_MID:
			new_state = search_state_closest(state,mid_state);
			state_req = mid_state;
			break;
		case FIXED_STATE_HIGH:
			new_state = search_state_closest(state,high_state);
			state_req = high_state;
			break;
		default:
			ipc = IPC(TS_MEMBER(ts, inst), TS_MEMBER(ts, re_cy));
      new_state = evaluate_ipc(ipc,state,&(TS_MEMBER(ts,hist_step)),&state_req);
		}
		if (new_state >= 0 && new_state < total_states)
			mask = states[new_state].cpumask;
		#if defined(SCHED_DEBUG) && DEBUG == 2
		else{
			assigncpu_debug("Negative state for %s",ts->comm);
		}
		#endif

	} while (read_seqretry(&states_seq_lock, seq));

	hint_inc(state_req);


	/* Push statastics to the debug buffer if enabled */
	p = get_log();
	if (p) {
		p->entry.type = LOG_SCH;
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
	put_log(p);

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
  int cpu = smp_processor_id();
	cpumask_t mask = CPU_MASK_NONE;
	unsigned seq;
	do {
		seq = read_seqbegin(&states_seq_lock);
		state = search_state_closest(cur_cpu_state[cpu],base_state);
		if (state >= 0 && state < total_states)
			mask = states[state].cpumask;
	} while (read_seqretry(&states_seq_lock, seq));
	TS_MEMBER(ts, hist_step) = 0;
	if(cpus_empty(mask)){
		mask = total_online_mask;
		TS_MEMBER(ts, cpustate) = cur_cpu_state[cpu];
	} else {
		TS_MEMBER(ts, cpustate) = state;
	}
	put_work(ts,mask);

}


