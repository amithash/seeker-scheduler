/******************************************************************************\
 * FILE: freq.c
 * DESCRIPTION: This file contains function and implementation in changing
 * the frequency of a cpu. It does all the communication with cpufreq-driver
 * via the generic cpufreq layer. 
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include <linux/cpufreq.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "user.h"

#define DRIVER_TARGET_LOCKING 1
#define DRIVER_TARGET_NONLOCKING 0

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
 * 				Global Datastructures 				*
 ********************************************************************************/

/* Have a per-cpu information of freq_info. */
static DEFINE_PER_CPU(struct freq_info_t, freq_info);

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* parameter for user to specify states allowed */
extern int allowed_states[MAX_STATES];

/* Length of the allowed_states array */
extern int allowed_states_length;

/********************************************************************************
 * 				Macros						*
 ********************************************************************************/

/* Macro to make the access of the per-cpu structure easy */
#define FREQ_INFO(cpu) (&per_cpu(freq_info,(cpu)))

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
	if (locking == DRIVER_TARGET_NONLOCKING)
		return __cpufreq_driver_target(policy, frequency, relation);

	return cpufreq_driver_target(policy, frequency, relation);
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
static int internal_set_freq(unsigned int cpu, unsigned int freq_ind,
			     int locking)
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
	ret_val =
	    driver_target(policy, policy->cur, CPUFREQ_RELATION_H, locking);
	if (ret_val == -EAGAIN)
		ret_val =
		    driver_target(policy, policy->cur, CPUFREQ_RELATION_H,
				  locking);
	if (ret_val) {
		error("Target did not work for cpu %d transition to %d, "
		      "with a return error code: %d", cpu, policy->cur,
		      ret_val);
		return ret_val;
	}
	debug("Setting frequency of %d to %d", cpu, policy->cur);
	inform_freq_change(cpu, freq_ind);

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
	return internal_set_freq(cpu, freq_ind, DRIVER_TARGET_LOCKING);
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
	return internal_set_freq(cpu, freq_ind, DRIVER_TARGET_NONLOCKING);
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

void gov_init_freq_info_cpu(int cpu, struct cpufreq_policy *policy)
{
	FREQ_INFO(cpu)->policy = policy;
	FREQ_INFO(cpu)->latency = policy->cpuinfo.transition_latency;
}

void gov_exit_freq_info_cpu(int cpu)
{
	FREQ_INFO(cpu)->policy = NULL;
}

void sort_table(unsigned int *table, int len)
{
	unsigned int tmp;
	int i, j;

	for (i = 0; i < len - 1; i++) {
		for (j = 0; j < len - 1; j++) {
			if (table[j] <= table[j + 1])
				continue;

			tmp = table[j];
			table[j] = table[j + 1];
			table[j + 1] = tmp;
		}
	}
}

void prune_states(int cpu)
{
	int i, j;
	unsigned int *table = FREQ_INFO(cpu)->table;
	unsigned int *valid = FREQ_INFO(cpu)->valid_entry;
	unsigned int num_states = FREQ_INFO(cpu)->num_states;

	/* validate entries and mark them */
	for (i = 0; i < num_states; i++) {
		if (allowed_states[i] < 0) {
			warn("invalid state: %d. Using 0", allowed_states[i]);
			allowed_states[i] = 0;
		}

		if (allowed_states[i] >= num_states) {
			warn("Invalid state: %d, Using %d", allowed_states[i],
			     num_states - 1);

			allowed_states[i] = num_states - 1;
		}

		valid[allowed_states[i]] = 2;
	}

	for (i = 0; i < allowed_states_length; i++) {
		if (valid[i] == 2) {
			continue;
		}
		for (j = i; j < MAX_STATES - 1; j++) {
			valid[j] = valid[j + 1];
			table[j] = table[j + 1];
		}
		i--;
		num_states--;
	}
	FREQ_INFO(cpu)->num_states = num_states;
}

int init_freqs(void)
{
	int i, j;
	struct cpufreq_frequency_table *table;
	int cpus = num_online_cpus();
	int ind;

	for (i = 0; i < cpus; i++) {
		FREQ_INFO(i)->cpu = i;
		FREQ_INFO(i)->cur_freq = -1;	/* Not known */
		FREQ_INFO(i)->num_states = 0;
		table = cpufreq_frequency_get_table(i);
		for (j = 0; table[j].frequency != CPUFREQ_TABLE_END; j++) {
			ind = FREQ_INFO(i)->num_states;

			if (table[j].frequency == CPUFREQ_ENTRY_INVALID)
				continue;

			if (ind >= MAX_STATES) {
				warn("Reached max supported states: %d, "
				     "Recompile with larger"
				     "Value for MAX_STATES in "
				     "seeker_cpufreq.h", MAX_STATES);
				break;
			}
			FREQ_INFO(i)->table[ind] = table[j].frequency;
			FREQ_INFO(i)->valid_entry[ind] = 1;
			FREQ_INFO(i)->num_states++;
		}
		for (j = FREQ_INFO(i)->num_states; j < MAX_STATES; j++) {
			FREQ_INFO(i)->valid_entry[j] = 0;
		}

		sort_table(FREQ_INFO(i)->table, FREQ_INFO(i)->num_states);

		if (allowed_states_length > FREQ_INFO(i)->num_states) {
			warn("Only %d states are supported for cpu %d"
			     " rest will be truncated.",
			     FREQ_INFO(i)->num_states, i);

			allowed_states_length = FREQ_INFO(i)->num_states;
		}

		if (allowed_states_length > 0)
			prune_states(i);

		/* Print to user */
		printk(KERN_INFO "FREQUENCY(%d)", i);
		for (j = 0; j < FREQ_INFO(i)->num_states; j++) {
			printk(" %d", FREQ_INFO(i)->table[j]);
		}
		printk("\n");
	}

	return 0;
}
