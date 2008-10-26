
#ifndef _HW_COUNTERS_H_
#define _HW_COUNTERS_H_

/* Definations of the evtsel and mask 
 * for instructions retired,
 * real unhalted cycles
 * reference unhalted cycles 
 * for the AMD as it does not have fixed counters
 */
#define PMU_INST_EVTSEL 0x00000000
#define PMU_INST_MASK 0x00000000
#define PMU_RECY_EVTSEL 0x00000000
#define PMU_RECY_MASK 0x00000000
#define PMU_RFCY_EVTSEL 0x00000000
#define PMU_RFCY_MASK 0x00000000


void clear_counters(int cpu);
void enable_pmu_counters(void);
int configure_counters(void);
void read_counters(int cpu);

#endif

