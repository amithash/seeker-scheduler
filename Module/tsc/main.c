
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

#include "tsc_int.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the "
		   "Time stamp counter");

tstamp_t time_stamp[NR_CPUS] = {
	{0, 0, 0, 0}
};

void read_time_stamp(void)
{
	u32 low, high;
	int cpu_id = smp_processor_id();
	if (likely(cpu_id < NR_CPUS)) {
		// rdtsc is no longer supported by the linux kernel.
		rdmsr(TIME_STAMP_COUNTER, low, high);
		time_stamp[cpu_id].last_low = time_stamp[cpu_id].low;
		time_stamp[cpu_id].last_high = time_stamp[cpu_id].high;
		time_stamp[cpu_id].low = low;
		time_stamp[cpu_id].high = high;
	}
}

EXPORT_SYMBOL_GPL(read_time_stamp);

u64 get_time_stamp(u32 cpu_id)
{
	u64 ts = (u64) time_stamp[cpu_id].low;
	ts = ts | ((u64) time_stamp[cpu_id].high << 32);
	return ts;
}

EXPORT_SYMBOL_GPL(get_time_stamp);

u64 get_last_time_stamp(u32 cpu_id)
{
	u64 ts = (u64) time_stamp[cpu_id].last_low;
	ts = ts | ((u64) time_stamp[cpu_id].last_high << 32);
	return ts;
}

EXPORT_SYMBOL_GPL(get_last_time_stamp);

//must be called using ON_EACH_CPU
void tsc_init_msrs(void *info)
{
	int cpu = smp_processor_id();
	if (cpu != 0) {
		time_stamp[cpu] = time_stamp[0];
	}
}

EXPORT_SYMBOL_GPL(tsc_init_msrs);

static int __init tsc__init(void)
{
	if (ON_EACH_CPU(tsc_init_msrs, NULL, 1, 1) < 0) {
		error("Could not init tsc on all cpus!");
		return -ENODEV;
	}
	return 0;
}

static void __exit tsc__exit(void)
{
	;
}

module_init(tsc__init);
module_exit(tsc__exit);
