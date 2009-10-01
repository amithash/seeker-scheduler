/******************************************************************************\
 * FILE: probe.c
 * DESCRIPTION: Functions to probe important functions in the kernel relating
 * to scheduling to insert the performance directed scheduling functionality 
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 * Copyright 2006 Tipp Mosely                                                  *
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

#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>

#include <seeker.h>

#include <pmu.h>

#include "pds.h"
#include "log.h"
#include "hwcounters.h"
#include "tsc_intf.h"
#include "sched_debug.h"
#include "migrate.h"
#include "state.h"

/* seeker's magic short */
#define SEEKER_MAGIC_NUMBER 0xdea

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

void inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);
int inst_schedule(struct kprobe *p, struct pt_regs *regs);
void inst_release_thread(struct task_struct *t);
void inst_scheduler_tick(void);

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* Per-cpu task_structs updated by __switch_to */
struct task_struct *ts[NR_CPUS] = { NULL };

/* Probe for sched_fork */
struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *) inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};

/* Probe for __switch_to */
struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *) inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

/* Probe for scheduler_tick */
struct jprobe jp_scheduler_tick = {
	.entry = (kprobe_opcode_t *) inst_scheduler_tick,
	.kp.symbol_name = "scheduler_tick",
};

/* Probe for schedule */
struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};

/* Probe for release thread */
struct jprobe jp_release_thread = {
	.entry = (kprobe_opcode_t *) inst_release_thread,
	.kp.symbol_name = "release_thread",
};

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* mutate.c: Current interval */
extern u64 interval_count;

/* hwcounters.c: current value of counters */
extern u64 pmu_val[NR_CPUS][3];

/* state.c: Current state of cpu's */
extern int cur_cpu_state[NR_CPUS];

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * is_blacklist_task - Should seeker manage this task?
 * p -> task struct of the task to check.
 * @return - 1 if it is a blacklist task, 0 otherwise.
 * 
 * Return 1 if this is a blacklist task. 
 *******************************************************************************/
int is_blacklist_task(struct task_struct *p)
{
	if (strcmp(p->comm, "debugd") == 0)
		return 1;
	if (strncmp(p->comm, "events/", 7) == 0)
		return 1;
	if (strncmp(p->comm, "migration/", 10) == 0)
		return 1;
	if (strncmp(p->comm, "ksoftirqd/", 10) == 0)
		return 1;
	return 0;
}

/*******************************************************************************
 * inst_scheduler_tick - probe for scheduler_tick
 * @return - None
 * @Side Effect - calls the probe of scheduler.
 *
 * This is one of the probes (along with scheduler) to deal with scheduling
 *******************************************************************************/
void inst_scheduler_tick(void)
{
	inst_schedule(NULL, NULL);
	jprobe_return();
}

/*******************************************************************************
 * inst_release_thread - probe for release_thread
 * @t - the task which is getting released (murder!)
 * @return - None
 * @Side Effect - If debug has started, then pid is logged.
 *
 * Logs the pids and command names of threads when they die
 *******************************************************************************/
void inst_release_thread(struct task_struct *t)
{
	struct log_block *p = NULL;
	if (TS_MEMBER(t, seeker_scheduled) != SEEKER_MAGIC_NUMBER)
		goto out;
	if (is_blacklist_task(t))
		goto out;
	p = get_log();
	if (p) {
		p->entry.type = LOG_PID;
		p->entry.u.tpid.pid = (u32) (t->pid);
		memcpy(&(p->entry.u.tpid.name[0]), t->comm, 16);
	}

	put_log(p);

	/* Make sure there is no pending work on this task */
	cancel_task_work(t);
out:
	jprobe_return();

}

/*******************************************************************************
 * inst_schedule - Probe for schedule
 * @p - Not used
 * @regs - Not used
 * @return - always 0
 * @Side Effects - Updates the tasks counters and requests a re-schedule 
 *                 if the total executed instructions exceed INST_THRESHOLD
 *
 * Responsible for updating reading/writing the performance counters.
 * And also the one which decides when a task has executed enough
 *******************************************************************************/
int inst_schedule(struct kprobe *p, struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	if (!ts[cpu]
	    || TS_MEMBER(ts[cpu], seeker_scheduled) != SEEKER_MAGIC_NUMBER)
		return 0;

	if (is_blacklist_task(ts[cpu]))
		return 0;

	read_counters(cpu);
	if (TS_MEMBER(ts[cpu], interval) != interval_count)
		TS_MEMBER(ts[cpu], interval) = interval_count;
	TS_MEMBER(ts[cpu], inst) += pmu_val[cpu][0];
	TS_MEMBER(ts[cpu], re_cy) += pmu_val[cpu][1];
	TS_MEMBER(ts[cpu], ref_cy) += get_tsc_cycles();
	clear_counters(cpu);
	if (TS_MEMBER(ts[cpu], inst) > INST_THRESHOLD
	    || TS_MEMBER(ts[cpu], cpustate) != cur_cpu_state[cpu]
	    || TS_MEMBER(ts[cpu], interval) != interval_count) {
		set_tsk_need_resched(ts[cpu]);	/* lazy, as we are anyway 
						   getting into schedule */
	}
	return 0;
}

