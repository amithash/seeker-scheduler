
#ifndef _HW_COUNTERS_H_
#define _HW_COUNTERS_H_

/* Definations of the evtsel and mask 
 * for instructions retired,
 * real unhalted cycles
 * reference unhalted cycles 
 * for the AMD as it does not have fixed counters
 */
#define PMU_INST_EVTSEL 0xC0
#define PMU_INST_MASK 0x00
#define PMU_RECY_EVTSEL 0x76
#define PMU_RECY_MASK 0x00
#define PMU_RFCY_EVTSEL 0x76
#define PMU_RFCY_MASK 0x00

void clear_counters(int cpu);
void enable_pmu_counters(void *info);
int configure_counters(void);
void read_counters(int cpu);

#endif
