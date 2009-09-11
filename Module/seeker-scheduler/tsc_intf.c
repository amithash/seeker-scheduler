/*****************************************************
 * Copyright 2009 Amithash Prasad                    *
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

#include <seeker.h>
#include "tsc_intf.h"

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* contains the previous value of tsc for each cpu */
unsigned long long tsc_val[NR_CPUS] = { 0 };

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * get_tsc_cycles - get cycles from last time,
 * @Return Cycles executed on this cpu from the last call.
 * @Side Effect - tsc_val is updated for the current cpu.
 *
 * Get the cycles executed on this cpu from the last call.
 ********************************************************************************/
unsigned long long get_tsc_cycles(void)
{
	int cpu = get_cpu();
	unsigned long long ret = 0;
	unsigned long long val = native_read_tsc();
	ret = val - tsc_val[cpu];
	tsc_val[cpu] = val;
	put_cpu();
	return ret;
}

/********************************************************************************
 * init_tsc - initialize tsc_val for this cpu.
 * @info - not used
 * @Side Effect - tsc_val is updated
 *
 * initialize tsc_val for the current CPU.
 ********************************************************************************/
static void init_tsc(void *info)
{
	tsc_val[smp_processor_id()] = native_read_tsc();
}

/********************************************************************************
 * init_tsc_intf - Initialize tsc_intf
 * @Return - Error code, 0 for success
 * @Side Effect - tsc_val is updated for the current value for each cpu.
 *
 * Initialize tsc_val for each online cpu, to avoid an errornous first reading.
 ********************************************************************************/
int init_tsc_intf(void)
{
	if (ON_EACH_CPU(init_tsc, NULL, 1, 1) < 0) {
		error("Cound not initialize tsc_intf.");
		return -1;
	}
	return 0;
}
