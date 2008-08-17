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

#include "sinterrupt.h"
#include "sio.h"
#include "sprobe.h"


#define SEEKER_SAMPLE_MINOR 240

#define PMU_ISR "smp_apic_pmu_interrupt" 


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

static int inst_schedule(struct kprobe *p, struct pt_regs *regs);
static void inst_release_thread(struct task_struct *t);
static void inst___switch_to(struct task_struct *from,
			     struct task_struct *to);
/* I am going to say this only once. If you do not want to use
 * pmu_intr, then why should I force you to patch your kernel?
 * That is completely uncalled for! And hence I am checking for
 * one define I have introduced into the kernel. And hence if you
 * have not patched the kernel, you do not get to use the feature,
 * and live happly ever after with the timer interrupt!
 */
#ifdef LOCAL_PMU_VECTOR
void inst_smp_apic_pmu_interrupt(struct pt_regs *regs);
#endif

static struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};
#ifdef LOCAL_PMU_VECTOR
static struct jprobe jp_smp_pmu_interrupt = {
	.entry = (kprobe_opcode_t *)inst_smp_apic_pmu_interrupt,
	.kp.symbol_name = PMU_ISR,
};
#endif

static struct jprobe jp_release_thread = {
	.entry = (kprobe_opcode_t *)inst_release_thread,
#ifdef SCHED_EXIT_EXISTS
	.kp.symbol_name = "sched_exit",
#else
	.kp.symbol_name = "release_thread",
#endif
};

static struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

static spinlock_t sample_lock;

static int generic_open(struct inode *i, struct file *f) {return 0;}
static int generic_close(struct inode *i, struct file *f) {return 0;}
static void seeker_sampler_exit_handler(void);

static void clear_counters(void);
static int config_counters(void);

static int seeker_sample_open(struct inode *in, struct file * f);
static int seeker_sample_close(struct inode *in, struct file *f);
inline void seeker_sampler_exit_handler(void);
static void do_pid_log(struct task_struct *p);
#ifdef LOCAL_PMU_VECTOR
static void configure_enable_interrupts(void);
static void configure_disable_interrupts(void);
static void enable_apic_pmu(void);
#endif

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
	seeker_sampler_entry_t *pentry;
	unsigned long long now, period;
	unsigned long long last_ts;
	unsigned long long pmu_ctrs[NUM_COUNTERS + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS];
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
	/* log the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		pmu_ctrs[i] = get_counter_data(cpu_counters[cpu][i], cpu);
	}
	/* log the fixed counters */
	for(j=0;j<NUM_FIXED_COUNTERS;j++){
		pmu_ctrs[i++] = get_fcounter_data(j,cpu);
	}

	pmu_ctrs[i++] = get_temp(cpu);
	
	/* GETTING DATA FROM OTHER COUNTERS GO HERE */

	num = i;

	spin_lock(&sample_lock);
	pentry = log_alloc(seeker_sample_log, sizeof(seeker_sampler_entry_t));

	/* if no memory was allocated! :-( */
	if(unlikely(pentry == NULL)) {
		seeker_sampler_exit_handler();
		goto out;
	}

	pentry->type = SEEKER_SAMPLE;
	pentry->u.seeker_sample.cpu = cpu;
	pentry->u.seeker_sample.pid = cpu_pid[cpu];
	pentry->u.seeker_sample.cycles = period;
    
	/* log the pmu counters */
	for(i = 0; i < num; i++) {
		pentry->u.seeker_sample.counters[i] = pmu_ctrs[i];
	}
	log_commit(seeker_sample_log);
  
	out:
	spin_unlock(&sample_lock);
	clear_counters();
}

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

/************************* FILE OPS FUNCTIONS *******************************/

/*---------------------------------------------------------------------------*
 * Function: seeker_sample_log_read
 * Descreption: This is the "read" function. So daemons can read the buffer.
 * Input Parameters: Same params as a generic read function.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static ssize_t seeker_sample_log_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset){
	return log_read(seeker_sample_log, file_ptr, buf, count, offset);
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_open
 * Descreption: Seeker's file 'open' handle Creates and initializes the kernel 
 * 		buffer and enables PMU interrupts if required, and starts sampling
 * 		by setting dev_open to 1.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int seeker_sample_open(struct inode *in, struct file * f){

	int i,retval=0;
	seeker_sampler_entry_t *pentry;

	if(unlikely((seeker_sample_log = log_create(4096 * sizeof(seeker_sample_t))) == NULL)) {
		return -1;
	}

	if(unlikely((pentry = log_alloc(seeker_sample_log, sizeof(seeker_sampler_entry_t))) == NULL)){
		return -1;
	}

	pentry->type = SAMPLE_DEF;
	pentry->u.seeker_sample_def.num_counters = log_num_events + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS;
	for(i = 0; i < log_num_events; i++) {
		pentry->u.seeker_sample_def.counters[i] = log_events[i];
		pentry->u.seeker_sample_def.masks[i] = log_ev_masks[i];
	}

	/* add definations for the fixed counters:
	* 0x01:0x00 = FIXED COUNTER 0; 
	* 0x02:0x00 = FIXED COUNTER 1; 
	* 0x03:0x00 = FIXED COUNTER 2; 
	* 0x04:0x00 = TEMPERATURE
	*/
	pentry->u.seeker_sample_def.counters[i] = 0x01;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x02;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x03;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x04;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;

	log_commit(seeker_sample_log);

	retval = generic_open(in,f);
	dev_open = 1;
	/* Enable and configure interrupts on each cpu */
	#ifdef LOCAL_PMU_VECTOR
	if(pmu_intr >= 0){
		if(on_each_cpu((void *)configure_enable_interrupts,NULL,1,1) < 0){
			printk("Could not configure interrupts on all cpu's");
		}
	}
	#endif
	return retval;
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_close
 * Descreption: seeker's file close handle. Frees all log buffers, disables 
 * 		interrupts if enabled, and stops any sampling by setting
 * 		dev_open to 0.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int seeker_sample_close(struct inode *in, struct file *f){
	int retval=0;
	dev_open = 0;
	/* Disable interrupts on each cpu. */
	#ifdef LOCAL_PMU_VECTOR
	if(pmu_intr >= 0){
		if(unlikely(on_each_cpu((void *)configure_disable_interrupts,NULL,1,1) < 0)){
			printk("Oops... Could not disable interrupts on all cpu's");
		}
	}
	#endif
	if(likely(seeker_sample_log != NULL)) {
		log_free(seeker_sample_log);
		seeker_sample_log = NULL;
	}
	retval = generic_close(in,f);
	return retval;
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

