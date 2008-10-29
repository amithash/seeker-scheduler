
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
#include <linux/jiffies.h>
#include <asm/apic.h>
#include <asm/apicdef.h>
#include <asm/hw_irq.h>

#include <seeker.h>

#include "intr.h"
#include "sample.h"

extern int dev_open;
extern int sample_freq;
extern int pmu_intr;

struct timer_list sample_timer;
int sample_timer_started = 0;

struct struct_int_callbacks int_callbacks = {NULL,NULL,NULL,NULL,NULL};


/*---------------------------------------------------------------------------*
 * Function: do_timer_sample
 * Descreption: This is the timer ISR. Every time it goes off, a sample is taken
 * 		on all online cpu's. (See the call to do_sample) and re-calibrates
 * 		the timer,
 * Input Parameters: param -> not used,
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void do_timer_sample(unsigned long param)
{
	if(dev_open){
		if(unlikely(on_each_cpu((void*)do_sample, NULL, 1,1) < 0)){
			error("could not sample on all cpu's\n");
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
void configure_enable_interrupts(void)
{
		/* Configure the initial counter value as (-1) * sample_freq */
		int_callbacks.configure_interrupts(pmu_intr,((u32)0xFFFFFFFF-(u32)sample_freq + 2),0xFFFFFFFF);
		/* clear overflow flag, just to be sure. */
		int_callbacks.clear_ovf_status(pmu_intr);
		/* Now enable the interrupt */
		int_callbacks.enable_interrupts(pmu_intr);
}
	
/*---------------------------------------------------------------------------*
 * Function: configure_disable_interrupts
 * Descreption: Resets any configuration and disables all interrupts.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void configure_disable_interrupts(void)
{
		/* Configure the initial counter value as (-1) * sample_freq */
		int_callbacks.configure_interrupts(pmu_intr,0,0);
		/* clear overflow flag, just to be sure. */
		int_callbacks.clear_ovf_status(pmu_intr);
		/* Now enable the interrupt */
		int_callbacks.disable_interrupts(pmu_intr);
}

/*---------------------------------------------------------------------------*
 * Function: enable_apic_pmu
 * Descreption: Enables PMU Interrupts for the current cpu's apic 
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void enable_apic_pmu(void)
{
	apic_write(APIC_LVTPC, LOCAL_PMU_VECTOR);
}
#endif

