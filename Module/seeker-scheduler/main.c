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

#include "scpufreq.h"
#include "state.h"
#include "assigncpu.h"
#include "quanta.h"
#include "debug.h"

void inst_inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);

struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *)inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};
struct jprobe jp_inst___switch_to = {
	.entry = (kprobe_opcode_t *)inst_inst___switch_to,
	.kp.symbol_name = "inst___switch_to",
};
int total_online_cpus = 0;

int change_interval = 5; /* In seconds */
int delta=1;
int init = ALL_HIGH;
extern u64 interval_count;
extern int cur_cpu_state[MAX_STATES];

void inst_sched_fork(struct task_struct *new, int clone_flags)
{
	warn("%s loading",new->comm);
#ifdef SEEKER_PLUGIN_PATCH
	new->interval = interval_count;
	new->inst = 0;
	new->ref_cy = 0;
	new->re_cy = 0;
	new->cpustate = cur_cpu_state[smp_processor_id()];
#endif
	jprobe_return();
}

void inst_inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	put_mask_from_stats(from);
	jprobe_return();
}

static int __init scheduler_init(void)
{
#ifdef SEEKER_PLUGIN_PATCH
	int probe_ret;
	total_online_cpus = num_online_cpus();

	init_cpu_states(init);
	debug_init();
	if(unlikely((probe_ret = register_jprobe(&jp_sched_fork)))){
		error("Could not find sched_fork to probe, returned %d",probe_ret);
		return -ENOSYS;
	}
	if(unlikely(probe_ret = register_jprobe(&jp_inst___switch_to))){
		error("Could not find sched_fork to probe, returned %d",probe_ret);
		return -ENOSYS;
	}

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
	unregister_jprobe(&jp_sched_fork);
	unregister_jprobe(&jp_inst___switch_to);
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

