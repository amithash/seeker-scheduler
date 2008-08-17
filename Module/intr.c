
/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
 * Copyright 2006 Tipp Mosely                                             *
 *                                                                        *
 * This file is part of Seeker                                            *
 *                                                                        *
 * Seeker is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/


#include <asm/apic.h>
#include <asm/apicdef.h>
#include <asm/hw_irq.h>

#include "intr.h"

#define PMU_ISR "smp_apic_pmu_interrupt" 

/*---------------------------------------------------------------------------*
 * Function: inst_smp_apic_pmu_interrupt
 * Descreption: This is the probe function on the PMU overflow ISR 
 * 		Does not exist if the kernel is not patched. (Why should it?)
 * Input Parameters: regs -> Not used,
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
#ifdef LOCAL_PMU_VECTOR
void inst_smp_apic_pmu_interrupt(struct pt_regs *regs){
	int i,ovf=0;
	if(likely(fpmu_is_interrupt(pmu_intr) > 0)){
		fpmu_clear_ovf_status(pmu_intr);
		if(likely(dev_open == 1)){
			do_sample();
		}
	}
	else{
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			if(i == pmu_intr) continue;
			if(fpmu_is_interrupt(i) > 0){
				fpmu_clear_ovf_status(i);
				printk("Counter %d overflowed. Check if your sample_freq is unresonably large.\n",i);
				ovf=1;
			}
		}
		if(ovf == 0){
			printk("Supposedly no counter overflowed, check if something is wrong\n");
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
static int inst_schedule(struct kprobe *p, struct pt_regs *regs){
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
static void inst_release_thread(struct task_struct *t){
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
static void inst___switch_to(struct task_struct *from, struct task_struct *to){
	cpu_pid[smp_processor_id()] = to->pid;
	jprobe_return();
}

/*---------------------------------------------------------------------------*
 * Function: do_timer_sample
 * Descreption: This is the timer ISR. Every time it goes off, a sample is taken
 * 		on all online cpu's. (See the call to do_sample) and re-calibrates
 * 		the timer,
 * Input Parameters: param -> not used,
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void do_timer_sample(unsigned long param){
	if(dev_open){
		if(unlikely(on_each_cpu((void*)do_sample, NULL, 1,1) < 0)){
			printk("could not sample on all cpu's\n");
		}
	}

	mod_timer(&sample_timer, jiffies + sample_freq);  
}

#ifdef LOCAL_PMU_VECTOR
/*---------------------------------------------------------------------------*
 * Function: configure_enable_interrupt
 * Descreption: configures the counter:pmu_intr to overflow every:sample_freq
 * 		and then enables interrupts.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void configure_enable_interrupts(void){
		/* Configure the initial counter value as (-1) * sample_freq */
		fpmu_configure_interrupt(pmu_intr,((u32)0xFFFFFFFF-(u32)sample_freq + 2),0xFFFFFFFF);
		/* clear overflow flag, just to be sure. */
		fpmu_clear_ovf_status(pmu_intr);
		/* Now enable the interrupt */
		fpmu_enable_interrupt(pmu_intr);
}
	
/*---------------------------------------------------------------------------*
 * Function: configure_disable_interrupts
 * Descreption: Resets any configuration and disables all interrupts.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void configure_disable_interrupts(void){
		/* Configure the initial counter value as (-1) * sample_freq */
		fpmu_configure_interrupt(pmu_intr,0,0);
		/* clear overflow flag, just to be sure. */
		fpmu_clear_ovf_status(pmu_intr);
		/* Now enable the interrupt */
		fpmu_disable_interrupt(pmu_intr);
}

/*---------------------------------------------------------------------------*
 * Function: enable_apic_pmu
 * Descreption: Enables PMU Interrupts for the current cpu's apic 
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void enable_apic_pmu(void){
	apic_write_around(APIC_LVTPC, LOCAL_PMU_VECTOR);
}
#endif

