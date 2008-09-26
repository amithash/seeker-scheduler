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
#include "scpufreq.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");

// struct cpufreq_policy {
//        cpumask_t               cpus;   /* affected CPUs */
//        unsigned int            shared_type; /* ANY or ALL affected CPUs
//                                                should set cpufreq */
//        unsigned int            cpu;    /* cpu nr of registered CPU */
//        struct cpufreq_cpuinfo  cpuinfo;/* see above */
//
//        unsigned int            min;    /* in kHz */
//        unsigned int            max;    /* in kHz */
//        unsigned int            cur;    /* in kHz, only needed if cpufreq
//                                         * governors are used */
//        unsigned int            policy; /* see above */
//        struct cpufreq_governor *governor; /* see below */
//
//        struct work_struct      update; /* if update_policy() needs to be
//                                         * called, but you're in IRQ context */
//
//        struct cpufreq_real_policy      user_policy;
//
//        struct kobject          kobj;
//        struct completion       kobj_unregister;
//};

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

	return 0;
}

static void __exit seeker_cpufreq_exit(void)
{
	/* Revert changes to cpufreq to make it useable by
	 * user process again */
	;
}

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);
