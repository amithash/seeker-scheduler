#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <seeker.h>
#include <pmu.h>
#include <fpmu.h>

#include "hwcounters.h"

#if NUM_FIXED_COUNTERS == 0
int sys_counters[NR_CPUS][3] = {{0,0,0}};
#endif

u64 pmu_val[NR_CPUS][3];
int ERROR=0;

void enable_pmu_counters(void *info)
{
	int cpu = get_cpu();
#if NUM_FIXED_COUNTERS > 0
	fcounters_enable(0);
#else
	if((sys_counters[cpu][0] = counter_enable(PMU_INST_EVTSEL,PMU_INST_MASK,0)) < 0){
		error("Could not enable INST on %d",cpu);
		sys_counters[cpu][0] = 0;
		ERROR=1;
	}
	if((sys_counters[cpu][1] = counter_enable(PMU_RECY_EVTSEL,PMU_RECY_MASK,0)) < 0){
		error("Could not enable RECY on cpu %d",cpu);
		sys_counters[cpu][1] = 1;
		ERROR=1;
	}
	if((sys_counters[cpu][2] = counter_enable(PMU_RFCY_EVTSEL,PMU_RFCY_MASK,0)) < 0){
		error("Could not enable RFCY on cpu %d",cpu);
		sys_counters[cpu][2] = 2;
		ERROR=1;
	}
#endif	
	clear_counters(cpu);
	put_cpu();
}

int configure_counters(void)
{
	if(ON_EACH_CPU(enable_pmu_counters,NULL,1,1) < 0){
		error("Counters could not be configured");
		return -1;
	}
	if(ERROR)
		return -1;

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
	pmu_val[cpu][1] = get_counter_data(sys_counters[cpu][1],cpu);
	pmu_val[cpu][2] = get_counter_data(sys_counters[cpu][2],cpu);
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

	
