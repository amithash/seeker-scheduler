/******************************************************************************\
 * FILE: interface.c
 * DESCRIPTION: This file implements functions which take care of registering
 * with cpufreq such that communication is enabled for functions implemented
 * in freq.c
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

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * cpufreq_seeker_setspeed - governor interface to set speed.
 * @policy - the cpufreq policy for which the speed is to be set.
 * @freq - the freq to be set. 
 * @return success.
 *
 * governor userspace interface informs a speed to be set and this function
 * will be called with the cpu's profile as a parameter.
 *******************************************************************************/
static int cpufreq_seeker_setspeed(struct cpufreq_policy *policy,
				   unsigned int freq)
{
	debug("User wants speed to be %u on cpu %d", freq, policy->cpu);
	return __set_freq(policy->cpu, freq);
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
static ssize_t cpufreq_seeker_showspeed(struct cpufreq_policy *policy,
					char *buf)
{
	debug("Someone called showspeed... so let's show them something");
	sprintf(buf, "%d", policy->cur);
	return (strlen(buf) + 1) * sizeof(char);
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
		gov_init_freq_info_cpu(cpu, policy);
		info("Latency for cpu %d = %d nanoseconds", cpu,
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

void register_self(void)
{
	cpufreq_register_governor(&seeker_governor);
}

void deregister_self(void)
{
	cpufreq_unregister_governor(&seeker_governor);
}
