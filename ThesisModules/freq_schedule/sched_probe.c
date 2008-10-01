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
#include "../Module/seeker.h"

#define SIG_IRQ 0x10

#define INIT_CPU_MASK ~(0xFFFFFFFF << NR_CPUS)
#define CPU_TO_MASK(cpu) 1 << (cpu)
#define MASK_TO_CPU(mask) __mask_to_cpu((mask))

pid_t cpu_pid[NR_CPUS];
struct task_struct *ts[NR_CPUS];

static irqreturn_t int_90_handler(int a, void *info);
inline unsigned int __mask_to_cpu(unsigned int mask);
void inst___switch_to(struct task_struct *from, struct task_struct *to);

struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

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
// p->cpu_allowed = mask
//

inline unsigned int __mask_to_cpu(unsigned int mask)
{
	unsigned int cpu = -1;
	while(mask){
		cpu++;
		mask >>= 1;
	}
	return cpu;
}

void inst___switch_to(struct task_struct *from, struct task_struct *to)
{
	cpu_pid[smp_processor_id()] = to->pid;
	ts[smp_processor_id()] = to;
	jprobe_return();
}

static irqreturn_t int_90_handler(int a, void *dev_id)
{
	warn("Testing Interrupt %x handling",SIG_IRQ);
	return IRQ_HANDLED;
}


static int __init scheduler_init(void)
{
	int i;
	int probe_ret;
	for(i=0;i<NR_CPUS;i++){
		ts[i] = NULL;
		cpu_pid[i] = -1;
	}
	if(unlikely((probe_ret = register_jprobe(&jp___switch_to)))){
		error("Could not find __switch_to to probe, returned %d",probe_ret);
		return probe_ret;
	}
	if(unlikely(probe_ret = request_irq(SIG_IRQ, int_90_handler, IRQF_DISABLED, "int90", NULL))){
		error("Could not assign int %x, return=%d",SIG_IRQ,probe_ret);
		return probe_ret;
	}
	return 0;
}

static void __exit scheduler_exit(void)
{
	unregister_jprobe(&jp___switch_to);
	free_irq(90, NULL);
}

module_init(scheduler_init);
module_exit(scheduler_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("instruments scheduling functions to do extra work");

