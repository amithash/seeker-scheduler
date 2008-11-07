#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>
#include <scpufreq.h>

#include "state.h"
#include "debug.h"


static int new_cpu_state[NR_CPUS];
extern struct state_desc states[MAX_STATES];
extern unsigned int max_state_in_system;
extern int total_online_cpus;
extern int max_allowed_states[NR_CPUS];
extern int cur_cpu_state[NR_CPUS];

u64 interval_count;

inline int procs(int hints,int total, int total_load);

inline int procs(int hints,int total, int total_load)
{
	int ans;
	if(hints == 0)
		return 0;
	if(hints == total)
		return total_load;
	
	ans = div((hints * total_load),total);
	return ans < 0 ? 0 : ans;
}

void choose_layout(int delta)
{
	int demand[MAX_STATES];
	int i,j;
	int load;
	int work_required;
	int work_done;
	int winning_cpu;
	int winning_cpu_state = 0;
	int total;
	struct debug_block *p = NULL;
	int stop = 0;
	unsigned long irq_flags;
	int low = 0;
	int high = 0;

	for(i=0;i<total_online_cpus;i++){
		new_cpu_state[i] = cur_cpu_state[i];
		load += weighted_cpuload(i) >= SCHED_LOAD_SCALE ? 1 : 0;
		work_done += cur_cpu_state[i];
	}
	for(j=0;j<max_state_in_system;j++){
		total += states[j].demand;
	}
	for(j=0;j<max_state_in_system;j++){
		demand[j] = procs(states[j].demand,total,load);
		debug("required cpus for state %d = %d",j,demand[j]);
	}

	while(delta > 0 && !stop){
		/* Need a higher cpu find lowest and increase it by a state */
		if((work_required - work_done) > 0){
			/* There is a possibility of infinite
			 * oscillations (Atleast till delta of course)
			 * but we do not want that, so check if the other
			 * code section has been executed */
			if(high == 1)
				break;
			low = 1;
			winning_cpu = 0;
			winning_cpu_state = max_state_in_system;
			for(i=0;i<total_online_cpus;i++){
				if(new_cpu_state[i] < winning_cpu_state){
					winning_cpu_state = new_cpu_state[i];
					winning_cpu = i;
				}
			}
			if(new_cpu_state[winning_cpu] < max_state_in_system){
				new_cpu_state[winning_cpu]++;
				delta--;
				work_required++;
			}
		} else if((work_required - work_done) < 0){ /* Lower */
			/* same as above */
			if(low == 1) 
				break;
			high = 1;
			winning_cpu = 0;
			winning_cpu_state = 0;
			for(i=0;i<total_online_cpus;i++){
				if(new_cpu_state[i] > winning_cpu_state){
					winning_cpu_state = new_cpu_state[i];
					winning_cpu = i;
				}
			}
			if(new_cpu_state[winning_cpu] > 0){
				new_cpu_state[winning_cpu]--;
				work_required--;
				delta++;
			}
		} else {
			/* They are equal, best case reached,
			 * break */
			break;
		}
	}

	/* Record the changes in the debug log */
	p = get_debug(&irq_flags);
	if(p){
		p->entry.type = DEBUG_MUT;
		p->entry.u.mut.interval = interval_count;
		p->entry.u.mut.count = max_state_in_system;
	}
	mark_states_inconsistent();
	for(j=0;j<max_state_in_system;j++){
		states[j].cpus = 0;
		cpus_clear(states[j].cpumask);
		if(p){
			p->entry.u.mut.cpus_req[j] = demand[j];
			p->entry.u.mut.cpus_given[j] = 0;
		}
		states[j].demand = 0;
	}


	for(i=0;i<total_online_cpus;i++){
		if(new_cpu_state[i] != cur_cpu_state[i]){
			cur_cpu_state[i] = new_cpu_state[i];
			set_freq(i,new_cpu_state[i]);
			if(p)
				p->entry.u.mut.cpus_given[cur_cpu_state[i]]++;
		}
		states[cur_cpu_state[i]].cpus++;
		cpu_set(i,states[cur_cpu_state[i]].cpumask);
	}
	mark_states_consistent();
	put_debug(p,&irq_flags);
}


