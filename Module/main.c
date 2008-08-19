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

#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/fs.h>

#include <seeker-headers.h>
#include "seeker.h"
#include "intr.h"
#include "io.h"
#include "probe.h"
#include "sample.h"
#include "alloc.h"
#include "log.h"
#include "exit.h"


#define SEEKER_SAMPLE_MINOR 240

/************************* Parameter variables *******************************/

int log_events[MAX_COUNTERS_PER_CPU];
unsigned int log_ev_masks[MAX_COUNTERS_PER_CPU];
int log_num_events = 0;
int sample_freq=100;
int os_flag = 0;
int pmu_intr = -1;

/************************* Declarations & Prototypes *************************/


static struct file_operations seeker_sample_fops;
static struct miscdevice seeker_sample_mdev;
static int mdev_registered = 0;
static int kprobes_registered = 0;
int dev_open = 0;

extern struct timer_list sample_timer;
extern int sample_timer_started;

extern struct kprobe kp_schedule;
#ifdef LOCAL_PMU_VECTOR
extern struct jprobe jp_smp_pmu_interrupt;
#endif
extern struct jprobe jp_release_thread;
extern struct jprobe jp___switch_to;


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_log_init
 * Descreption: registers seeker as a device with a dev name seeker_samples
 * 		This also tells the system what are the read/open/close 
 * 		functions.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int seeker_sample_log_init(void)
{
	log_init();
	seeker_sample_fops.open = seeker_sample_open;
	seeker_sample_fops.release = seeker_sample_close;
	seeker_sample_fops.read = seeker_sample_log_read;
  
	seeker_sample_mdev.minor = SEEKER_SAMPLE_MINOR;
	seeker_sample_mdev.name = "seeker_samples";
	seeker_sample_mdev.fops = &seeker_sample_fops;

	if(unlikely(misc_register(&seeker_sample_mdev) < 0)) {
		error("Device register failed");
		return -1;
	} else {
		mdev_registered = 1;
	}
	return 0;
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sampler_init
 * Descreption: Initialization function called at module creation time.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int __init seeker_sampler_init(void)
{
	int i;
	int probe_ret;

	printk("---------------------------------------\n");
	if( log_num_events <= 0 ) {
		printk("Monitoring only fixed counters and "
		       "temperature. PMU NOT CONFIGURED\n");
	}

	if(unlikely(msrs_init() < 0)) {
		error("msrs_init failure\n");
		return -1;
	}

	if(unlikely(seeker_sample_log_init()  <0)) {
		error("seeker_sample_log_init failure\n");
		seeker_sampler_exit_handler();
		return -1;
	}

	printk("seeker sampler module loaded logging %d events\n", 
	       log_num_events + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS);

	for(i = 0; i < log_num_events; i++) {
		printk("%d:0x%x ", log_events[i], log_ev_masks[i]);
	}
	printk("\n");
	printk("Fixed pmu 1,2,3 and temperature also monitored\n");

	if(pmu_intr == -1){
		printk("Timer is used as sampling interrupt source");
		if( sample_freq ) {
			sample_freq = HZ/sample_freq;
			printk("seeker sampler sampling every %d jiffies, HZ is %d\n",
				sample_freq, HZ);

			init_timer(&sample_timer);
			sample_timer.function = &do_timer_sample;
			mod_timer(&sample_timer, jiffies + sample_freq);
			sample_timer_started = 1;
		}
	}
	else{
		#ifdef LOCAL_PMU_VECTOR
		printk("Fixed counter %d used as the sampling interrupt source, "
		       "sampling every %d events\n",pmu_intr,sample_freq);
		if(sample_freq<=0){
			return -1;
		}
		if((probe_ret = register_jprobe(&jp_smp_pmu_interrupt)) < 0){
			printk(KERN_ALERT "Could not find %s to probe, returned %d\n",
			       PMU_ISR,probe_ret);
			return -1;
		}
		if(on_each_cpu((void *)enable_apic_pmu, NULL, 1, 1) < 0){
			printk("Could not enable local pmu interrupt on all cpu's\n");
		}
		#else
		error("An attempt is made to use the pmu_intr "
		"facility without applying a seeker patch to the kernel. Exiting");
		return -1;
		#endif
	}

	/* Register kprobes */

	if((unlikely(probe_ret = register_kprobe(&kp_schedule)) < 0)){
		error("Could not find schededule to probe, returned %d",probe_ret);
		return -1;
	}
	if(unlikely((probe_ret = register_jprobe(&jp_release_thread)) < 0)){
		error("Could not find release_thread to probe, returned %d",probe_ret);
		return -1;
	}
	if(unlikely((probe_ret = register_jprobe(&jp___switch_to)) < 0)){
		error("Could not find __switch_to to probe, returned %d",probe_ret);
		return -1;
	}
	kprobes_registered = 1;
	printk("kprobe registered\n");


	return 0;
}



/*---------------------------------------------------------------------------*
 * Function: seeker_samper_exit_handler
 * Descreption: This is the exit handler. Called by the module exit function
 * 		and any error check - failures.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
void seeker_sampler_exit_handler(void)
{
	if(dev_open)
		seeker_sample_close(NULL,NULL);
	
	if( sample_timer_started ) {
		del_timer_sync(&sample_timer);
		sample_timer_started = 0;
	}

	if( kprobes_registered ) {
		unregister_kprobe(&kp_schedule);
		unregister_jprobe(&jp_release_thread);
		unregister_jprobe(&jp___switch_to);
		#ifdef LOCAL_PMU_VECTOR
		if(pmu_intr >=0){
			unregister_jprobe(&jp_smp_pmu_interrupt);
		}
		#endif
		printk("kprobe unregistered\n");
		kprobes_registered = 0;
	}

	if( mdev_registered ) {
		misc_deregister(&seeker_sample_mdev);
		mdev_registered = 0;
	}
	log_finalize();
	/* Just incase something happens when the device and interrupts are enabled.... */
	#ifdef LOCAL_PMU_VECTOR
	if(dev_open == 1){
		/* Disable interrupts if they were enabled in the first palce. */
		if(pmu_intr >= 0){
			/* Disable interrupts on all cpus */
			if(unlikely(on_each_cpu((void *)configure_disable_interrupts,NULL,1,1) < 0)){
				error("Oops... Could not disable interrupts on all cpu's");
			}
		}
	}
	#endif
}	

