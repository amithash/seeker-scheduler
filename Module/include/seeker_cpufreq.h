
#ifndef __SCPUFREQ_H_
#define __SCPUFREQ_H_


#define MAX_STATES 8

/* Error codes */
#define ERR_USER_EXISTS -1
#define ERR_USER_MEM_LOW -2
#define ERR_INV_USER -3
#define ERR_INV_CALLBACK -4

struct scpufreq_user{
	int (*inform) (int cpu, int state);
	int user_id;
};


int register_scpufreq_user(struct scpufreq_user *u);
int deregister_scpufreq_user(struct scpufreq_user *u);

int set_freq(unsigned int cpu,unsigned int freq_ind);
int __set_freq(unsigned int cpu,unsigned int freq_ind);
unsigned int get_freq(unsigned int cpu);
int inc_freq(unsigned int cpu);
int dec_freq(unsigned int cpu);
int get_total_states(void);
int get_max_states(int cpu);

#endif

