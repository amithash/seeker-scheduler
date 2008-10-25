
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
#include <linux/sched.h>
#include <asm/hw_irq.h>

#include <seeker.h>

#include "probe.h"
#include "intr.h"
#include "sample.h"


/* Now, now, now, let me explain what exactly happened for you to realize what 
 * went on here. I developed all this on the linux 2.6.18 kernel for the core 2
 * duo, Tipp, the guy who made p4sample also used sched_exit to probe when a 
 * thread exited. Now, in linux-2.6.23, they pulled the rug. changed that 
 * function's name from sched_exit to release_thread. And hence the kernel
 * version check,
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
#define SCHED_EXIT_EXISTS 1
#endif

extern int dev_open;
extern pid_t cpu_pid[NR_CPUS];
extern struct task_struct *ts[NR_CPUS];

struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};
#ifdef LOCAL_PMU_VECTOR
struct jprobe jp_smp_pmu_interrupt = {
	.entry = (kprobe_opcode_t *)inst_smp_apic_pmu_interrupt,
	.kp.symbol_name = PMU_ISR,
};
#endif

struct jprobe jp_release_thread = {
	.entry = (kprobe_opcode_t *)inst_release_thread,
#ifdef SCHED_EXIT_EXISTS
	.kp.symbol_name = "sched_exit",
#else
	.kp.symbol_name = "release_thread",
#endif
};

struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

extern int pmu_intr;

/*---------------------------------------------------------------------------*
 * Function: inst_smp_apic_pmu_interrupt
 * Descreption: This is the probe function on the PMU overflow ISR 
 * 		Does not exist if the kernel is not patched. (Why should it?)
 * Input Parameters: regs -> Not used,
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
#ifdef LOCAL_PMU_VECTOR
void inst_smp_apic_pmu_interrupt(struct pt_regs *regs)
{
	int i,ovf=0;
	if(likely(int_callbacks.is_interrupt(pmu_intr) > 0)){
		int_callbacks.clear_ovf_status(pmu_intr);
		if(likely(dev_open == 1)){
			do_sample();
		}
	}
	else{
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			if(i == pmu_intr) 
				continue;
			if(int_callbacks.is_interrupt(i) > 0){
				int_callbacks.clear_ovf_status(i);
				warn("Counter %d overflowed. Check if your sample_freq is unresonably large.\n",i);
				ovf=1;
			}
		}
		if(ovf == 0){
			warn("Supposedly no counter overflowed, check if something is wrong\n");
		}
	}
	jprobe_return();
}
#endif

/*---------------------------------------------------------------------------*
 * Function: inst_schedule
 * Descreption: Probes the schedule function. So, reschedules, system calls etc
 * 		can be detected and a new sample is started.
 * Input Parameters: kprobes parameter and regs. Not used.
 * Output Parameters: always returns 0
 *---------------------------------------------------------------------------*/
int inst_schedule(struct kprobe *p, struct pt_regs *regs)
{
	if(dev_open){
		do_sample();
	}
	return 0;
}

/*---------------------------------------------------------------------------*
 * Function: inst_release_thread
 * Descreption: The pid  / name of the application is logged
 * Input Parameters: exiting tasks struct.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void inst_release_thread(struct task_struct *t)
{
	if(dev_open){
		do_pid_log(t);
	}
	jprobe_return();
}


/*---------------------------------------------------------------------------*
 * Function: inst__switch_to
 * Descreption: Detects a task switch and records that, so we use the right
 * 		PID for our samples! There would be chaos without this!
 * Input Parameters: "from" task struct, "to" task struct
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	int cpu = smp_processor_id();
	cpu_pid[cpu] = to->pid;
	ts[cpu] = to;
	jprobe_return();
}
EXPORT_SYMBOL_GPL(inst___switch_to);


