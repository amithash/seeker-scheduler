#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "../seeker.h"
#include "../scpufreq.h"
#include "hint.h"

#define ABS(i) ((i) >= 0 ? (i) : (-1*(i)))

extern int max_allowed_states[NR_CPUS];
extern int cur_cpu_state[NR_CPUS];
inline int procs(int hints,int total, int proc);

inline int procs(int hints,int total, int proc)
{
	int i;
	int hp = hints * proc;
	if(hints == 0)
		return 0;
	if(hints == total)
		return proc;
	for(i=1;i<proc;i++){
		if((hp <= i*total) && (hp > (i-1)*total))
			break;
	}
	return i;
}

void choose_layout(int dt)
{
	int cpus;
	int hint[MAX_STATES];
	int count;
	int i,j;
	int total = 0;
	int cpus_in_state[MAX_STATES];
	short cpus_bitmask[NR_CPUS]={0};
	int req_cpus = 0;
	int delta = dt;

	/* RAM is not a problem, cpu cycles are.
	 * so use actual values here rather than
	 * NR_CPUS or MAX_STATES
	 */
	cpus = num_online_cpus();
	count = get_hint(hint);
	req_cpus = cpus;

	/* Total Hint */
	for(i=0;i<count;i++)
		total += hint[i];
	/* Num of cpus required for this state */
	for(j=0;j<count;j++){
		cpus_in_state[j] = procs(hint[j],total,cpus);
		debug("required cpus for state %d = %d",j,cpus_in_state[j]);
	}
	
	/* For each cpu. Check if it can
	 * get away with the state it is
	 * in 
	 */
	for(i=0;i<cpus;i++){
		if(cpus_in_state[cur_cpu_state[i]] > 0){
			cpus_bitmask[i] = 1;
			cpus_in_state[cur_cpu_state[i]]--;
			req_cpus--;
		}
	}

	if(req_cpus == 0)
		return;

	for(i=0;i<cpus;i++){
		if(req_cpus == 0 || delta == 0)
			return;
		if(cpus_bitmask[i])
			continue;

		/* Prefer to increase a state by 1 then
		 * to decrease the state by 1 */
		if(cur_cpu_state[i] < count && 
		   cpus_in_state[cur_cpu_state[i]+1] > 0){
			cpus_bitmask[i] = 1;
			cur_cpu_state[i]++;
			cpus_in_state[cur_cpu_state[i]]--;
			set_freq(i,cur_cpu_state[i]);
			req_cpus--;
			delta--;
		} else if(cur_cpu_state[i] > 0 && 
			  cpus_in_state[cur_cpu_state[i]-1] > 0){
			cpus_bitmask[i] = 1;
			cur_cpu_state[i]--;
			cpus_in_state[cur_cpu_state[i]]--;
			set_freq(i,cur_cpu_state[i]);
			req_cpus--;
			delta--;
		}
	}

	/* We have kept required procs.
	 * We have choosen to prefer to
	 * increase cpu state by 1 then
	 * to decrease by 1. Now we assign 
	 * the remaining delta.
	 */
	for(j=0;j<count;j++){
		for(i=0;i<cpus;i++){
			if(req_cpus == 0 || delta == 0)
				return;
	
			if(cpus_in_state[j] == 0)
				continue;
			if(cpus_bitmask[i])
				continue;

			/* Will this change honor delta? */
			if((cur_cpu_state[i]-j) <= delta || (j - cur_cpu_state[i]) <= delta){
				cur_cpu_state[i] = j;
				cpus_in_state[j]--;
				delta -= ABS(cur_cpu_state[i]-j);
				req_cpus--;
				set_freq(i,j);
				break;
			} else{
				/* Will not honor. Fix allow a change of what is allowed.*/
				if(j > cur_cpu_state[i]){
					cur_cpu_state[i] += delta;
				} else {
					cur_cpu_state[i] -= delta;
				}
				req_cpus--;
				delta = 0;
			}

		}
	}
}

