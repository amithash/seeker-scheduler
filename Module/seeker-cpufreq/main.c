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

struct freq_info_t{
	unsigned int cpu;
	unsigned int cur_freq;
	unsigned int num_states;
	unsigned int table[MAX_STATES];
	struct cpufreq_policy *policy;
};

static int freqs = 0;

static int cpufreq_seeker_governor(struct cpufreq_policy *policy, unsigned int event)
{
	unsigned int cpu = policy->cpu;
	switch(event){
		case CPUFREQ_GOV_START:
			info("Starting governor on cpu %d",cpu);
			break;
		case CPUFREQ_GOV_STOP:
			info("Stopping governor on cpu %d",cpu);
			break;
		case CPUFREQ_GOV_LIMITS:
			info("Setting limits %d to %d for cpu %d",policy->min,policy->max,cpu);
			break;	
		default:
			info("Unknown");
			break;
	}
	return 0;
}

struct cpufreq_governor seeker_governor = {
	.name = "seeker",
	.owner = THIS_MODULE,
	.max_transition_latency = 1000000000,
	.governor = cpufreq_seeker_governor,
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");

struct freq_info_t freq_info[NR_CPUS];

struct cpufreq_policy *cpu_policy[NR_CPUS] = {NULL};

unsigned int get_freq(unsigned int cpu)
{
	if(freq_info[cpu].cpu >= 0){
		if(freq_info[cpu].cur_freq != -1)
			return freq_info[cpu].cur_freq;
	}
	return -1;
}
EXPORT_SYMBOL_GPL(get_freq);

int set_freq(unsigned int cpu, unsigned int freq_ind)
{
	struct cpufreq_policy *policy = NULL;
	if(unlikely(cpu >= NR_CPUS))
		return -1;
	if(freq_ind == freq_info[cpu].cur_freq)
		return 0;
	if(freq_ind >= freq_info[cpu].num_states)
		return -1;

	policy = freq_info[cpu].policy;
	if(!policy){
		error("Error, governor not initialized for cpu %d",cpu);
		return -1;
	}
	policy->cur = freq_info[cpu].table[freq_ind];
	/* Start a worker thread to do the actual update */
	freq_info[cpu].cur_freq = freq_ind;
	cpufreq_driver_target(policy,policy->cur,CPUFREQ_RELATION_H);
	return 0;
}
EXPORT_SYMBOL_GPL(set_freq);

int inc_freq(unsigned int cpu)
{
	if(freq_info[cpu].cur_freq == freq_info[cpu].num_states-1)
		return 0;
	return set_freq(cpu, freq_info[cpu].cur_freq+1);
}
EXPORT_SYMBOL_GPL(inc_freq);

int dec_freq(unsigned int cpu)
{
	if(freq_info[cpu].cur_freq == 0)
		return 0;
	return set_freq(cpu, freq_info[cpu].cur_freq-1);
}
EXPORT_SYMBOL_GPL(dec_freq);

int get_max_states(int cpu)
{
	return freq_info[cpu].num_states;
}
EXPORT_SYMBOL_GPL(get_max_states);

#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))
static int __init seeker_cpufreq_init(void)
{
	int i,j,k,l;
	unsigned int tmp;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *table;
	int cpus = num_online_cpus();
	cpufreq_register_governor(&seeker_governor);
	for(i=0;i<cpus;i++){
		policy = freq_info[i].policy = cpufreq_cpu_get(i);
		info("Related cpus for cpu%d are (bitmask) %d",i,CPUMASK_TO_UINT(policy->related_cpus));
		cpus_clear(policy->cpus);
		cpu_set(i,policy->cpus);
		//policy->update.func = &scpufreq_update_freq;
		policy->governor = &seeker_governor;
		cpufreq_cpu_put(policy);
		freq_info[i].cpu = i;
		freq_info[i].cur_freq = -1; /* Not known */
		freq_info[i].num_states = 0;
		table = cpufreq_frequency_get_table(i);
		printk("CPU %d\nFrequencies:\n",i);
		for(j=0;table[j].frequency != CPUFREQ_TABLE_END;j++){
			freq_info[i].num_states++;
			freq_info[i].table[j] = table[j].frequency;
			printk("%d ",table[j].frequency);
		}
		printk("\n");
		/* Sort Table */
		for(k=0;k<freq_info[i].num_states-1;k++){
			for(l=0;l<freq_info[i].num_states-1;l++){
				if(freq_info[i].table[l] > freq_info[i].table[l+1]){
					tmp = freq_info[i].table[l];
					freq_info[i].table[l] = freq_info[i].table[l+1];
					freq_info[i].table[l+1] = tmp;
				}
			}
		}
	}
	/* From now on frequency refered by the index from freq_info. */
	if(freqs < 0)
		freqs = 0;
	if(freqs >= freq_info[0].num_states)
		freqs = freq_info[0].num_states-1;

	for(i=0;i<cpus;i++){
		if(set_freq(i,freqs))
			warn("Param for cpu %d = %d is not valid (avaliable=0,...%d). "
				"CPU speed is left unchanged.\n",i,freq_info[i].num_states-1,freqs);
	}

	return 0;
}

static void __exit seeker_cpufreq_exit(void)
{
	/* Revert changes to cpufreq to make it useable by
	 * user process again */
	int i;
	struct cpufreq_policy *policy;
	int cpus = num_online_cpus();
	flush_scheduled_work();
	for(i=0;i<cpus;i++){
		policy = cpufreq_cpu_get(i);
		policy->governor = NULL;
		cpufreq_cpu_put(policy);
	}
	cpufreq_unregister_governor(&seeker_governor);
}

module_param(freqs,int, 0444);
MODULE_PARM_DESC(freqs, "Optional, sets the cpus with the current freq_index: 0,1,... Num states in increasing frequencies");

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);