/*******************************************************************************
 * inst_sched_fork - Probe for sched_fork
 * @new - The task which is going to start
 * @clone_flags - Not used.
 * @return - None
 * @Side Effects - Sets the new tasks to be managed by seeker. Inits the members
 *                 to zero
 *
 * A tasks birth. Init member elements required by seeker. Assign SEEKER_MAGIC_
 * NUMBER to seeker_scheduled to let other probes know that this is a new task
 * and hence managed by seeker. 
 *******************************************************************************/
void inst_sched_fork(struct task_struct *new, int clone_flags)
{
	TS_MEMBER(new, fixed_state) = -1;
	TS_MEMBER(new, interval) = interval_count;
	TS_MEMBER(new, inst) = 0;
	TS_MEMBER(new, ref_cy) = get_tsc_cycles();
	TS_MEMBER(new, ref_cy) = 0;
	TS_MEMBER(new, re_cy) = 0;

	if (is_blacklist_task(new) == 0) {
		TS_MEMBER(new, seeker_scheduled) = SEEKER_MAGIC_NUMBER;
		initial_mask(new);
	}

	jprobe_return();
}

/*******************************************************************************
 * inst___switch_to - Probe for __switch_to
 * @from - Previous task
 * @to   - Next chosen task.
 * @return - None
 * @Side Effect - Updates ts to "to", updates counter elements, and calls 
 *                the scheduler to decide the fate of "from".
 *
 * Decides the fate of "from" while remembering "to"
 *******************************************************************************/
void inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	int cpu = smp_processor_id();
	ts[cpu] = to;

	if (TS_MEMBER(to, seeker_scheduled) == SEEKER_MAGIC_NUMBER &&
	    is_blacklist_task(to) == 0)
		usage_inc(TS_MEMBER(to, cpustate));

	read_counters(cpu);
	TS_MEMBER(from, inst) += pmu_val[cpu][0];
	TS_MEMBER(from, re_cy) += pmu_val[cpu][1];
	TS_MEMBER(from, ref_cy) += get_tsc_cycles();
	clear_counters(cpu);

	if (TS_MEMBER(from, seeker_scheduled) != SEEKER_MAGIC_NUMBER) {
		goto get_out;
	}

	if (is_blacklist_task(from))
		goto get_out;

	usage_dec(TS_MEMBER(from, cpustate));

	if (is_blacklist_task(from) == 0)
		put_mask_from_stats(from);
get_out:
	jprobe_return();
}

int insert_probes(void)
{
	int probe_ret;

	/* One of the good uses of goto! For each of the registering, 
	 * if they fail, we still need to de-register anything done
	 * in the past and by taken cared by ordered goto's
	 */
	if (unlikely((probe_ret = register_jprobe(&jp_scheduler_tick)))) {
		error
		    ("Could not find scheduler_tick to probe, returned %d",
		     probe_ret);
		goto no_scheduler_tick;
	}
	info("Registered jp_scheduler_tick");

	if (unlikely((probe_ret = register_jprobe(&jp_sched_fork)))) {
		error("Could not find sched_fork to probe, returned %d",
		      probe_ret);
		goto no_sched_fork;
	}
	info("Registered jp_sched_fork");

	if (unlikely((probe_ret = register_jprobe(&jp___switch_to)))) {
		error("Could not find __switch_to to probe, returned %d",
		      probe_ret);
		goto no___switch_to;
	}
	info("Registered jp_sched_fork");

	if (unlikely((probe_ret = register_kprobe(&kp_schedule)) < 0)) {
		error("schedule register successful, but schedule failed");
		goto no_schedule;
	}
	info("Registering of kp_schedule was successful");

	if (unlikely((probe_ret = register_jprobe(&jp_release_thread)) < 0)) {
		error("schedule register successful, but schedule failed");
		goto no_release_thread;
	}
	info("Registering of kp_schedule was successful");
	return 0;

no_release_thread:
	unregister_kprobe(&kp_schedule);
no_schedule:
	unregister_jprobe(&jp___switch_to);
no___switch_to:
	unregister_jprobe(&jp_sched_fork);
no_sched_fork:
	unregister_jprobe(&jp_scheduler_tick);
no_scheduler_tick:
	return -1;

}

int remove_probes(void)
{
	debug("Unregistering probes");
	unregister_jprobe(&jp_scheduler_tick);
	unregister_jprobe(&jp_sched_fork);
	unregister_kprobe(&kp_schedule);
	unregister_jprobe(&jp___switch_to);
	unregister_jprobe(&jp_release_thread);
	debug("Debug exiting");

	return 0;
}
