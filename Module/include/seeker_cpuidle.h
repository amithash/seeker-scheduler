
#ifndef __SCPUIDLE_H_
#define __SCPUIDLE_H_


#define MAX_IDLE_STATES 5

#define ERR_CPU_DISABLED -1
#define ERR_CPU_INVALID -2
#define ERR_STATE_INVALID -3
#define ALL_OK 0


void enter_seeker_cpuidle(void);
void exit_seeker_cpuidle(void);
int sleep_proc(int cpu, int state);

#endif

