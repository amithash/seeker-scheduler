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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>
#include <linux/percpu.h>

#include <seeker.h>

#include "fpmu_int.h"

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* the control register discreption for each cpu */
fixctrl_t fcontrol[NR_CPUS] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/* The counter values for each cpu and counter */
fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{
#if NUM_FIXED_COUNTERS > 0
	 {0, 0, MSR_PERF_FIXED_CTR0}	/* 0 */
#endif
#if NUM_FIXED_COUNTERS > 1
	 , {0, 0, MSR_PERF_FIXED_CTR1}	/* 1 */
#endif
#if NUM_FIXED_COUNTERS > 2
	 , {0, 0, MSR_PERF_FIXED_CTR2}	/* 2 */
#endif
	 }
};

/* The `cleared` state for each fixed counter */
fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS] = {
	{
#if NUM_FIXED_COUNTERS > 0
	 {0, 0, 0}
#endif
#if NUM_FIXED_COUNTERS > 1
	 , {0, 0, 0}
#endif
#if NUM_FIXED_COUNTERS > 2
	 , {0, 0, 0}
#endif
	 }
};

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/



/*******************************************************************************
 * fpmu_init_msrs - initialize msrs on _this_ cpu.
 * @info - Not used.
 *
 * Initialize (clear and set to default values) the Fixed PMU MSR's.
 * Typical use is to call this using smp_call_function_single or on_each_cpu.
 *******************************************************************************/
void fpmu_init_msrs(void *info)
{
#if NUM_FIXED_COUNTERS > 0
	int i;
	int cpu_id = get_cpu();
	if (likely(cpu_id < NR_CPUS)) {
		for (i = 0; i < NUM_FIXED_COUNTERS; i++) {
			if (cpu_id != 0) {
				fcounters[cpu_id][i] = fcounters[0][i];
				fcleared[cpu_id][i] = fcleared[0][i];
			}
			fcounter_clear(i);
		}
		if (cpu_id != 0) {
			fcontrol[cpu_id] = fcontrol[0];
		}
		control_clear();
		fcounters_disable();
	}
	put_cpu();
#endif
}

EXPORT_SYMBOL_GPL(fpmu_init_msrs);

/*******************************************************************************
 * fpmu_init - The init function for fpmu.
 * @Side Effects - Calls fpmu_init_msrs on each cpu and hence initializing 
 * 		   them on _all_ cpus.
 *
 * Initialize fpmu.
 *******************************************************************************/
static int __init fpmu_init(void)
{
#if NUM_FIXED_COUNTERS > 0
	if (ON_EACH_CPU(&fpmu_init_msrs, NULL, 1, 1) < 0) {
		error("Could not enable anything, panicing and exiting");
		return -ENODEV;
	}
#endif
	return 0;
}

/*******************************************************************************
 * fpmu_exit - exit function for fpmu.
 * @Side Effects - Clears any counters being used. 
 *
 * Does nothing. but we need an exit function.
 *******************************************************************************/
static void __exit fpmu_exit(void)
{
	;
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(fpmu_init);
module_exit(fpmu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the "
		   "fixed performance monitoring counters");

