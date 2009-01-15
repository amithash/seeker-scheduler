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
#include <linux/cpuidle.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/moduleparam.h>

#include <seeker.h>

#include "seeker_cpuidle.h"

struct cpu_state_t{
	int cpu;
	int enabled;
	int valid;
	struct cpuidle_device *dev;
};

static struct cpu_state_t cpu_state[NR_CPUS];

static int seeker_cpuidle_enable(struct cpuidle_device *dev)
{
	if(cpu_state[dev->cpu].valid == 0)
		return -1;

	cpu_state[dev->cpu].dev = dev;
	cpu_state[dev->cpu].enabled = 1;
	return 0;
}

static void seeker_cpuidle_disable(struct cpuidle_device *dev)
{
	if(cpu_state[dev->cpu].valid == 0)
		return;

	cpu_state[dev->cpu].enabled = 0;
}

static int seeker_cpuidle_select(struct cpuidle_device *dev)
{
	return 0;
}

struct cpuidle_governor seeker_governor = {
	.name = "seeker",
	.owner = THIS_MODULE,
	.rating = 1000,
	.enable = &seeker_cpuidle_enable,
	.disable = &seeker_cpuidle_disable,
	.select = &seeker_cpuidle_select,
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpuidle driver");


static int __init seeker_cpuidle_init(void)
{
	int i;
	int ret;
	int total_cpus = num_online_cpus();
	for(i=0;i<total_cpus;i++){
		cpu_state[i].cpu = i;
		cpu_state[i].valid = 1;
		cpu_state[i].enabled = 0;
	}
	for(i=total_cpus;i<NR_CPUS;i++){
		cpu_state[i].cpu = i;
		cpu_state[i].valid = 0;
		cpu_state[i].enabled = 0;
	}

	ret = cpuidle_register_governor(&seeker_governor);
	if(!ret){
		info("Switching to seeker as a governor");
		cpuidle_switch_governor(&seeker_governor);
	}
	return ret;
}

static void __exit seeker_cpuidle_exit(void)
{
	cpuidle_unregister_governor(&seeker_governor);
}

module_init(seeker_cpuidle_init);
module_exit(seeker_cpuidle_exit);

