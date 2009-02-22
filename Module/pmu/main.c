
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

#include <seeker.h>

#include "pmu_int.h"



/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/


/* evtsel defined per counter per cpu. the structures are
 * different for K8 and K10 so initialized using #ifdefs. 
 */
#if defined(ARCH_K8) || defined(ARCH_K10)
evtsel_t evtsel[NR_CPUS][NUM_COUNTERS] = {
	{
#if NUM_COUNTERS > 0
	 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0, 0, EVTSEL0}	/*0 */
#endif
#if NUM_COUNTERS > 1
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0, 0, EVTSEL1}	/*1 */
#endif
#if NUM_COUNTERS > 2
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0, 0, EVTSEL2}	/*2 */
#endif
#if NUM_COUNTERS > 3
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0, 0, EVTSEL3}	/*3 */
#endif
	 }
};

#else

evtsel_t evtsel[NR_CPUS][NUM_COUNTERS] = {
	{
#if NUM_COUNTERS > 0
	 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, EVTSEL0}	/*0 */
#endif
#if NUM_COUNTERS > 1
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, EVTSEL1}	/*1 */
#endif
#if NUM_COUNTERS > 2
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, EVTSEL2}	/*2 */
#endif
#if NUM_COUNTERS > 3
	 , {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, EVTSEL3}	/*3 */
#endif
	 }
};
#endif

/* contains a per counter, per cpu counter information */
counter_t counters[NR_CPUS][NUM_COUNTERS] = {
	{

#if NUM_COUNTERS > 0
	 {0, 0, PMC0, 0, 0, 0}	/* 0 */
#endif
#if NUM_COUNTERS > 1
	 , {0, 0, PMC1, 1, 0, 0}	/* 1 */
#endif
#if NUM_COUNTERS > 2
	 , {0, 0, PMC2, 2, 0, 0}	/* 2 */
#endif
#if NUM_COUNTERS > 3
	 , {0, 0, PMC3, 3, 0, 0}	/* 3 */
#endif
	 }
};

/* Contains the value to be stored in the counters upon a counter clear */
cleared_t cleared[NR_CPUS][NUM_COUNTERS] = {
	{
#if NUM_COUNTERS > 0
	 {0, 0, 0}
#endif
#if NUM_COUNTERS > 1
	 , {0, 0, 0}
#endif
#if NUM_COUNTERS > 2
	 , {0, 0, 0}
#endif
#if NUM_COUNTERS > 3
	 , {0, 0, 0}
#endif
	 }
};

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/


/*******************************************************************************
 * pmu_init_msrs - initialize msrs on _this_ cpu.
 * @info - Not used.
 *
 * Initialize (clear and set to default values) the PMU MSR's.
 *******************************************************************************/
void pmu_init_msrs(void *info)
{
#if NUM_COUNTERS > 0
	int i;
	int cpu = get_cpu();
	if (cpu != 0) {
		for (i = 0; i < NUM_COUNTERS; i++) {
			counters[cpu][i] = counters[0][i];
			evtsel[cpu][i] = evtsel[0][i];
		}
	}
	for (i = 0; i < NUM_COUNTERS; i++) {
		warn("cpu %d counter %d enabled %d", cpu, i,
		     counters[cpu][i].enabled);
		evtsel_clear(i);
		counter_disable(i);
		counter_clear(i);
	}
	put_cpu();
#endif
}
EXPORT_SYMBOL_GPL(pmu_init_msrs);

/*******************************************************************************
 * pmu_init - The init function for pmu.
 * @Side Effects - Calls pmu_init_msrs on each cpu and hence initializing 
 * 		   them on _all_ cpus.
 *
 * Initialize pmu.
 *******************************************************************************/
static int __init pmu_init(void)
{
	if (ON_EACH_CPU(&pmu_init_msrs, NULL, 1, 1) < 0) {
		error("Could not enable all counters. Panicing and exiting");
		return -ENODEV;
	}
	return 0;
}

/*******************************************************************************
 * pmu_exit - exit function for pmu.
 * @Side Effects - Clears any counters being used. 
 *******************************************************************************/
static void __exit pmu_exit(void)
{
	int i, j;
	for (i = 0; i < NR_CPUS; i++) {
		for (j = 0; j < NUM_COUNTERS; j++) {
			counters[i][j].enabled = 0;
		}
	}
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(pmu_init);
module_exit(pmu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the PMU");

