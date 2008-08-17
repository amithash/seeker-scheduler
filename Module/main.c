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
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/fs.h>
#include <linux/cpumask.h>
#include <linux/kprobes.h>
#include <linux/vmalloc.h>

#if !defined(KERNEL_VERSION)
# define KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE + 1)
#endif

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

#include <seeker-headers.h>
#include "generic_log.h"

#include "seeker.h"

// Interrupt handling code
#include "intr.h"

// File handling code
#include "io.h"

// Kprobes using code
#include "probe.h"

// Code talking to pmu.
#include "sample.h"


#define SEEKER_SAMPLE_MINOR 240



/************************* Parameter variables *******************************/

static int log_events[MAX_COUNTERS_PER_CPU];
static unsigned int log_ev_masks[MAX_COUNTERS_PER_CPU];
static int log_num_events = 0;
static int sample_freq=100;
static int os_flag = 0;
static int pmu_intr = -1;

/************************* Declarations & Prototypes *************************/

static struct timer_list sample_timer;
static int sample_timer_started = 0;
static int cpu_counters[NR_CPUS][MAX_COUNTERS_PER_CPU];

static log_t *seeker_sample_log;
static pid_t cpu_pid[NR_CPUS] = {-1};

static struct file_operations seeker_sample_fops;
static struct miscdevice seeker_sample_mdev;
static int mdev_registered = 0;
static int kprobes_registered = 0;
static int dev_open = 0;


static spinlock_t sample_lock;

static void seeker_sampler_exit_handler(void);

static void clear_counters(void);
static int config_counters(void);

inline void seeker_sampler_exit_handler(void);
static void do_pid_log(struct task_struct *p);

/************************* Miscellaneous functions ***************************/


/*---------------------------------------------------------------------------*
 * Function: clear_counters
 * Descreption: Clears all counters which can be cleared.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void clear_counters(void){
	int i;
	int cpu = smp_processor_id();

	/* clear the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		counter_clear(cpu_counters[cpu][i]);
	}
	for(i = 0; i < NUM_FIXED_COUNTERS; i++) {
		fcounter_clear(i);
	}

	/* OTHER COUNTERS SHOULD BE CLEARED HERE (If it makes sense that is) */
}

/************************* Logging functions *********************************/


/*---------------------------------------------------------------------------*
 * Function: do_pid_log
 * Descreption: Called whenever release_thread/sched_exit is executed. Logs the pids.
 * Input Parameters: exiting tasks structure.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void do_pid_log(struct task_struct *p) {
	seeker_sampler_entry_t *pentry;

	if(unlikely((pentry = log_alloc(seeker_sample_log, sizeof(seeker_sampler_entry_t))) == NULL)){
		seeker_sampler_exit_handler();
		return;
	}
	pentry->type = PIDTAB_ENTRY;
	pentry->u.pidtab_entry.pid = (u32)(p->pid);
	pentry->u.pidtab_entry.total_cycles = 0;
	(void)memcpy(&(pentry->u.pidtab_entry.name), p->comm, 
			sizeof(pentry->u.pidtab_entry.name));

	log_commit(seeker_sample_log);
}


/*---------------------------------------------------------------------------*
 * Function: do_sample
 * Descreption: The mother load! This is the function which takes samples.
 * 		Called everytime an ISR goes off (Timer/PMU/schedule)
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void do_sample(void) {
	int i,j;
	int cpu;
	log_block_t *pentry;
	unsigned long long now, period;
	unsigned long long last_ts;
	int num = 0;

	read_time_stamp();  
	counter_read();
	fcounter_read();
	if(unlikely(read_temp() == -1)){
		printk(KERN_INFO "Temperature value not valid, retrying\n");
		read_temp();
	}

	/* OTHER COUNTER READ CALLS ARE DONE HERE */

	cpu = smp_processor_id();

	last_ts = get_last_time_stamp(cpu);
	now = get_time_stamp(cpu);
	period = now - last_ts;
	pentry = log_create();
	if(!pentry){
		seeker_sampler_exit_handler();
		goto out;
	}

	pentry->sample.type = SEEKER_SAMPLE;
	pentry->sample.u.seeker_sample.cpu = cpu;
	pentry->sample.u.seeker_sample.pid = cpu_pid[cpu];
	pentry->sample.u.seeker_sample.cycles = period;
	/* log the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		pentry->sample.u.seeker_sample.counters[i] = get_counter_data(cpu_counters[cpu][i], cpu);
		
	}
	/* log the fixed counters */
	for(j=0;j<NUM_FIXED_COUNTERS;j++){
		pentry->sample.u.seeker_sample.counters[i++] = get_fcounter_data(j,cpu);
	}

	pentry->sample.u.seeker_sample.counters[i++] = get_temp(cpu);
	
	/* GETTING DATA FROM OTHER COUNTERS GO HERE */

	log_link(pentry);
  
	out:
	clear_counters();
}




