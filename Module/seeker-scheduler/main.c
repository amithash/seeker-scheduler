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
#include <linux/interrupt.h>

#include <seeker.h>

#include <pmu.h>
#include <fpmu.h>

#include "scpufreq.h"
#include "state.h"
#include "assigncpu.h"
#include "mutate.h"
#include "debug.h"
#include "hwcounters.h"

#define MAX_INSTRUCTIONS_BEFORE_SCHEDULE 10000000


void inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);
int inst_schedule(struct kprobe *p, struct pt_regs *regs);

static struct timer_list state_change_timer;
static u64 interval_jiffies;

struct task_struct *ts[NR_CPUS] = {NULL};
int using_seeker = 1;

struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *)inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};
struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};
struct jprobe jp_inst___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "inst___switch_to",
};

struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};

int total_online_cpus = 0;

int change_interval = 5;
int delta=1;
int init = ALL_HIGH;
extern u64 interval_count;
extern int cur_cpu_state[MAX_STATES];
extern u64 pmu_val[NR_CPUS][3];

static void state_change(unsigned long param)
{
	choose_layout(delta);
	mod_timer(&state_change_timer, jiffies + interval_jiffies);
}


int inst_schedule(struct kprobe *p, struct pt_regs *regs)
{
	int cpu = get_cpu();
	if(!ts[cpu])
		goto inst_schedule_out;

	read_counters(cpu);
#ifdef SEEKER_PLUGIN_PATCH
	if(ts[cpu]->interval != interval_count)
		ts[cpu]->interval = interval_count;
	ts[cpu]->inst   += pmu_val[cpu][0];
	ts[cpu]->re_cy  += pmu_val[cpu][1];
	ts[cpu]->ref_cy += pmu_val[cpu][2];
	clear_counters(cpu);
	if(ts[cpu]->inst > INST_THRESHOLD)
		set_tsk_need_resched(ts[cpu]);
#endif
inst_schedule_out:
	put_cpu();
	return 0;
}

void inst_sched_fork(struct task_struct *new, int clone_flags)
{
#ifdef SEEKER_PLUGIN_PATCH
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
	if(!using_seeker){
		read_counters(cpu);
	#ifdef SEEKER_PLUGIN_PATCH
		if(from->interval != interval_count)
			from->interval = interval_count;
		from->inst   += pmu_val[cpu][0];
		from->re_cy  += pmu_val[cpu][1];
		from->ref_cy += pmu_val[cpu][2];
	#endif
		clear_counters(cpu);
	}
	ts[cpu] = to;
	put_mask_from_stats(from);
	jprobe_return();
}

static int scheduler_init(void)
{
#ifdef SEEKER_PLUGIN_PATCH
	int probe_ret;
	total_online_cpus = num_online_cpus();

	init_cpu_states(init);
	if(debug_init() != 0)
		return -ENODEV;

	if(unlikely((probe_ret = register_jprobe(&jp_sched_fork)))){
		error("Could not find sched_fork to probe, returned %d",probe_ret);
		return -ENOSYS;
	}
	if((probe_ret = register_jprobe(&jp___switch_to)) < 0){
		/* Seeker is loaded. probe its instrumentation functions instead */
		using_seeker = 1;
		if(unlikely((probe_ret = register_jprobe(&jp_inst___switch_to)) < 0)){
			error("Register inst___switch_to probe failed with %d",probe_ret);
			return -ENOSYS;
		}
	} else {
		if(unlikely((probe_ret = register_kprobe(&kp_schedule))<0)){
			error("schedule register successful, but schedule failed");
			return -ENOSYS;
		}
		configure_counters();
	}

	interval_jiffies = change_interval * HZ;

	return 0;
	setup_timer(&state_change_timer,state_change,0);
	mod_timer(&state_change_timer,jiffies+interval_jiffies);
#else
	error("You are trying to use this module without patching "
		"the kernel with schedmod. Refer to the "
		"seeker/Patches/README for details");

	return -ENOTSUPP;
#endif
}

static void scheduler_exit(void)
{
	debug_exit();
	unregister_jprobe(&jp_sched_fork);
	if(using_seeker){
		unregister_jprobe(&jp_inst___switch_to);
	} else {
		unregister_jprobe(&jp___switch_to);
		unregister_kprobe(&kp_schedule);
	}
	del_timer_sync(&state_change_timer);
}

module_init(scheduler_init);
module_exit(scheduler_exit);

module_param(change_interval,int,0444);
MODULE_PARM_DESC(change_interval, "Interval in seconds to try and change the global state (Default 5 seconds)");

module_param(init,int,0444);
MODULE_PARM_DESC(init,"Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");

module_param(delta,int,0444);
MODULE_PARM_DESC(delta,"Type of state machine to use 1,2,.. default:1");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("instruments scheduling functions to do extra work");

