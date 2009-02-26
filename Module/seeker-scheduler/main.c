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
#include <linux/kprobes.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <seeker.h>

#include <pmu.h>
#include <fpmu.h>

#include "seeker_cpufreq.h"
#include "state.h"
#include "stats.h"
#include "assigncpu.h"
#include "mutate.h"
#include "debug.h"
#include "hwcounters.h"
#include "tsc_intf.h"

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
static void state_change(struct work_struct *w);

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* The mutataor's work struct */
static DECLARE_DELAYED_WORK(state_work, state_change);

/* Mutator interval time in jiffies */
static u64 interval_jiffies;

/* Timer started flag */
static int timer_started = 0;

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

/* Contains the value of num_online_cpus(), updated by init */
int total_online_cpus = 0;

#ifdef DEBUG
/* counts the total number of times schedule was called */
unsigned int total_schedules = 0;

/* Counts the total number of times a negative newstates was encountered */
unsigned int negative_newstates = 0;

/* Counts the total number of times mask was empty. */
unsigned int mask_empty_cond = 0;

/* Counts the total number of times events/x was tried to be scheduled */
unsigned int num_events = 0;
#endif

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* mutate.c: Current interval */
extern u64 interval_count;

/* state.c: Current state of cpu's */
extern int cur_cpu_state[MAX_STATES];

/* hwcounters.c: current value of counters */
extern u64 pmu_val[NR_CPUS][3];

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

/* Mutator interval in seconds */
int change_interval = 5;

/* flag requesting disabling the scheduler and mutator */
int disable_scheduling = 0;

/* DELTA of the mutator */
int delta = 1;

/* method of initializing the states */
int init = ALL_LOW;

/* Static Layout of states */
int static_layout[NR_CPUS];

/* Count of elements in static_layout */
int static_layout_length = 0;

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
	debug("total schedules in this interval: %d", total_schedules);
	debug("total negative states warning: %d", negative_newstates);
	debug("Times mask was empty: %d", mask_empty_cond);
	debug("Total Events skipped %d", num_events);
#ifdef DEBUG
	mask_empty_cond = 0;
	total_schedules = 0;
	negative_newstates = 0;
	num_events = 0;
#endif

	debug("State change now @ %ld", jiffies);
	choose_layout(delta);
	if (timer_started) {
		schedule_delayed_work(&state_work, interval_jiffies);
	}
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
	struct debug_block *p = NULL;
	if (TS_MEMBER(t, seeker_scheduled) != SEEKER_MAGIC_NUMBER)
		jprobe_return();
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_PID;
		p->entry.u.tpid.pid = (u32) (t->pid);
		memcpy(&(p->entry.u.tpid.name[0]), t->comm, 16);
	}
	put_debug(p);
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
		set_tsk_need_resched(ts[cpu]);	/* lazy, as we are anyway getting into schedule */
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
	TS_MEMBER(new, seeker_scheduled) = SEEKER_MAGIC_NUMBER;
	TS_MEMBER(new, fixed_state) = -1;
	TS_MEMBER(new, interval) = interval_count;
	TS_MEMBER(new, inst) = 0;
	TS_MEMBER(new, ref_cy) = get_tsc_cycles();
	TS_MEMBER(new, ref_cy) = 0;
	TS_MEMBER(new, re_cy) = 0;
	TS_MEMBER(new, cpustate) = cur_cpu_state[smp_processor_id()];
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

	if (TS_MEMBER(from, seeker_scheduled) != SEEKER_MAGIC_NUMBER) {
		goto get_out;
	}

	read_counters(cpu);
	TS_MEMBER(from, inst) += pmu_val[cpu][0];
	TS_MEMBER(from, re_cy) += pmu_val[cpu][1];
	TS_MEMBER(from, ref_cy) += get_tsc_cycles();
	clear_counters(cpu);

	if (strncmp(from->comm, "events", sizeof(char) * 6) == 0) {
#ifdef DEBUG
		num_events++;
#endif
		goto get_out;
	}

	put_mask_from_stats(from);
get_out:
	jprobe_return();
}

/*******************************************************************************
 * scheduler_init - Module init function
 * @return - 0 if success, else a negative error code.
 *
 * Initializes the different elements, registers all the probes, starts the 
 * mutator work. 
 *******************************************************************************/
static int scheduler_init(void)
{
#ifdef SEEKER_PLUGIN_PATCH
	int probe_ret;
	if (static_layout_length != 0) {
		init = STATIC_LAYOUT;
	}
	if (disable_scheduling != 0) {
		disable_scheduling = 1;
	}

	total_online_cpus = num_online_cpus();
	init_idle_logger();

	if (init_tsc_intf()) {
		error("Could not init tsc_intf");
		return -ENOSYS;
	}

	if (configure_counters() != 0) {
		error("Configuring counters failed");
		return -ENOSYS;
	}
	info("Configuring counters was successful");

	/* Please keep this BEFORE the probe registeration and
	 * the timer initialization. init_cpu_states makes this 
	 * assumption by not taking any locks */
	init_cpu_states(init);
	if (!(init == STATIC_LAYOUT || disable_scheduling))
		init_mutator();

	if (debug_init() != 0)
		return -ENODEV;

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


	if (init != STATIC_LAYOUT) {
		interval_jiffies = change_interval * HZ;
		timer_started = 1;
		init_timer_deferrable(&state_work.timer);
		schedule_delayed_work(&state_work, interval_jiffies);
		info("Started Timer");
	}

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
	return -ENOSYS;

#else
#warning "This module will NOT work on this unpatched, unblessed kernel"
	error("You are trying to use this module without patching "
	      "the kernel with schedmod. Refer to the "
	      "seeker/Patches/README for details");

	return -ENOTSUPP;
#endif
}

/*******************************************************************************
 * scheduler_exit - Module exit functionn
 *
 * Unregisters all probes, stops the mutator work if it has started.
 * Stops and kicks anyone using the debug interface. 
 *******************************************************************************/
static void scheduler_exit(void)
{
	debug("removing the state change timer");
	exit_cpu_states();
	if (timer_started) {
		timer_started = 0;
		cancel_delayed_work(&state_work);
	}
	debug("Unregistering probes");
	unregister_jprobe(&jp_scheduler_tick);
	unregister_jprobe(&jp_sched_fork);
	unregister_kprobe(&kp_schedule);
	unregister_jprobe(&jp___switch_to);
	unregister_jprobe(&jp_release_thread);
	debug("Debug exiting");
	debug_exit();
	debug("Exiting the counters");
	exit_counters();
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(scheduler_init);
module_exit(scheduler_exit);

module_param(change_interval, int, 0444);
MODULE_PARM_DESC(change_interval,
		 "Interval in seconds to try and change the global state (Default 5 seconds)");

module_param(init, int, 0444);
MODULE_PARM_DESC(init,
		 "Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");

module_param(disable_scheduling, int, 0444);
MODULE_PARM_DESC(disable_scheduling,
		 "Set to not allow scheduling. Does a dry run. Also enables static layout.");

module_param_array(static_layout, int, &static_layout_length, 0444);
MODULE_PARM_DESC(static_layout, "Use to set a static_layout to use");

module_param(delta, int, 0444);
MODULE_PARM_DESC(delta, "Type of state machine to use 1,2,.. default:1");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("instruments scheduling functions to do extra work");
