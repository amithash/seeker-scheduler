
#ifndef __STATE_H_
#define __STATE_H_

#define ALL_HIGH 1
#define BALANCE 2
#define ALL_LOW 3

struct state_desc{
	short state;
	cpumask_t cpumask;
	short cpus;
	unsigned int demand;
};

void hint_inc(int state);
void hint_dec(int state);
int init_cpu_states(unsigned int how);
void mark_states_inconsistent(void);
void mark_states_consistent(void);
int is_states_consistent(void);

#endif
