#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <seeker.h>
#include <pmu.h>
#include <fpmu.h>

#include "hwcounters.h"

int sys_counters[NR_CPUS][3];
u64 pmu_val[NR_CPUS][3];

void enable_pmu_counters(void)
{
	int cpu = smp_processor_id();
#if NUM_FIXED_COUNTERS > 0
		fpmu_init_msrs();
		fcounters_enable(0);
		fcounters_enable(1);
		fcounters_enable(2);
#else
		pmu_init_msrs();
		sys_counters[cpu][0] = counter_enable(PMU_INST_EVTSEL,PMU_INST_MASK,0);
		sys_counters[cpu][1] = counter_enable(PMU_RECY_EVTSEL,PMU_RECY_MASK,0);
		sys_counters[cpu][2] = counter_enable(PMU_RFCY_EVTSEL,PMU_RFCY_MASK,0);
#endif
		clear_counters(cpu);
}

int configure_counters(void)
{
	if(on_each_cpu((void *)enable_pmu_counters,NULL,1,1) < 0){
		error("Counters could not be configured");
		return -1;
	}
	return 0;
}

void read_counters(int cpu)
{
#if NUM_FIXED_COUNTERS > 0
	fcounter_read();
	pmu_val[cpu][0] = get_fcounter_data(0,cpu);
	pmu_val[cpu][1] = get_fcounter_data(1,cpu);
	pmu_val[cpu][2] = get_fcounter_data(2,cpu);
#else
	counter_read();
	pmu_val[cpu][0] = get_counter_data(sys_counters[cpu][0],cpu);
	pmu_val[cpu][1] = get_counter_data(sys_counters[cpu][0],cpu);
	pmu_val[cpu][2] = get_counter_data(sys_counters[cpu][0],cpu);
#endif
}

void clear_counters(int cpu)
{
#if NUM_FIXED_COUNTERS > 0
	fcounter_clear(0);
	fcounter_clear(1);
	fcounter_clear(2);
#else
	counter_clear(sys_counters[cpu][0]);
	counter_clear(sys_counters[cpu][1]);
	counter_clear(sys_counters[cpu][2]);
#endif
}
	
