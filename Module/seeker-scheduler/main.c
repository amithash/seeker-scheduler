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
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/cpumask.h>

#include <seeker.h>


#include "seeker_cpufreq.h"
#include "state.h"
#include "stats.h"
#include "migrate.h"
#include "mutate.h"
#include "other_mutators.h"
#include "probe.h"
#include "assigncpu.h"
#include "debug.h"
#include "hwcounters.h"
#include "tsc_intf.h"
#include "sched_debug.h"

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

static void state_change(struct work_struct *w);

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* The mutataor's work struct */
static DECLARE_DELAYED_WORK(state_work, state_change);

/* Mutator interval time in jiffies */
static u64 interval_jiffies;

/* Timer started flag */
static int timer_started = 0;

/* Contains the value of num_online_cpus(), updated by init */
int total_online_cpus = 0;

/* Mask of all allowed cpus */
cpumask_t total_online_mask = CPU_MASK_NONE;

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/


extern struct state_desc states[MAX_STATES];


/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

/* Mutator interval in seconds */
int change_interval = 1000;

/* flag requesting disabling the scheduler and mutator */
int disable_scheduling = 0;

/* DELTA of the mutator */
int delta = 1;

/* method of initializing the states */
int init = ALL_LOW;

/* Static Layout of states */
int static_layout[NR_CPUS];

/* Count of elements in static_layout */
int static_layout_length = 0;

/* allowed cpus to limit total cpus used. */
int allowed_cpus = 0;

int mutation_method = 0;

/* The home state for the scheduler and the mutator */
int base_state = 0;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * state_change - work callback 
 * @w - the work struct calling this function.
 * @return - None
 * @Side Effect - Calls mutator which does its thing, and schedules itself. 
 *
 * The work routine responsible to call the mutator
 *******************************************************************************/
static void state_change(struct work_struct *w)
{
	debug("State change now @ %ld", jiffies);
	switch(mutation_method){
		case 1: 
			ondemand();
			break;
		case 2: 
			conservative();
			break;
		case 0:
		default:
			choose_layout(delta);
	}
	if (timer_started) {
		schedule_delayed_work(&state_work, interval_jiffies);
	}
}

/*******************************************************************************
 * scheduler_init - Module init function
 * @return - 0 if success, else a negative error code.
 *
 * Initializes the different elements, registers all the probes, starts the 
 * mutator work. 
 *******************************************************************************/
static int scheduler_init(void)
{
	int i;

#ifndef SEEKER_PLUGIN_PATCH

#warning "This module will NOT work on this unpatched, unblessed kernel"

	error("You are trying to use this module without patching "
	      "the kernel with schedmod. Refer to the "
	      "seeker/Patches/README for details");

	return -ENOTSUPP;

#endif


	if (static_layout_length != 0) {
		init = STATIC_LAYOUT;
	}
	if (base_state != 0){
		init = BASE_LAYOUT;
		if(base_state < 0){
			base_state = 0;
		}
	}
	if(mutation_method != 0){
		disable_scheduling = 1;
	}
	if (disable_scheduling != 0) {
		disable_scheduling = 1;
	}

	total_online_cpus = num_online_cpus();

	if(allowed_cpus != 0){
		if(allowed_cpus > 0 && allowed_cpus <= total_online_cpus){
			total_online_cpus = allowed_cpus;
		} else {
			warn("allowed_cpus has to be within (0,%d]",total_online_cpus);
		}
	}
	cpus_clear(total_online_mask);
	for(i=0;i<total_online_cpus;i++){
		cpu_set(i,total_online_mask);
	}

	info("Total online mask = %x\n",CPUMASK_TO_UINT(total_online_mask));

	init_mig_pool();

	init_idle_logger();

	if (init_tsc_intf()) {
		error("Could not init tsc_intf");
		return -ENOSYS;
	}

	if (configure_counters() != 0) {
		error("Configuring counters failed");
		return -ENOSYS;
	}
	info("Configuring counters was successful");

	/* Please keep this BEFORE the probe registeration and
	 * the timer initialization. init_cpu_states makes this 
	 * assumption by not taking any locks */
	init_cpu_states(init);
	if (!(init == STATIC_LAYOUT || disable_scheduling))
		init_mutator();

	if (debug_init() != 0)
		return -ENODEV;

  if(insert_probes()){
    return -ENOSYS;
  }

	if (init != STATIC_LAYOUT) {
		interval_jiffies = (change_interval * HZ) / 1000;
		if(interval_jiffies < 1){
			warn("change_interval=%dms makes the interval lower"
				"than the scheduling quanta. adjusting it to equal"
				"to the quanta = %dms",change_interval,(1000/HZ));
			interval_jiffies = 1;
			change_interval = 1000 / HZ;
		}

		timer_started = 1;
		init_timer_deferrable(&state_work.timer);
		schedule_delayed_work(&state_work, interval_jiffies);
		info("Started Timer");
	}
	init_assigncpu_logger();


  return 0;
}

/*******************************************************************************
 * scheduler_exit - Module exit functionn
 *
 * Unregisters all probes, stops the mutator work if it has started.
 * Stops and kicks anyone using the debug interface. 
 *******************************************************************************/
static void scheduler_exit(void)
{
	debug("removing the state change timer");
	exit_cpu_states();
	if (timer_started) {
		timer_started = 0;
		cancel_delayed_work(&state_work);
	}
	stop_state_logger();

  if(remove_probes()){
    error("ERROR: Probes not properly removed");
  }

	debug_exit();
	debug("Exiting the counters");
	exit_counters();
	exit_mig_pool();
	exit_assigncpu_logger();
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(scheduler_init);
module_exit(scheduler_exit);

module_param(change_interval, int, 0444);
MODULE_PARM_DESC(change_interval,
		 "Interval in ms to change the global state (Default: 1000)");

module_param(mutation_method, int, 0444);
MODULE_PARM_DESC(mutation_method, 
		"Type of mutation: Delta (default) - 0, ondemand - 1, conservative - 2");

module_param(base_state, int, 0444);
MODULE_PARM_DESC(base_state,
		"The base state the scheduler/mutator shall take");

module_param(init, int, 0444);
MODULE_PARM_DESC(init,
		 "Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");
module_param(allowed_cpus, int, 0444);
MODULE_PARM_DESC(allowed_cpus, "Limit cpus to this number, default is all online cpus.");

module_param(disable_scheduling, int, 0444);
MODULE_PARM_DESC(disable_scheduling,
		 "Set to not allow scheduling. Does a dry run. Also enables static layout.");

module_param_array(static_layout, int, &static_layout_length, 0444);
MODULE_PARM_DESC(static_layout, "Use to set a static_layout to use");

module_param(delta, int, 0444);
MODULE_PARM_DESC(delta, "Type of state machine to use 1,2,.. default:1");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("instruments scheduling functions to do extra work");
