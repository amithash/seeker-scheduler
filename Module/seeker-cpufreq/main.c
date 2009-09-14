/*****************************************************
 * Copyright 2009 Amithash Prasad                    *
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/moduleparam.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "user.h"
#include "freq.h"


/********************************************************************************
 * 				Function Declarations				*
 ********************************************************************************/

static ssize_t cpufreq_seeker_showspeed(struct cpufreq_policy *policy, 
		char *buf);
static int cpufreq_seeker_governor(struct cpufreq_policy *policy,
				   unsigned int event);
static int cpufreq_seeker_setspeed(struct cpufreq_policy *policy, 
				   unsigned int freq);

/********************************************************************************
 * 				Global Datastructures 				*
 ********************************************************************************/

/* The cpufreq governor structure for this module */
struct cpufreq_governor seeker_governor = {
	.name = "seeker",
	.owner = THIS_MODULE,
	.max_transition_latency = CPUFREQ_ETERNAL,
	.governor = cpufreq_seeker_governor,
	.show_setspeed = cpufreq_seeker_showspeed,
	.store_setspeed = cpufreq_seeker_setspeed,
};

/* parameter for user to specify states allowed */
int allowed_states[MAX_STATES];

/* Length of the allowed_states array */
int allowed_states_length = 0;

/********************************************************************************
 * 				Macros						*
 ********************************************************************************/

/* Macro convert cpumask to an unsigned integer so that it can be printed */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))



/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

static int cpufreq_seeker_setspeed(struct cpufreq_policy *policy, unsigned int freq)
{
	debug("User wants speed to be %u on cpu %d",freq,policy->cpu);
	return __set_freq(policy->cpu,freq);
}

/*******************************************************************************
 * cpufreq_seeker_showspeed - cpufreq governor interface to show current speed.
 * @policy - the cpufreq policy for which the speed is required.
 * @buf - The buffer to place the speed in kHz.
 * @return - The number of bytes copied into buf.
 *
 * Copies a string which is the current speed for cpu defined in policy 
 * into buf and returns the number of bytes copied.
 *******************************************************************************/
static ssize_t cpufreq_seeker_showspeed(struct cpufreq_policy *policy, char *buf)
{
	debug("Someone called showspeed... so let's show them something");
	sprintf(buf,"%d",policy->cur);
	return (strlen(buf)+1)*sizeof(char);
}

/*******************************************************************************
 * cpufreq_seeker_governor - The governor interface to seeker_cpufreq.
 * @policy - The policy of the cpu we are dealing with.
 * @event - describes what we are supposed to do.
 *
 * event=CPUFREQ_GOV_START - Enables seeker-cpufreq on policy->cpu
 * event=CPUFREQ_GOV_STOP  - Disables seeker-cpufreq on policy->cpu
 * event=CPUFREQ_GOV_LIMITS - Does nothing. Stop someone from trying to change
 * 			      speed.
 *******************************************************************************/
static int cpufreq_seeker_governor(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	switch (event) {
	case CPUFREQ_GOV_START:
		info("Starting governor on cpu %d", cpu);
		gov_init_freq_info_cpu(cpu,policy);
		info("Latency for cpu %d = %d nanoseconds",cpu,
					policy->cpuinfo.transition_latency);
		break;
	case CPUFREQ_GOV_STOP:
		info("Stopping governor on cpu %d", cpu);
		gov_exit_freq_info_cpu(cpu);
		break;
	case CPUFREQ_GOV_LIMITS:
		info("Ha ha, Nice try!");
		break;
	default:
		info("Unknown");
		break;
	}
	return 0;
}




/*******************************************************************************
 * seeker_cpufreq_init - Init function for seeker_cpufreq
 * @return  - 0 if success, error code otherwise.
 * @Side Effects - freq_info is populated by reading from cpufreq_table.
 *
 * Initialize seeker_cpufreq's data structures and register self as a governor.
 *******************************************************************************/
static int __init seeker_cpufreq_init(void)
{
	cpufreq_register_governor(&seeker_governor);

	init_user_interface();

	init_freqs();

	if(init_freqs()){
		cpufreq_unregister_governor(&seeker_governor);
		return -ENODEV;
	}

	return 0;
}

/*******************************************************************************
 * seeker_cpufreq_exit - seeker_cpufreq's exit routine.
 *
 * Unregister self as a governor.
 *******************************************************************************/
static void __exit seeker_cpufreq_exit(void)
{
	exit_user_interface();
	cpufreq_unregister_governor(&seeker_governor);
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);

module_param_array(allowed_states, int, &allowed_states_length, 0444);
MODULE_PARM_DESC(allowed_states, "Use this to provide an array for allowed states.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");


