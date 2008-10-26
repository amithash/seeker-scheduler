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
#include "quanta.h"
#include "debug.h"

/* Definations of the evtsel and mask 
 * for instructions retired,
 * real unhalted cycles
 * reference unhalted cycles 
 * for the AMD as it does not have fixed counters
 */
#define PMU_INST_EVTSEL 0x00000000
#define PMU_INST_MASK 0x00000000
#define PMU_RECY_EVTSEL 0x00000000
#define PMU_RECY_MASK 0x00000000
#define PMU_RFCY_EVTSEL 0x00000000
#define PMU_RFCY_MASK 0x00000000

#define MAX_INSTRUCTIONS_BEFORE_SCHEDULE 10000000


void inst___switch_to(struct task_struct *from, struct task_struct *to);
void inst_sched_fork(struct task_struct *new, int clone_flags);
int inst_schedule(struct kprobe *p, struct pt_regs *regs);

struct task_struct *ts[NR_CPUS] = {NULL};

struct jprobe jp_sched_fork = {
	.entry = (kprobe_opcode_t *)inst_sched_fork,
	.kp.symbol_name = "sched_fork",
};
struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

struct jprobe jp_schedule = {
	.entry = (kprobe_opcode_t *)inst_schedule,
	.kp.symbol_name = "inst_schedule",
};

struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};

int total_online_cpus = 0;

int change_interval = 5; /* In seconds */
int delta=1;
int init = ALL_HIGH;
extern u64 interval_count;
extern int cur_cpu_state[MAX_STATES];
int sys_counters[NR_CPUS][3];
u64 pmu_val[NR_CPUS][3];


void enable_pmu_counters(void);
int configure_counters(void);
inline void read_counters(int cpu);

void enable_pmu_counters(void)
{
	int cpu = smp_processor_id();
	if(NUM_FIXED_COUNTERS > 0){
		fpmu_init_msrs();
		fcounters_enable(0);
		fcounter_clear(0);
		fcounter_clear(1);
		fcounter_clear(2);
	} else {
		pmu_init_msrs();
		sys_counters[cpu][0] = counter_enable(PMU_INST_EVTSEL,PMU_INST_MASK,0);
		sys_counters[cpu][1] = counter_enable(PMU_RECY_EVTSEL,PMU_RECY_MASK,0);
		sys_counters[cpu][2] = counter_enable(PMU_RFCY_EVTSEL,PMU_RFCY_MASK,0);
		counter_clear(sys_counters[cpu][0]);
		counter_clear(sys_counters[cpu][1]);
		counter_clear(sys_counters[cpu][2]);
	}
}

int configure_counters(void)
{
	if(on_each_cpu((void *)enable_pmu_counters,NULL,1,1) < 0){
		error("Counters could not be configured");
		return -1;
	}
	return 0;
}

inline void read_counters(int cpu)
{
#if NUM_FIXED_COUNTERS > 0
	fcounter_read();
	pmu_val[cpu][0] = get_fcounter_data(0,cpu);
	pmu_val[cpu][1] = get_fcounter_data(1,cpu);
	pmu_val[cpu][2] = get_fcounter_data(2,cpu);
#else
	counter_read();
	pmu_val[cpu][0] = get_counter_data(sys_counters[cpu][0],cpu);
	pmu_val[cpu][1] = get_counter_data(sys_counters[cpu][0],cpu);
	pmu_val[cpu][2] = get_counter_data(sys_counters[cpu][0],cpu);
#endif
}	

int inst_schedule(struct kprobe *p, struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	read_counters(cpu);
#ifdef SEEKER_PLUGIN_PATCH
	if(ts[cpu]->interval == interval_count){
		ts[cpu]->inst += pmu_val[cpu][0];
		ts[cpu]->re_cy += pmu_val[cpu][1];
		ts[cpu]->ref_cy += pmu_val[cpu][2];
		if(ts[cpu]->inst > MAX_INSTRUCTIONS_BEFORE_SCHEDULE)
			set_tsk_need_resched(ts[cpu]);
	} else {
		ts[cpu]->interval = interval_count;
		ts[cpu]->inst = pmu_val[cpu][0];
		ts[cpu]->re_cy = pmu_val[cpu][1];
		ts[cpu]->ref_cy = pmu_val[cpu][2];
	}	
#endif
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
	ts[cpu] = to;
	warn("from %s to %s",from->comm,to->comm);
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
	if(unlikely((probe_ret = register_jprobe(&jp___switch_to))<0)){
		/* Seeker is loaded. Now probe its instrumentation functions */
		jp___switch_to.kp.symbol_name = "inst___switch_to";
		if(unlikely((probe_ret = register_jprobe(&jp___switch_to))<0)){
			error("Register __switch_to probe failed with %d",probe_ret);
			return -ENOSYS;
		}
		if(unlikely((probe_ret = register_jprobe(&jp_schedule))<0)){
			error("Register inst_schedule failed with %d",probe_ret);
			return -ENOSYS;
		}
		configure_counters();

	} else {
		if(unlikely((probe_ret = register_kprobe(&kp_schedule))<0)){
			error("__switch_to register successful, but schedule failed");
			return -ENOSYS;
		}
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
	unregister_jprobe(&jp___switch_to);
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

