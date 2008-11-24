
#ifndef __SCPUFREQ_H_
#define __SCPUFREQ_H_


#define MAX_STATES 8

int set_freq(unsigned int cpu,unsigned int freq_ind);
unsigned int get_freq(unsigned int cpu);
int inc_freq(unsigned int cpu);
int dec_freq(unsigned int cpu);
int get_total_states(void);
int get_max_states(int cpu);

#endif

