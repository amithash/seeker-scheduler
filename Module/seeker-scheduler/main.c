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
#include <callback.h>

#include "scpufreq.h"
#include "hint.h"
#include "state.h"
#include "assigncpu.h"
#include "quanta.h"
#include "debug.h"

extern int hint[MAX_STATES];
void inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);

struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *)inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};

int change_interval = 5; /* In seconds */
int delta=1;
int init = ALL_HIGH;

// resched_task(struct task_struct *p)
// static void enqueue_task(struct rq *rq, struct task_struct *p, int wakeup)
// static void activate_task(struct rq *rq, struct task_struct *p, int wakeup)
//
// returns if the task is currently executing.
// inline int task_curr(const struct task_struct *p)
//
// returns the weight of load on cpu
// unsigned long weighted_cpuload(const int cpu)
//
// make task p to run on new_cpu
// void set_task_cpu(struct task_struct *p, unsigned int new_cpu)
//
// start task
// void sched_fork(struct task_struct *p, int clone_flags)
//
// wake up newly created task
// void fastcall wake_up_new_task(struct task_struct *p, unsigned long clone_flags)
//
// struct task_struct *p
// p->cpus_allowed = mask
//

void inst_sched_fork(struct task_struct *new, int clone_flags){
#ifdef SEEKER_PLUGIN_PATCH
	new->interval = interval_count;
	new->inst = 0;
	new->ref_cy = 0;
	new->re_cy = 0;
#endif
	jprobe_return();
}

void inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	put_mask_from_stats(from);
}

static int __init scheduler_init(void)
{
#ifdef SEEKER_PLUGIN_PATCH
	int probe_ret;

	init_system();
	seeker_set_callback(&inst___switch_to);
	debug_init();
	if(unlikely((probe_ret = register_jprobe(&jp_sched_fork)))){
		error("Could not find sched_fork to probe, returned %d",probe_ret);
		return -ENOSYS;
	}
	init_cpu_states(init);
	create_timer();
	return 0;
#else
	error("You are trying to use this module without patching the kernel with schedmod. Refer to the seeker/Patches/README for details");
	return -ENOTSUPP;
#endif
}

static void __exit scheduler_exit(void)
{
	debug_exit();
	seeker_clear_callback();
	unregister_jprobe(&jp_sched_fork);
	destroy_timer();
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