/************************* Initialization functions **************************/


/*---------------------------------------------------------------------------*
 * Function: config_counters
 * Descreption: configure all counters on the current cpu. 
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int config_counters(void){
	int i;
	int cpu = smp_processor_id();

	/* enable and configure the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		cpu_counters[cpu][i] =
		counter_enable(log_events[i], log_ev_masks[i], os_flag);
		if(unlikely(cpu_counters[cpu][i] < 0)) {
			printk("Could not allocate counter for event %d\n", log_events[i]);
			return -1;
		}
		printk("%d: Allocated counter %d for %d:%x\n", cpu, cpu_counters[cpu][i],
								log_events[i], log_ev_masks[i]);
	}

	fcounters_enable(os_flag);
	printk("%d: enabled the fixed counters\n",cpu);

	/* ENABLING AND CONFIGURATION OF COUNTERS ARE DONE HERE. 
	 * NOTE: This is different from initialization. This is where they are enabled 
	 * If such a facility is provided by the hardware */

	clear_counters();

	return 0;
}



/*---------------------------------------------------------------------------*
 * Function: msrs_init
 * Descreption: Initialize msrs if any for each counter used, on all online cpus
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int msrs_init(void){

	/* initialize the pmu counters */
	if(log_num_events > 0){
		if(unlikely(on_each_cpu((void*)pmu_init_msrs,NULL, 1,1) < 0)){
			printk("could not initialize counters !, line %d\n",__LINE__);
			return -1;
  		}                   
	}
	/* initialize the fixed pmu counters */
	if(unlikely(on_each_cpu((void*)fpmu_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize fixed counters !, line %d\n",__LINE__);
		return -1;
  	}                   
	/* initialize the temperature sensors */
	if(unlikely(on_each_cpu((void*)therm_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize the temperature sensors !, line %d\n",__LINE__);
		return -1;
  	}
	/* initialize the time stamp counter */
	if(unlikely(on_each_cpu((void*)tsc_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize the time stamp counter !, line %d\n",__LINE__);
		return -1;
  	}

	/* OTHER COUNTER's MSRS CAN BE INITIALIZED HERE. */

	// setup the counters modifications -- needed only for counters with information from seeker 
	// that is currently only for the variable pmu counters.
	if(unlikely(on_each_cpu((void*)config_counters,NULL, 1,1) < 0)){
		printk("could not configure counters!, line %d\n",__LINE__);
		return -1;
  	}

	return 0;
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_log_init
 * Descreption: registers seeker as a device with a dev name seeker_samples
 * 		This also tells the system what are the read/open/close 
 * 		functions.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int seeker_sample_log_init(void){

	spin_lock_init(&sample_lock);

	seeker_sample_fops.open = seeker_sample_open;
	seeker_sample_fops.release = seeker_sample_close;
	seeker_sample_fops.read = seeker_sample_log_read;
  
	seeker_sample_mdev.minor = SEEKER_SAMPLE_MINOR;
	seeker_sample_mdev.name = "seeker_samples";
	seeker_sample_mdev.fops = &seeker_sample_fops;

	if(unlikely(misc_register(&seeker_sample_mdev) < 0)) {
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
		printk("Monitoring only fixed counters and temperature. PMU NOT CONFIGURED\n");
	}

	if(unlikely(msrs_init() < 0)) {
		printk("msrs_init failure\n");
		return -1;
	}

	if(unlikely(seeker_sample_log_init()  <0)) {
		printk("seeker_sample_log_init failure\n");
		seeker_sampler_exit_handler();
		return -1;
	}

	printk("seeker sampler module loaded logging %d events\n", log_num_events + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS);

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
		printk("Fixed counter %d used as the sampling interrupt source, sampling every %d events\n",pmu_intr,sample_freq);
		if(sample_freq<=0){
			return -1;
		}
		if((probe_ret = register_jprobe(&jp_smp_pmu_interrupt)) < 0){
			printk(KERN_ALERT "Could not find %s to probe, returned %d\n",PMU_ISR,probe_ret);
			return -1;
		}
		if(on_each_cpu((void *)enable_apic_pmu, NULL, 1, 1) < 0){
			printk("Could not enable local pmu interrupt on all cpu's\n");
		}
		#else
		printk(KERN_ALERT "An attempt is made to use the pmu_intr facility without applying a seeker patch to the kernel. Exiting\n");
		return -1;
		#endif
	}

	/* Register kprobes */

	if((unlikely(probe_ret = register_kprobe(&kp_schedule)) < 0)){
		printk(KERN_ALERT "Could not find schededule to probe, returned %d\n",probe_ret);
		return -1;
	}
	if(unlikely((probe_ret = register_jprobe(&jp_release_thread)) < 0)){
		printk(KERN_ALERT "Could not find release_thread to probe, returned %d\n",probe_ret);
		return -1;
	}
	if(unlikely((probe_ret = register_jprobe(&jp___switch_to)) < 0)){
		printk(KERN_ALERT "Could not find __switch_to to probe, returned %d\n",probe_ret);
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
inline void seeker_sampler_exit_handler(void){
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

	if(likely(seeker_sample_log != NULL)) {
		log_free(seeker_sample_log);
		seeker_sample_log = NULL;
	}
	/* Just incase something happens when the device and interrupts are enabled.... */
	#ifdef LOCAL_PMU_VECTOR
	if(dev_open == 1){
		/* Disable interrupts if they were enabled in the first palce. */
		if(pmu_intr >= 0){
			/* Disable interrupts on all cpus */
			if(unlikely(on_each_cpu((void *)configure_disable_interrupts,NULL,1,1) < 0)){
				printk("Oops... Could not disable interrupts on all cpu's");
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
static void __exit seeker_sampler_exit(void){
	seeker_sampler_exit_handler();
}



/*---------------------------------------------------------------------------*
 * Descreption: Module Parameters.
 *---------------------------------------------------------------------------*/


module_param_named(sample_freq, sample_freq, int, 0444);
MODULE_PARM_DESC(sample_freq, "The sampling frequency, either samples per second or sample per x events");

module_param(os_flag, int, 0444);
MODULE_PARM_DESC(os_flag, "0->Sample in user space only, 1->Sample in user and kernel space");

module_param(pmu_intr, int, 0444);
MODULE_PARM_DESC(pmu_intr, "pmu_intr=x where x is the fixed counter which will be the interrupt source");

module_param_array(log_events, int, &log_num_events, 0444);
MODULE_PARM_DESC(log_events, "The event numbers. Refer *_EVENTS.pdf");

module_param_array(log_ev_masks, int, &log_num_events, 0444);
MODULE_PARM_DESC(log_ev_masks, "The masks for the corrosponding event. Refer *_EVENTS.pdf");

module_init(seeker_sampler_init);
module_exit(seeker_sampler_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("seeker-sampler samples the hardware performance counters at regular intervals specified by the user");

/* EOF */

