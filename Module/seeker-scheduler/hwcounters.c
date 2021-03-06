/******************************************************************************\
 * FILE: hwcounters.c
 * DESCRIPTION: 
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <seeker.h>
#include <pmu.h>

#include "hwcounters.h"

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

#if NUM_FIXED_COUNTERS == 0
/* contain the counter numbers for inst, recy and rfcy for each cpu */
int sys_counters[NR_CPUS][2] = { {0, 0} };
#endif

/* Value of inst re_cy and rf_cy for each cpu */
u64 pmu_val[NR_CPUS][3];

/* Flag Indicates an error in the procedure */
int ERROR = 0;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * enable_cpu_counters - enable counters on current cpu
 * @info - Not used
 * @Side Effect - If PMU is used, store the counter numbers used.
 *
 * FPMU or PMU counters are enabled to count inst, recy and rfcy
 ********************************************************************************/
void enable_pmu_counters(void *info)
{
	int cpu = get_cpu();
#if NUM_FIXED_COUNTERS > 0
	fcounters_enable(0);
#else
	if ((sys_counters[cpu][0] =
	     counter_enable(PMU_INST_EVTSEL, PMU_INST_MASK, 0)) < 0) {
		error("Could not enable INST on %d", cpu);
		sys_counters[cpu][0] = 0;
		ERROR = 1;
	}
	if ((sys_counters[cpu][1] =
	     counter_enable(PMU_RECY_EVTSEL, PMU_RECY_MASK, 0)) < 0) {
		error("Could not enable RECY on cpu %d", cpu);
		sys_counters[cpu][1] = 1;
		ERROR = 1;
	}
#endif
	clear_counters(cpu);
	put_cpu();
}

/********************************************************************************
 * disable_pmu_coounters - disable all used counters on current cpu.
 * @info - Not Used
 * @Side Effect - disables counters, and instructs pmu/fpmu to free them
 *
 * Disable and free counters to be used by another module. 
 ********************************************************************************/
void disable_pmu_counters(void *info)
{
#if NUM_FIXED_COUNTERS > 0
	fcounters_disable();
#else
	int cpu = get_cpu();
	counter_disable(sys_counters[cpu][0]);
	counter_disable(sys_counters[cpu][1]);
	put_cpu();
#endif
}

/********************************************************************************
 * configure_counters - enable counters on ALL online cpus
 * @Return - 0 for success -1 otherwise.
 * @Side Effect - calls enable_pmu_counters on each online cpu.
 *
 * Enable counters on all online cpus. 
 ********************************************************************************/
int configure_counters(void)
{
	if (ON_EACH_CPU(enable_pmu_counters, NULL, 1, 1) < 0) {
		error("Counters could not be configured");
		return -1;
	}
	if (ERROR)
		return -1;

	return 0;
}

/********************************************************************************
 * exit_counters - disable counters on ALL cpus.
 * @Side Effect - calls disable_pmu_counters on each cpu.
 *
 * Disables counters on all cpus. Must be called before exiting the module,
 * else re-use without reloading pmu/fpmu is not allowed. 
 ********************************************************************************/
void exit_counters(void)
{
	if (ON_EACH_CPU(disable_pmu_counters, NULL, 1, 1) < 0) {
		error("Counters could not be disabled");
	}
}

/********************************************************************************
 * read_counters - Read performance counters on this cpu.
 * @cpu - cpu number of this cpu
 * @Side Effect - Reads the performance counters and puts them in pmu_val for the
 * current cpu indicated by "cpu" and clear the counters.
 *
 * Read pmu counters into pmu_val
 ********************************************************************************/
void read_counters(int cpu)
{
#if NUM_FIXED_COUNTERS > 0
	fcounter_read();
	pmu_val[cpu][0] = get_fcounter_data(0, cpu);
	pmu_val[cpu][1] = get_fcounter_data(1, cpu);
#else
	counter_read();
	pmu_val[cpu][0] = get_counter_data(sys_counters[cpu][0], cpu);
	pmu_val[cpu][1] = get_counter_data(sys_counters[cpu][1], cpu);
#endif
}

void clear_counters(int cpu)
{
#if NUM_FIXED_COUNTERS > 0
	fcounter_clear(0);
	fcounter_clear(1);
#else
	counter_clear(sys_counters[cpu][0]);
	counter_clear(sys_counters[cpu][1]);
#endif
}
