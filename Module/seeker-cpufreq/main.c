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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");

/* Per cpu structure to keep the information for each cpu */
struct freq_info_t {
	unsigned int cpu;
	unsigned int cur_freq;
	unsigned int num_states;
	unsigned int table[MAX_STATES];
	struct cpufreq_policy *policy;
};
static DEFINE_PER_CPU(struct freq_info_t, freq_info);
#define FREQ_INFO(cpu) (&per_cpu(freq_info,(cpu)))

static ssize_t cpufreq_seeker_showspeed(struct cpufreq_policy *policy, char *buf)
{
	debug("Someone called showspeed... so let's show them something");
	sprintf(buf,"%d",policy->cur);
	return (strlen(buf)+1)*sizeof(char);
}

static int cpufreq_seeker_governor(struct cpufreq_policy *policy,
				   unsigned int event);

/* The cpufreq governor structure for this module */
struct cpufreq_governor seeker_governor = {
	.name = "seeker",
	.owner = THIS_MODULE,
	.max_transition_latency = CPUFREQ_ETERNAL,
	.governor = cpufreq_seeker_governor,
	.show_setspeed = cpufreq_seeker_showspeed,
};
/* the governor function. It just prints infomation when it is called */
static int cpufreq_seeker_governor(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	switch (event) {
	case CPUFREQ_GOV_START:
		info("Starting governor on cpu %d", cpu);
		FREQ_INFO(cpu)->policy = policy;
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

/* Users can get the current freq of a cpu */
unsigned int get_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cpu >= 0) {
		if (FREQ_INFO(cpu)->cur_freq != -1)
			return FREQ_INFO(cpu)->cur_freq;
	}
	return -1;
}

EXPORT_SYMBOL_GPL(get_freq);

/* Users can set the freq of a cpu. 
 * Do NOT call this from within interrupt context 
 */
int set_freq(unsigned int cpu, unsigned int freq_ind)
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
	ret_val = cpufreq_driver_target(policy, policy->cur, CPUFREQ_RELATION_H);
	if(ret_val == -EAGAIN)
		ret_val = cpufreq_driver_target(policy,policy->cur,CPUFREQ_RELATION_H);
	if(ret_val)
		error("Target did not work for cpu %d transition to %d, with a return error code: %d",cpu,policy->cur,ret_val);
	else 
		info("Setting frequency of cpu %d to %d",cpu,policy->cur);

	return ret_val;
}

EXPORT_SYMBOL_GPL(set_freq);

/* Increment the freq. Users need not worry on number of states. */
int inc_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cur_freq == FREQ_INFO(cpu)->num_states - 1)
		return 0;
	return set_freq(cpu, FREQ_INFO(cpu)->cur_freq + 1);
}

EXPORT_SYMBOL_GPL(inc_freq);

/* Decrement the freq. Users need not worry on going below 0 */
int dec_freq(unsigned int cpu)
{
	if (FREQ_INFO(cpu)->cur_freq == 0)
		return 0;
	return set_freq(cpu, FREQ_INFO(cpu)->cur_freq - 1);
}

EXPORT_SYMBOL_GPL(dec_freq);

/* Get the max states supported by cpu */
int get_max_states(int cpu)
{
	return FREQ_INFO(cpu)->num_states;
}

EXPORT_SYMBOL_GPL(get_max_states);

/* Macro used to convert cpumask to an unsigned integer.
 * so that it can be printed
 */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

/* init */
static int __init seeker_cpufreq_init(void)
{
	int i, j, k, l;
	unsigned int tmp;
	struct cpufreq_frequency_table *table;
	int cpus = num_online_cpus();
	cpufreq_register_governor(&seeker_governor);
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
			printk("%d ", table[j].frequency);
		}
		printk("\n");
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
	}
	return 0;
}

/* Exit */
static void __exit seeker_cpufreq_exit(void)
{
	cpufreq_unregister_governor(&seeker_governor);
}

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);

