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


/********************************************************************************
 * 				Global Prototyprs 				*
 ********************************************************************************/

/* Per cpu structure to keep the information for each cpu */
struct freq_info_t {
	unsigned int cpu;
	unsigned int cur_freq;
	unsigned int num_states;
	unsigned int table[MAX_STATES];
	unsigned int valid_entry[MAX_STATES];
	unsigned int latency;
	struct cpufreq_policy *policy;
};


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

/* Have a per-cpu information of freq_info. */
static DEFINE_PER_CPU(struct freq_info_t, freq_info);


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
static int allowed_states[MAX_STATES];

/* Length of the allowed_states array */
static int allowed_states_length = 0;

/********************************************************************************
 * 				Macros						*
 ********************************************************************************/

/* Macro convert cpumask to an unsigned integer so that it can be printed */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

/* Macro to make the access of the per-cpu structure easy */
#define FREQ_INFO(cpu) (&per_cpu(freq_info,(cpu)))

#define DRIVER_TARGET_LOCKING 1
#define DRIVER_TARGET_NONLOCKING 0

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
		FREQ_INFO(cpu)->policy = policy;
		FREQ_INFO(cpu)->latency = policy->cpuinfo.transition_latency;
		info("Latency for cpu %d = %d nanoseconds",cpu,FREQ_INFO(cpu)->latency);
		break;
	case CPUFREQ_GOV_STOP:
		info("Stopping governor on cpu %d", cpu);
		FREQ_INFO(cpu)->policy = NULL;
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
 * get_freq - Get the current performance number for cpu.
 * @cpu - cpu for which the performance number is required.
 *
 * returns the performance number for cpu = `cpu`.
 *******************************************************************************/
unsigned int get_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cpu >= 0) {
		if (FREQ_INFO(cpu)->cur_freq != -1)
			return FREQ_INFO(cpu)->cur_freq;
	}
	return -1;
}

EXPORT_SYMBOL_GPL(get_freq);

/*******************************************************************************
 * driver_target - Wrapper for cpufreq_driver_target.
 * @policy - the policy struct of the cpu.
 * @frequency - desired frequency
 * @relation - If frequency is not avaliable, higher or lower?
 * @locking - see discreption.
 *
 * if locking = DRIVER_TARGET_LOCKING ->
 * cpufreq_driver_target(policy,frequency,relation) is called
 * if locking = DRIVER_TARGET_NONLOCKING ->
 * __cpufreq_driver_target(policy,frequency,relation) is called
 *
 * If in doubt use DRIVER_TARGET_LOCKING, and only switch to 
 * DRIVER_TARGET_NONLOCKING if things hang.
 *******************************************************************************/
static int driver_target(struct cpufreq_policy *policy, unsigned int frequency, 
		  unsigned int relation, int locking)
{
	if(locking == DRIVER_TARGET_NONLOCKING)
		return __cpufreq_driver_target(policy,frequency,relation);

	return cpufreq_driver_target(policy,frequency,relation);
}

/*******************************************************************************
 * internal_set_freq - called by set_freq and __set_freq.
 * @cpu - cpu for which change is reqired.
 * @freq_ind - Performance number required.
 * @locking - see below.
 *
 * if locking is passed on to driver_target below.
 * See above. This is done this way to remove code duplication between
 * set_freq and __set_freq.
 *******************************************************************************/
static int internal_set_freq(unsigned int cpu, unsigned int freq_ind, int locking)
{
	int ret_val;
	struct cpufreq_policy *policy = NULL;
	if (unlikely(cpu >= NR_CPUS || freq_ind >= FREQ_INFO(cpu)->num_states))
		return -1;
	policy = FREQ_INFO(cpu)->policy;
	if (!policy) {
		error("Error, governor not initialized for cpu %d", cpu);
		return -1;
	}
	policy->cur = FREQ_INFO(cpu)->table[freq_ind];
	ret_val = driver_target(policy, policy->cur, CPUFREQ_RELATION_H,locking);
	if(ret_val == -EAGAIN)
		ret_val = driver_target(policy,policy->cur,CPUFREQ_RELATION_H,locking);
	if(ret_val){
		error("Target did not work for cpu %d transition to %d, "
		      "with a return error code: %d",cpu,policy->cur,ret_val);
		return ret_val;
	}
	debug("Setting frequency of %d to %d",cpu,policy->cur);
	inform_freq_change(cpu,freq_ind);

	return ret_val;

}
/*******************************************************************************
 * set_freq - set the performance number for cpu.
 * @cpu - The cpu to set.
 * @freq_ind - the performance number we need to set `cpu` to.
 *
 * changes the frequency of `cpu` to one indicated by the performance number
 * freq_ind. 
 * NOTE: Do not call this from interrupt context! This function _might_ sleep.
 *******************************************************************************/
int set_freq(unsigned int cpu, unsigned int freq_ind)
{
	return internal_set_freq(cpu,freq_ind,DRIVER_TARGET_LOCKING);
}

EXPORT_SYMBOL_GPL(set_freq);

/*******************************************************************************
 * __set_freq - set the performance number for cpu. (Non locking)
 * @cpu - The cpu to set.
 * @freq_ind - the performance number we need to set `cpu` to.
 *
 * changes the frequency of `cpu` to one indicated by the performance number
 * freq_ind. 
 * NOTE: Do not call this from interrupt context! This function _might_ sleep.
 * but it promises that no locks will be held.
 *******************************************************************************/
