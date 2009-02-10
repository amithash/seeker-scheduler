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
MODULE_DESCRIPTION("Provides a method to set the current speed");

int freqs[NR_CPUS] = {0};
int freqs_length = -1;

static int __init setspeed_init(void)
{
	int i;
	int cpus = num_online_cpus();
	int total_states;
	int ret = 0;
	for(i=0;i<freqs_length && i < cpus;i++){
		total_states = get_max_states(i);
		if(freqs[i] < 0)
			freqs[i] = 0;
		if(freqs[i] >= total_states)
			freqs[i] = total_states-1;

		ret |= set_freq(i,freqs[i]);
	}
	for(i=freqs_length;i<cpus;i++)
		ret |= set_freq(i,0);
	return ret;
}

static void __exit setspeed_exit(void)
{
	;
}

module_param_array(freqs, int, &freqs_length, 0444);
MODULE_PARM_DESC(freqs,
		 "Optional, sets the cpus with the current freq_index: 0,1,... "
		 "Num states in increasing frequencies");

module_init(setspeed_init);
module_exit(setspeed_exit);