/*---------------------------------------------------------------------------*
 * Function: seeker_sampler_exit
 * Descreption: Called when the module is unloaded. It in turn calls the exit 
 * 		handler.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void __exit seeker_sampler_exit(void)
{
	seeker_sampler_exit_handler();
}



/*---------------------------------------------------------------------------*
 * Descreption: Module Parameters.
 *---------------------------------------------------------------------------*/


module_param_named(sample_freq, sample_freq, int, 0444);
MODULE_PARM_DESC(sample_freq, "The sampling frequency, either "
			      "samples per second or sample per x events");

module_param(os_flag, int, 0444);
MODULE_PARM_DESC(os_flag, "0->Sample in user space only, "
			  "1->Sample in user and kernel space");

module_param(pmu_intr, int, 0444);
MODULE_PARM_DESC(pmu_intr, "pmu_intr=x where x is the fixed counter "
			   "which will be the interrupt source");

module_param_array(log_events, int, &log_num_events, 0444);
MODULE_PARM_DESC(log_events, "The event numbers. Refer *_EVENTS.pdf");

module_param_array(log_ev_masks, int, &log_num_events, 0444);
MODULE_PARM_DESC(log_ev_masks, "The masks for the corrosponding "
			       "event. Refer *_EVENTS.pdf");

module_init(seeker_sampler_init);
module_exit(seeker_sampler_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("seeker-sampler samples the hardware performance "
		   "counters at regular intervals specified by the user");

/* EOF */

