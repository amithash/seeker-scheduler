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

extern fixctrl_t fcontrol[NR_CPUS];
extern fcounter_t fcounters[NR_CPUS][NUM_FIXED_COUNTERS];
extern fcleared_t fcleared[NR_CPUS][NUM_FIXED_COUNTERS];

// Read the fixed counter control register.
inline u64 control_read(void)
{
	#if NUM_FIXED_COUNTERS > 0
	return native_read_msr(MSR_PERF_FIXED_CTR_CTRL);
	#else
	return 0;
	#endif
}
EXPORT_SYMBOL_GPL(control_read);


//clears the fixed counter control registers.
inline void control_clear(void)
{
	#if NUM_FIXED_COUNTERS > 0
	u64 val = native_read_msr(MSR_PERF_FIXED_CTR_CTRL);
	val &= FIXSEL_RESERVED_BITS;
	wrmsrl(MSR_PERF_FIXED_CTR_CTRL,val);
	#endif
}
EXPORT_SYMBOL_GPL(control_clear);

//write to the fixed counter control registers.
inline void control_write(void)
{
	#if NUM_FIXED_COUNTERS > 0
	u64 val;
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		val = native_read_msr(MSR_PERF_FIXED_CTR_CTRL);
		memcpy(&val,&(fcontrol[cpu_id]),sizeof(u64));
		wrmsrl(MSR_PERF_FIXED_CTR_CTRL, val);
	}
	#endif
}
EXPORT_SYMBOL_GPL(control_write);

//must be called using ON_EACH_CPU
inline void fcounter_clear(u32 counter)
{
	#if NUM_FIXED_COUNTERS > 0
	int cpu_id;
	cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		wrmsrl(fcounters[cpu_id][counter].addr, fcleared[cpu_id][counter].all);
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounter_clear);

//must be called using ON_EACH_CPU
void fcounter_read(void)
{
	#if NUM_FIXED_COUNTERS > 0
	int cpu_id, i;
	cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		/* this is the "full" read of the full 64bits */
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			fcounters[cpu_id][i].all = native_read_msr(fcounters[cpu_id][i].addr);
			fcounters[cpu_id][i].high = (u32)(fcounters[cpu_id][i].all << 32);
			fcounters[cpu_id][i].low = (u32)fcounters[cpu_id][i].all;
		}
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounter_read);

//use this to get the counter data
u64 get_fcounter_data(u32 counter, u32 cpu_id)
{
	#if NUM_FIXED_COUNTERS > 0
	if(likely(counter < NUM_FIXED_COUNTERS && cpu_id < NR_CPUS)){
		return fcounters[cpu_id][counter].all - fcleared[cpu_id][counter].all;
	}
	else{
		return -1;
	}
	#else
	return 0;
	#endif
}
EXPORT_SYMBOL_GPL(get_fcounter_data);

//must be called using ON_EACH_CPU
inline void fcounters_disable(void)
{
	#if NUM_FIXED_COUNTERS > 0
	int i;
	int cpu_id = smp_processor_id(); 
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			fcounter_clear(i);
		}
		control_clear();			
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounters_disable);

//must be called using ON_EACH_CPU
void fcounters_enable(u32 os)
{
	#if NUM_FIXED_COUNTERS > 0
	u32 i;
	int cpu_id = smp_processor_id();
	if(likely(cpu_id < NR_CPUS)){
		for(i=0;i<NUM_FIXED_COUNTERS;i++){
			fcounter_clear(i);
		}
		fcontrol[cpu_id].os0 = os;
		fcontrol[cpu_id].os1 = os;
		fcontrol[cpu_id].os2 = os;
		fcontrol[cpu_id].usr0 = 1;
		fcontrol[cpu_id].usr1 = 1;
		fcontrol[cpu_id].usr2 = 1;
		fcontrol[cpu_id].pmi0 = 0;
		fcontrol[cpu_id].pmi1 = 0;
		fcontrol[cpu_id].pmi2 = 0;
		control_write();
	}
	#endif
}
EXPORT_SYMBOL_GPL(fcounters_enable);
