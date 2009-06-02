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
#include <linux/cpumask.h>

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
#include "other_mutators.h"
#include "sched_debug.h"

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
int is_blacklist_task(struct task_struct *p);

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

/* Mask of all allowed cpus */
cpumask_t total_online_mask = CPU_MASK_NONE;

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* mutate.c: Current interval */
extern u64 interval_count;

/* state.c: Current state of cpu's */
extern int cur_cpu_state[NR_CPUS];

/* hwcounters.c: current value of counters */
extern u64 pmu_val[NR_CPUS][3];

extern struct state_desc states[MAX_STATES];


/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

/* Mutator interval in seconds */
int change_interval = 1000;

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

/* allowed cpus to limit total cpus used. */
int allowed_cpus = 0;

int mutation_method = 0;

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
		case 1: 
			ondemand();
			break;
		case 2: 
			conservative();
			break;
		case 0:
		default:
			choose_layout(delta);
	}
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
		goto out;
	if(is_blacklist_task(t))
		goto out;
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_PID;
		p->entry.u.tpid.pid = (u32) (t->pid);
		memcpy(&(p->entry.u.tpid.name[0]), t->comm, 16);
	}

	put_debug(p);

	/* Make sure there is no pending work on this task */
	cancel_task_work(t);
out:
	jprobe_return();

}

/*******************************************************************************
 * is_blacklist_task - Should seeker manage this task?
 * p -> task struct of the task to check.
 * @return - 1 if it is a blacklist task, 0 otherwise.
 * 
 * Return 1 if this is a blacklist task. 
 *******************************************************************************/
int is_blacklist_task(struct task_struct *p)
{
	if(strcmp(p->comm,"debugd") == 0)
		return 1;
	if(strncmp(p->comm,"events/",7) == 0)
		return 1;
	if(strncmp(p->comm,"migration/",10) == 0)
		return 1;
	if(strncmp(p->comm,"ksoftirqd/",10) == 0)
		return 1;
	return 0;
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

	if(is_blacklist_task(ts[cpu]))
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
	TS_MEMBER(new, fixed_state) = -1;
	TS_MEMBER(new, interval) = interval_count;
	TS_MEMBER(new, inst) = 0;
	TS_MEMBER(new, ref_cy) = get_tsc_cycles();
	TS_MEMBER(new, ref_cy) = 0;
	TS_MEMBER(new, re_cy) = 0;

	if(is_blacklist_task(new) == 0){
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

	if(TS_MEMBER(to, seeker_scheduled) == SEEKER_MAGIC_NUMBER &&
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

	if(is_blacklist_task(from))
		goto get_out;

	usage_dec(TS_MEMBER(from, cpustate));

	if(is_blacklist_task(from) == 0)
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
	int i;
	if (static_layout_length != 0) {
		init = STATIC_LAYOUT;
	}
	if(mutation_method != 0){
		disable_scheduling = 1;
	}
	if (disable_scheduling != 0) {
		disable_scheduling = 1;
	}

	total_online_cpus = num_online_cpus();

	if(allowed_cpus != 0){
		if(allowed_cpus > 0 && allowed_cpus <= total_online_cpus){
			total_online_cpus = allowed_cpus;
		} else {
			warn("allowed_cpus has to be within (0,%d]",total_online_cpus);
		}
	}
	cpus_clear(total_online_mask);
	for(i=0;i<total_online_cpus;i++){
		cpu_set(i,total_online_mask);
	}

	info("Total online mask = %x\n",CPUMASK_TO_UINT(total_online_mask));

	init_mig_pool();

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
	init_assigncpu_logger();

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
	stop_state_logger();
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
	exit_mig_pool();
	exit_assigncpu_logger();
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(scheduler_init);
module_exit(scheduler_exit);

module_param(change_interval, int, 0444);
MODULE_PARM_DESC(change_interval,
		 "Interval in ms to change the global state (Default: 1000)");

module_param(mutation_method, int, 0444);
MODULE_PARM_DESC(mutation_method, 
		"Type of mutation: Delta (default) - 0, ondemand - 1, conservative - 2");

module_param(init, int, 0444);
MODULE_PARM_DESC(init,
		 "Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");
module_param(allowed_cpus, int, 0444);
MODULE_PARM_DESC(allowed_cpus, "Limit cpus to this number, default is all online cpus.");

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
