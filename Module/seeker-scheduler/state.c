#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <seeker.h>

#include "state.h"
#include "scpufreq.h"
#include "assigncpu.h"
#include "mutate.h"

extern int state_of_cpu[NR_CPUS];
int max_state_possible[NR_CPUS] = {0};
unsigned int max_state_in_system = -1;
int cur_cpu_state[NR_CPUS] = {0};
struct state_desc states[MAX_STATES];


int get_total_states(void)
{
	return max_state_in_system;
}

void hint_inc(int state)
{
	atomic_inc((void *)&(states[state].demand));
}
EXPORT_SYMBOL_GPL(hint_inc);

void hint_dec(int state)
{
	atomic_dec((void *)&(states[state].demand));
}
EXPORT_SYMBOL_GPL(hint_dec);

int freq_delta(int delta)
{
	int i;
	int cpus = num_online_cpus();

	choose_layout(delta);	

	get_state_of_cpu();
		
	for(i=0;i<cpus;i++)
		state_of_cpu[i] = cur_cpu_state[i];
	put_state_of_cpu();
	return 0;
}

int init_cpu_states(unsigned int how)
{
	int cpus = num_online_cpus();
	cpumask_t mask;
	int i;
	for(i=0;i<cpus;i++){
		max_state_possible[i] = get_max_states(i);
		if(max_state_in_system < max_state_possible[i])
			max_state_in_system = max_state_possible[i];
	}
	for(i=0;i<max_state_in_system;i++){
		states[i].state = i;
		states[i].cpus = 0;
		states[i].demand = 0;
		cpus_clear(states[i].cpumask);
	}

	switch(how){
		case ALL_HIGH:
			states[max_state_in_system-1].cpus = cpus;
			for(i=0;i<cpus;i++){
				mask = cpumask_of_cpu(i);
				cpus_or(states[max_state_in_system-1].cpumask,
					states[max_state_in_system-1].cpumask,
					mask);
				cur_cpu_state[i] = max_state_possible[i];
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		case ALL_LOW:
			states[0].cpus = cpus;
			for(i=0;i<cpus;i++){
				mask = cpumask_of_cpu(i);
				cpus_or(states[0].cpumask,
					states[0].cpumask,
					mask);
				cur_cpu_state[i] = 0;
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		case BALANCE:
			states[max_state_in_system-1].cpus = cpus >> 1;
			states[0].cpus = cpus - (cpus>>1);
			for(i=0;i<(cpus>>1);i++){
				mask = cpumask_of_cpu(i);
				cpus_or(states[max_state_in_system-1].cpumask,
					states[max_state_in_system-1].cpumask,
					mask);
				cur_cpu_state[i] = 0;
				set_freq(i,cur_cpu_state[i]);
			}
			for(;i<cpus;i++){
				mask = cpumask_of_cpu(i);
				cpus_or(states[0].cpumask,
					states[0].cpumask,
					mask);
				cur_cpu_state[i] = max_state_possible[i];
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		default:
			states[max_state_in_system-1].cpus = cpus;
			for(i=0;i<cpus;i++){
				mask = cpumask_of_cpu(i);
				cpus_or(states[max_state_in_system-1].cpumask,
					states[max_state_in_system-1].cpumask,
					mask);
				cur_cpu_state[i] = max_state_possible[i];
				set_freq(i,cur_cpu_state[i]);
			}
			break;
	}
	return 0;
}

