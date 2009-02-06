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

#define MAX_INSTRUCTIONS_BEFORE_SCHEDULE 10000000
#define SEEKER_MAGIC_NUMBER 0xdea


void inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);
int inst_schedule(struct kprobe *p, struct pt_regs *regs);
void inst_release_thread(struct task_struct *t);
void inst_scheduler_tick(void);

static void state_change(struct work_struct *w);
static DECLARE_DELAYED_WORK(state_work, state_change);

static u64 interval_jiffies;
static int timer_started = 0;

struct task_struct *ts[NR_CPUS] = {NULL};

struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *)inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};
struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

/* Consider using the scheduler_tick as a jprobe instead. 
 * if schedule 
 */
struct jprobe jp_scheduler_tick = {
	.entry = (kprobe_opcode_t *)inst_scheduler_tick,
	.kp.symbol_name = "scheduler_tick",
};

struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};

struct jprobe jp_release_thread = {
	.entry = (kprobe_opcode_t *)inst_release_thread,
	.kp.symbol_name = "release_thread",
};

int total_online_cpus = 0;

int change_interval = 5;
int disable_scheduling = 0;
int delta=1;
int init = ALL_LOW;
int static_layout = 0;
extern u64 interval_count;
extern int cur_cpu_state[MAX_STATES];
extern u64 pmu_val[NR_CPUS][3];
#ifdef DEBUG
unsigned int schedule_count = 0;
unsigned int scheduler_tick_count = 0;
#endif

static void state_change(struct work_struct *w)
{
	debug("scheduler_count=%u : scheduler_tick_count = %u",schedule_count,scheduler_tick_count);
#ifdef DEBUG
	schedule_count = scheduler_tick_count = 0;
#endif

	debug("State change now @ %ld",jiffies);
	choose_layout(delta);
	if(timer_started){
		schedule_delayed_work(&state_work, interval_jiffies);
	}
}

void inst_scheduler_tick(void)
{
#ifdef DEBUG
	scheduler_tick_count++;
#endif
	inst_schedule(NULL,NULL);
	jprobe_return();

}

void inst_release_thread(struct task_struct *t)
{
	struct debug_block *p = NULL;
	#ifdef SEEKER_PLUGIN_PATCH
	if(t->seeker_scheduled != SEEKER_MAGIC_NUMBER)
		jprobe_return();
	#endif
	p = get_debug();
	if(p){
		p->entry.type = DEBUG_PID;
		p->entry.u.tpid.pid = (u32)(t->pid);
		memcpy(&(p->entry.u.tpid.name[0]),t->comm,16);
	}
	put_debug(p);
	jprobe_return();
	
}

int inst_schedule(struct kprobe *p, struct pt_regs *regs)
{
	int cpu = smp_processor_id();
#ifdef DEBUG
	schedule_count++;
#endif
#ifdef SEEKER_PLUGIN_PATCH
	if(!ts[cpu] || ts[cpu]->seeker_scheduled != SEEKER_MAGIC_NUMBER)
		return 0;
#endif

	read_counters(cpu);
#ifdef SEEKER_PLUGIN_PATCH
	if(ts[cpu]->interval != interval_count)
		ts[cpu]->interval = interval_count;
	ts[cpu]->inst   += pmu_val[cpu][0];
	ts[cpu]->re_cy  += pmu_val[cpu][1];
	ts[cpu]->ref_cy += pmu_val[cpu][2];
	clear_counters(cpu);
	if(ts[cpu]->inst > INST_THRESHOLD && ts[cpu]->cpustate != cur_cpu_state[cpu]){
		set_tsk_need_resched(ts[cpu]); /* lazy, as we are anyway getting into schedule */
	}
#endif
	return 0;
}

void inst_sched_fork(struct task_struct *new, int clone_flags)
{
#ifdef SEEKER_PLUGIN_PATCH
	new->seeker_scheduled = SEEKER_MAGIC_NUMBER;
	new->fixed_state = -1;
	new->interval = interval_count;
	new->inst = 0;
	new->ref_cy = 0;
	new->re_cy = 0;
	new->cpustate = cur_cpu_state[smp_processor_id()];
#endif
	jprobe_return();
}

void inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	int cpu = smp_processor_id();
	ts[cpu] = to;
	#ifdef SEEKER_PLUGIN_PATCH
	if(from->seeker_scheduled != SEEKER_MAGIC_NUMBER){
		jprobe_return();
	}
	#endif

	read_counters(cpu);
	#ifdef SEEKER_PLUGIN_PATCH
	from->interval = interval_count;
	from->inst   += pmu_val[cpu][0];
	from->re_cy  += pmu_val[cpu][1];
	from->ref_cy += pmu_val[cpu][2];
	#endif
	clear_counters(cpu);

	put_mask_from_stats(from);
	jprobe_return();
}

static int scheduler_init(void)
{
#ifdef SEEKER_PLUGIN_PATCH
	int probe_ret;
	if(static_layout != 0){
		static_layout = 1;
		init = 4;
	}
	if(disable_scheduling != 0){
		disable_scheduling = 1;
		static_layout = 1;
		init = 4;
	}
	
	total_online_cpus = num_online_cpus();
	init_idle_logger();
	/* Please keep this BEFORE the probe registeration and
	 * the timer initialization. init_cpu_states makes this 
	 * assumption */
	init_cpu_states(init);
	if(static_layout == 0)
		init_mutator();

	if(debug_init() != 0)
		return -ENODEV;

	/* One of the good uses of goto! For each of the registering, 
	 * if they fail, we still need to de-register anything done
	 * in the past and taken cared of the ordered goto's
	 */
	if(unlikely((probe_ret = register_jprobe(&jp_scheduler_tick)))){
		error("Could not find scheduler_tick to probe, returned %d",probe_ret);
		goto no_scheduler_tick;
	}
	info("Registered jp_scheduler_tick");

	if(unlikely((probe_ret = register_jprobe(&jp_sched_fork)))){
		error("Could not find sched_fork to probe, returned %d",probe_ret);
		goto no_sched_fork;
	}
	info("Registered jp_sched_fork");

	if(unlikely((probe_ret = register_jprobe(&jp___switch_to)))){
		error("Could not find __switch_to to probe, returned %d",probe_ret);
		goto no___switch_to;
	}
	info("Registered jp_sched_fork");

	if(unlikely((probe_ret = register_kprobe(&kp_schedule))<0)){
		error("schedule register successful, but schedule failed");
		goto no_schedule;
	}
	info("Registering of kp_schedule was successful");

	if(unlikely((probe_ret = register_jprobe(&jp_release_thread))<0)){
		error("schedule register successful, but schedule failed");
		goto no_release_thread;
	}
	info("Registering of kp_schedule was successful");

	if(configure_counters() != 0){
		error("Configuring counters failed");
		goto no_counters;
	}
	info("Configuring counters was successful");

	/* static_layout = 0 implies, enabling the mutator */
	if(static_layout == 0){
		interval_jiffies = change_interval * HZ;
		timer_started = 1;
		init_timer_deferrable(&state_work.timer);
		schedule_delayed_work(&state_work, interval_jiffies);
		info("Started Timer");
	}

	return 0;

no_counters:
	unregister_jprobe(&jp_release_thread);
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
	error("You are trying to use this module without patching "
		"the kernel with schedmod. Refer to the "
		"seeker/Patches/README for details");

	return -ENOTSUPP;
#endif
}

static void scheduler_exit(void)
{
	debug("removing the state change timer");
	if(timer_started){
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
}

module_init(scheduler_init);
module_exit(scheduler_exit);

module_param(change_interval,int,0444);
MODULE_PARM_DESC(change_interval, "Interval in seconds to try and change the global state (Default 5 seconds)");

module_param(init,int,0444);
MODULE_PARM_DESC(init,"Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");

module_param(disable_scheduling,int,0444);
MODULE_PARM_DESC(disable_scheduling,"Set to not allow scheduling. Does a dry run. Also enables static layout.");

module_param(static_layout,int,0444);
MODULE_PARM_DESC(static_layout,"Use this parameter set (>0) to use a static layout defined by seeker-cpufreq");

module_param(delta,int,0444);
MODULE_PARM_DESC(delta,"Type of state machine to use 1,2,.. default:1");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("instruments scheduling functions to do extra work");

