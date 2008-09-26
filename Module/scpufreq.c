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
#include <asm/types.h>
#include <linux/moduleparam.h>
#include "seeker.h"
#include "scpufreq.h"

unsigned int freqs[NR_CPUS];
unsigned int freq_count = -1;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");

struct freq_info_t freq_info[NR_CPUS];

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
	struct cpufreq_policy *policy;
	if(unlikely(cpu >= NR_CPUS))
		return -1;
	if(freq_ind == freq_info[cpu].cur_freq)
		return 0;
	if(freq_ind < freq_info[cpu].num_states){
		policy = cpufreq_cpu_get(cpu);
		policy->min = freq_info[cpu].table[freq_ind];
		policy->max = freq_info[cpu].table[freq_ind];
		policy->cpus= cpumask_of_cpu(cpu);
		cpufreq_cpu_put(policy);
		freq_info[cpu].cur_freq = freq_ind;
		return 0;
	}
	return -1;
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




static int __init seeker_cpufreq_init(void)
{
	int i,j,k,l;
	unsigned int tmp;
	struct cpufreq_frequency_table *table;
	int cpus = num_online_cpus();
	for(i=0;i<cpus;i++){
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
	for(i=cpus;i<NR_CPUS;i++){
		freq_info[i].num_states = 0;
		freq_info[i].cpu = -1;
		freq_info[i].cur_freq = -1;
	}
	/* From now on frequency refered by the index from freq_info. */

	if(freq_count != -1){
		if(freq_count > cpus){
			warn("There are only %d cpus online. Ignoring the rest.",cpus);
			freq_count = cpus;
		}
		for(i=0;i<freq_count;i++){
			if(set_freq(i,freqs[i]))
				warn("Param for cpu %d = %d is not valid (avaliable=0,...%d). "
					"CPU speed is left unchanged.\n",i,freq_info[i].num_states-1,freqs[i]);
		}
	}

	return 0;
}

static void __exit seeker_cpufreq_exit(void)
{
	/* Revert changes to cpufreq to make it useable by
	 * user process again */
	;
}

module_param_array(freqs,int, &freq_count, 0444);
MODULE_PARM_DESC(freqs, "Optional, sets the cpus with the current freq_index: 0,1,... Num states in increasing frequencies");

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);
