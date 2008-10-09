#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


#include "state.h"
#include "../../Module/scpufreq.h"
#include "../../Module/seeker.h"

#include "../freq_schedule/hint.h"
#include "../freq_schedule/estimate.h"
#include "../freq_schedule/stats.h"

int max_state_possible[NR_CPUS] = {0};
int max_state_in_system = -1;
int cur_cpu_states[NR_CPUS] = {0};

int freq_delta(int delta)
{
	int *cpu_state = NULL;
	int hint[MAX_STATES];
	int cpus = num_online_cpus();
	warn("total online cpus = %d",cpus);	
	get_hint(hint,max_state_in_system+1);

	/* Process hint */

	get_state_of_cpu(cpu_state);
	/* Update state of cpu */
	put_state_of_cpu();
	return 0;
}

int init_cpu_states(unsigned int how)
{
	int cpus = num_online_cpus();
	int i;
	for(i=0;i<cpus;i++){
		max_state_possible[i] = get_max_states(i) - 1;
		if(max_state_in_system < max_state_possible[i])
			max_state_in_system = max_state_possible[i];
	}
	
	switch(how){
		case ALL_HIGH:
			for(i=0;i<cpus;i++)
				cur_cpu_states[i] = max_state_possible[i];
			break;
		case ALL_LOW:
			for(i=0;i<cpus;i++)
				cur_cpu_states[i] = 0;
			break;
		case BALANCE:
			for(i=0;i<(cpus>>1);i++)
				cur_cpu_states[i] = 0;
			for(;i<cpus;i++)
				cur_cpu_states[i] = max_state_possible[i];
			break;
		default:
			for(i=0;i<cpus;i++)
				cur_cpu_states[i] = max_state_possible[i];
			break;
	}
	for(i=0;i<cpus;i++)
		set_freq(i,cur_cpu_states[i]);
	return 0;
}

