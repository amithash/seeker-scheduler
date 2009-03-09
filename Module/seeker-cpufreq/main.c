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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/moduleparam.h>

#include <seeker.h>

#include "seeker_cpufreq.h"

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


struct scpufreq_user_list{
	struct scpufreq_user *user;
	struct scpufreq_user_list *next;
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

/* List of registered users */
struct scpufreq_user_list *user_head = NULL;

/* Lock to be held whenever accessing user_head. */
static DEFINE_SPINLOCK(user_lock);

/* The counter to give a unique un-used id 4B ids can be provided.*/
static unsigned int current_highest_id = 0;

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
 * create_user - creates a new user in the user list.
 * @u - The user struct.
 * @return - 0 on success, error code on failure
 *
 * Create a new user node and link into the users list. 
 *******************************************************************************/
static int create_user(struct scpufreq_user *u)
{
	struct scpufreq_user_list *i;
	if(!u)
		return ERR_INV_USER;
	if(!u->inform)
		return ERR_INV_CALLBACK;
	spin_lock(&user_lock);
	if(user_head){
		for(i=user_head;i->next;i=i->next);
		i->next = kmalloc(sizeof(struct scpufreq_user_list), GFP_KERNEL);
		i = i->next;
	} else {
		i = user_head = kmalloc(sizeof(struct scpufreq_user_list), GFP_KERNEL);
	}
	if(!i){
		spin_unlock(&user_lock);
		return ERR_USER_MEM_LOW;
	}
	i->next = NULL;
	i->user = u;
	i->user->user_id = current_highest_id++;
	spin_unlock(&user_lock);
	return 0;
}

/*******************************************************************************
 * destroy_user - Removes and deallocated a user from the users list.
 * @u - The user struct
 * @return - 0 on success, error code on failure.
 *
 * Search for a user in the list with id = u->user_id and if found, unlink 
 * from the list and de-allocate the memory.
 *******************************************************************************/
static int destroy_user(struct scpufreq_user *u)
{
	struct scpufreq_user_list *i,*j;
	if(!u)
		return ERR_INV_USER;
	if(!u->inform)
		return ERR_INV_CALLBACK;
	spin_lock(&user_lock);

	for(i=user_head,j=NULL;i && i->user->user_id != u->user_id ;j=i,i=i->next);

	if(!i){
		spin_unlock(&user_lock);
		return ERR_INV_USER;
	}
	if(!j)
		user_head = i->next;
	else
		j->next = i->next;
	spin_unlock(&user_lock);
	kfree(i);
	return 0;
}


/*******************************************************************************
 * inform_freq_change - Infrom all registered users of a freq change.
 * @cpu - The cpu for which freq is changing.
 * @state - The new state cpu will take.
 *
 * Call the inform callback for every registered users. 
 *******************************************************************************/
void inform_freq_change(int cpu, int state)
{
	struct scpufreq_user_list *i;
	int dummy_ret = 0;

	for(i=user_head;i;i=i->next){
		if(i->user->inform)
			dummy_ret |= i->user->inform(cpu,state);
	}
	if(dummy_ret){
		warn("Some of the users returned an error code which was lost");
	}
}

/*******************************************************************************
 * register_scpufreq_user - Register a new user of scpufreq.
 * @u - The user struct for the calling user.
 * 
 * Register user u with scpufreq. All notifications will be sent. 
 *******************************************************************************/
int register_scpufreq_user(struct scpufreq_user *u)
{
	return create_user(u);
}
EXPORT_SYMBOL_GPL(register_scpufreq_user);

/*******************************************************************************
 * deregister_scpufreq_user - Un-register a new user of scpufreq.
 * @u - The user struct for the calling user.
 * 
 * Un-register user u with scpufreq. All notifications will no longer be sent. 
 *******************************************************************************/
int deregister_scpufreq_user(struct scpufreq_user *u)
{
	return destroy_user(u);
}
EXPORT_SYMBOL_GPL(deregister_scpufreq_user);

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
	ret_val = cpufreq_driver_target(policy, policy->cur, CPUFREQ_RELATION_H);
	if(ret_val == -EAGAIN)
		ret_val = cpufreq_driver_target(policy,policy->cur,CPUFREQ_RELATION_H);
	if(ret_val)
		error("Target did not work for cpu %d transition to %d, with a return error code: %d",cpu,policy->cur,ret_val);
	else 
		info("Setting frequency of cpu %d to %d",cpu,policy->cur);

	inform_freq_change(cpu,freq_ind);

	return ret_val;
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
	int ret_val;
	struct cpufreq_policy *policy = NULL;
	if (unlikely(cpu >= NR_CPUS || freq_ind >= FREQ_INFO(cpu)->num_states))
		return -1;
	if (freq_ind == FREQ_INFO(cpu)->cur_freq)
		return 0;

	policy = FREQ_INFO(cpu)->policy;
	if (!policy) {
		error("Error, governor not initialized for cpu %d", cpu);
		return -1;
	}
	policy->cur = FREQ_INFO(cpu)->table[freq_ind];
	ret_val = __cpufreq_driver_target(policy, policy->cur, CPUFREQ_RELATION_H);
	if(ret_val == -EAGAIN)
		ret_val = __cpufreq_driver_target(policy,policy->cur,CPUFREQ_RELATION_H);
#ifdef DEBUG
	if(ret_val)
		debug("Target did not work for cpu %d transition to %d",cpu,policy->cur);
	else 
		debug("Setting frequency of cpu %d to %d",cpu,policy->cur);
#endif

	inform_freq_change(cpu,freq_ind);

	return ret_val;
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

	user_head = NULL;
	spin_lock_init(&user_lock);

	for (i = 0; i < cpus; i++) {
		FREQ_INFO(i)->cpu = i;
		FREQ_INFO(i)->cur_freq = -1;	/* Not known */
		FREQ_INFO(i)->num_states = 0;
		table = cpufreq_frequency_get_table(i);
		printk("CPU %d\nFrequencies:\n", i);
		for (j = 0; table[j].frequency != CPUFREQ_TABLE_END; j++) {
			if (table[j].frequency == CPUFREQ_ENTRY_INVALID)
				continue;
			FREQ_INFO(i)->num_states++;
			FREQ_INFO(i)->table[j] = table[j].frequency;
			FREQ_INFO(i)->valid_entry[j] = 1;
			printk("%d ", table[j].frequency);
		}
		printk("\n");
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
			for(j=0;j<MAX_STATES && j < FREQ_INFO(i)->num_states; j++){
				if(allowed_states[j] < 0 || 
				   allowed_states[j] >= FREQ_INFO(i)->num_states){
					error("Entry %d = %d is not a valid state, must be in [0,%d)",
						j,allowed_states[j],FREQ_INFO(i)->num_states);
						
					continue;
				}
				FREQ_INFO(i)->valid_entry[allowed_states[j]] = 2;
			}
			for(j=0;j<MAX_STATES-1;j++){
				if(FREQ_INFO(i)->valid_entry[j] != 2){
					FREQ_INFO(i)->valid_entry[j] = FREQ_INFO(i)->valid_entry[j+1];
					FREQ_INFO(i)->table[j] = FREQ_INFO(i)->table[j+1];
					j--;
					FREQ_INFO(i)->num_states--;
				}
			}
		}
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
	struct scpufreq_user_list *i,*j;

	spin_lock(&user_lock);
	for(i=user_head;i;){
		j = i->next;
		kfree(i);
		i = j;
	}
	user_head = NULL;
	spin_unlock(&user_lock);
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