int __set_freq(unsigned int cpu, unsigned int freq_ind)
{
	return internal_set_freq(cpu,freq_ind,DRIVER_TARGET_NONLOCKING);
}
EXPORT_SYMBOL_GPL(__set_freq);

/*******************************************************************************
 * inc_freq - Increment the performnace number of cpu.
 * @cpu - The cpu for which the performance number has to change.
 *
 * Increment the performance number (Increase frequency) if possible.
 * NOTE: Do not call this from interrupt context! This function _might_ sleep.
 *******************************************************************************/
int inc_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cur_freq == FREQ_INFO(cpu)->num_states - 1)
		return 0;
	return set_freq(cpu, FREQ_INFO(cpu)->cur_freq + 1);
}

EXPORT_SYMBOL_GPL(inc_freq);

/*******************************************************************************
 * dec_freq - Decrement the performnace number of cpu.
 * @cpu - The cpu for which the performance number has to change.
 *
 * Decrement the performance number (Decrease frequency) if possible.
 * NOTE: Do not call this from interrupt context! This function _might_ sleep.
 *******************************************************************************/
int dec_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cur_freq == 0)
		return 0;
	return set_freq(cpu, FREQ_INFO(cpu)->cur_freq - 1);
}

EXPORT_SYMBOL_GPL(dec_freq);

/*******************************************************************************
 * get_max_states - Get the max performance states for cpu.
 * @cpu - the cpu for which the value is required.
 * @return - The total performance states supported by cpu.
 *
 * Return the max number of performance states supported by cpu=`cpu` on 
 * this system.
 *******************************************************************************/
int get_max_states(int cpu)
{
	return FREQ_INFO(cpu)->num_states;
}

EXPORT_SYMBOL_GPL(get_max_states);


/*******************************************************************************
 * seeker_cpufreq_init - Init function for seeker_cpufreq
 * @return  - 0 if success, error code otherwise.
 * @Side Effects - freq_info is populated by reading from cpufreq_table.
 *
 * Initialize seeker_cpufreq's data structures and register self as a governor.
 *******************************************************************************/
static int __init seeker_cpufreq_init(void)
{
	int i, j, k, l;
	unsigned int tmp;
	struct cpufreq_frequency_table *table;
	int cpus = num_online_cpus();

	cpufreq_register_governor(&seeker_governor);

	init_user_interface();

	for (i = 0; i < cpus; i++) {
		FREQ_INFO(i)->cpu = i;
		FREQ_INFO(i)->cur_freq = -1;	/* Not known */
		FREQ_INFO(i)->num_states = 0;
		table = cpufreq_frequency_get_table(i);
		for (j = 0; table[j].frequency != CPUFREQ_TABLE_END; j++) {
			if (table[j].frequency == CPUFREQ_ENTRY_INVALID)
				continue;
			FREQ_INFO(i)->num_states++;
			FREQ_INFO(i)->table[j] = table[j].frequency;
			FREQ_INFO(i)->valid_entry[j] = 1;
		}
		for(j=FREQ_INFO(i)->num_states;j<MAX_STATES;j++){
			FREQ_INFO(i)->valid_entry[j] = 0;
		}
		/* Sort Table. This is a one time thing, and hence, I am doing a simple bubble sort.
		 * Nothing fancy */
		for (k = 0; k < FREQ_INFO(i)->num_states - 1; k++) {
			for (l = 0; l < FREQ_INFO(i)->num_states - 1; l++) {
				if (FREQ_INFO(i)->table[l] >
				    FREQ_INFO(i)->table[l + 1]) {
					tmp = FREQ_INFO(i)->table[l];
					FREQ_INFO(i)->table[l] =
					    FREQ_INFO(i)->table[l + 1];
					FREQ_INFO(i)->table[l + 1] = tmp;
				}
			}
		}
		if(allowed_states_length){

			/* validate number of entries */
			if(allowed_states_length < 1 || 
					allowed_states_length > FREQ_INFO(i)->num_states){
				goto err_fatal;
			}

			/* validate entries and mark them */
			for(j=0;j<MAX_STATES && j < FREQ_INFO(i)->num_states; j++){
				if(allowed_states[j] < 0 || 
				   allowed_states[j] >= FREQ_INFO(i)->num_states){
					error("Entry %d = %d is not a valid state, must be in [0,%d)",
						j,allowed_states[j],FREQ_INFO(i)->num_states);
						
					goto err_fatal;
				}
				FREQ_INFO(i)->valid_entry[allowed_states[j]] = 2;
			}
			for(j=0;j<allowed_states_length;j++){
				int k;
				if(FREQ_INFO(i)->valid_entry[j] == 2){
					continue;
				}
				/* shift left */
				for(k=j;k<MAX_STATES-1;k++){
					FREQ_INFO(i)->valid_entry[k] = FREQ_INFO(i)->valid_entry[k+1];
					FREQ_INFO(i)->table[k] = FREQ_INFO(i)->table[k+1];
				}
				j--;
				FREQ_INFO(i)->num_states--;
			}
		}
		/* Print to user */
		printk(KERN_INFO "FREQUENCY(%d)",i);
		for(j=0;j<FREQ_INFO(i)->num_states;j++){
			printk(" %d",FREQ_INFO(i)->table[j]);
		}
		printk("\n");
	}

	return 0;
err_fatal:
	cpufreq_unregister_governor(&seeker_governor);
	return -ENODEV;
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


