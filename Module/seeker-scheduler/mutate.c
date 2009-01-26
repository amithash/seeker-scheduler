#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "state.h"
#include "stats.h"
#include "debug.h"

static int demand_field[MAX_STATES];
static int new_cpu_state[NR_CPUS];
static int state_matrix[NR_CPUS][MAX_STATES];
static int demand[MAX_STATES];
extern struct state_desc states[MAX_STATES];
extern unsigned int max_state_in_system;
extern int total_online_cpus;
extern int max_allowed_states[NR_CPUS];
extern int cur_cpu_state[NR_CPUS];

u64 interval_count;

struct proc_info{
	unsigned int sleep_time;
	unsigned int awake;
};

static struct proc_info info[NR_CPUS];

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

void init_mutator(void)
{
	int i;
	for(i=0;i<NR_CPUS;i++){
		info[i].sleep_time = 0;
		info[i].awake = 1;
	}
}

void update_state_matrix(int delta)
{
	int i,j,k;
	for(i=0;i<total_online_cpus;i++){
		if(info[i].awake == 0){
			for(j=0;j<max_state_in_system;j++)
				state_matrix[i][j] = 0;
			continue;
		}
		for(j=new_cpu_state[i],k=0; j<max_state_in_system; j++,k++){
			if(k>delta)
				state_matrix[i][j] = 0;
			else
				state_matrix[i][j] = (max_state_in_system-k);
		}

		for(j=new_cpu_state[i]-1,k=1; j>=0; j--,k++){
			if(k>delta)
				state_matrix[i][j] = 0;
			else
				state_matrix[i][j] = (max_state_in_system-k);
		}
	}
}

/* Create a demand field such that, each state
 * gets its share and also shares half of what it
 * gets with its friends = friend_count on either side
 * = (max_state_in_system/2)-1.
 */
void update_demand_field(int friend_count)
{
	int i,j,k;
	int share;
	for(i=0;i<max_state_in_system;i++){
		demand_field[i] = 1;
	}
	share = demand[i] >> 1;
	for(i=0;i<max_state_in_system;i++){
		demand_field[i] += (demand[i] + share);
		for(j=i+1,k=1; j<max_state_in_system && (share - k) > 0 && k <= friend_count; j++,k++){
			demand_field[j] += (share-k);
		}
		for(j=i-1,k=1; j >= 0 && (share - k) > 0 && k <= friend_count; j--,k++){
			demand_field[j] += (share-k);
		}
	}
}

void choose_layout(int delta)
{
	int total = 0;
	int cpus_demanded[MAX_STATES];
	int load = 0;
	struct debug_block *p = NULL;
	unsigned int i,j;
	int winner=0;
	int total_demand = 0;
	unsigned int winner_val = 0;
	unsigned int winner_best_proc = 0;
	unsigned int winner_best_proc_value = 0;
	unsigned int winner_best_low_proc_value = 0;
	unsigned int best_proc = 0;
	unsigned int best_proc_value = 0;
	unsigned int best_low_proc_value = 0;
	int poison[NR_CPUS];
	int sum;
	int total_iter = 0;
	int friends = (max_state_in_system >> 1)-1;

	interval_count++;
	if(delta < 1)
		return;

	/* Create a state matrix such that, the cell which
	 * indicates a processors current state, gets the highest
	 * value = iax_state_in_system^2, and parabolically decreases on either side.
	 */
	for(i=0;i<total_online_cpus;i++){
		poison[i] = 1;
		new_cpu_state[i] = cur_cpu_state[i];
		load = ADD_LOAD(load,get_cpu_load(i));
	}
	load = LOAD_TO_UINT(load);
	debug("load of system = %d",load);

	/* Total Hint */
	
	for(j=0;j<max_state_in_system;j++){
		total += states[j].demand;
	}

	/* Num of cpus required for this state 
	 * SUM(demand[]) could be < cpus. 
	 * Make sure to bring down their states. */
	for(j=0;j<max_state_in_system;j++){
		cpus_demanded[j] = demand[j] = procs(states[j].demand,total,load);
		total_demand += demand[j];
	}

	/* Now for each delta to spend, hold an auction */
	while(delta > 0 && total_iter < total_online_cpus && total_demand > 0){
		winner = 0;
		winner_val = 0;
		winner_best_proc = 0;
		winner_best_proc_value = 0;
		winner_best_low_proc_value = 0;
		total_iter++;
	
		debug("Iteration %d",total_iter);
		
		update_state_matrix(delta);
		update_demand_field(friends);

		/* There is an optimization here, so do not get confused.
		 * Technically, each column in the state matrix is supposed
		 * to be multiplied by the demand. But that is done here,
		 * as the demand decreases once won.
		 */

		/* For each state, */
		for(j=0;j<max_state_in_system;j++){
			sum = 0;
			best_proc = 0;
			best_proc_value = 0;
			best_low_proc_value = -1;

			/* Sum the cost over all rows */
			/* Do here to make the longest sleeping processor to sleep more 
			 * does this conserve more power? */
			for(i=0;i<total_online_cpus;i++){
				if((state_matrix[i][j] * poison[i]) > best_proc_value){
					best_proc_value = (state_matrix[i][j] * poison[i] * demand_field[j]) - info[i].sleep_time;
					best_proc = i;
				} else if(state_matrix[i][j] < best_low_proc_value){
					best_low_proc_value = (state_matrix[i][j] * demand_field[j]) - info[i].sleep_time;
				}
				sum += (state_matrix[i][j] * poison[i]);
			}

			sum = sum * demand_field[j];
			debug("sum for state %d is %d with demand %d",j,sum,demand_field[j]);

			/* Find the max sum and the sate, and its best proc 
			 * If there is contention for that, choose the one
			 * with the best proc, if there is contention for both,
			 * choose the one with the best lowest proc,
			 * if there is contention for that too, then first come
			 * first serve */
			if(sum < winner_val)
				continue;
			if(sum > winner_val) 
				goto assign;
			if(best_proc_value < winner_best_proc_value)
				continue;
			if(best_proc_value > winner_best_proc_value)
				goto assign;
			if(best_low_proc_value < winner_best_low_proc_value)
				continue;

assign:
			winner = j;
			winner_val = sum;
			winner_best_proc= best_proc;
			winner_best_proc_value = best_proc_value;
			winner_best_low_proc_value = best_low_proc_value;
		}
		/* A winning val of 0 indicated a failed auction.
		 * all contenstents are broke. Go home loosers.*/
		if(winner_val <= 0)
			break;

		debug("Winner is state %d choosing cpu %d",winner,winner_best_proc);

		/* Now the winning state, reduces its demand */
		if(demand[winner] > 0){
			demand[winner]--;
			total_demand--;
		}

		/* The best processor is best_proc */
		/* Poison the choosen processor element */
		poison[winner_best_proc] = 0;

		/* Subtract that from the delta */
		delta -= ABS(cur_cpu_state[winner_best_proc] - winner);

		/* Assign the new cpus state to be the winner */
		new_cpu_state[winner_best_proc] = winner;

		/* Continue the auction if delta > 0  or till all cpus are allocated */
	}
	p = get_debug();
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
			p->entry.u.mut.cpus_req[j] = cpus_demanded[j];
			p->entry.u.mut.cpus_given[j] = 0;
		}
		states[j].demand = 0;
	}


	for(i=0;i<total_online_cpus;i++){
		if(poison[i] == 1)
			continue;
			// new_cpu_state[i] = 0;
		if(p)
			p->entry.u.mut.cpus_given[new_cpu_state[i]]++;
		states[new_cpu_state[i]].cpus++;
		cpu_set(i,states[new_cpu_state[i]].cpumask);
	}
	mark_states_consistent();
	put_debug(p);
	/* This is purposefully put in a different loop 
	 * due to the intereference with put_debug();
	 */
	for(i=0;i<total_online_cpus;i++){
		/* CPU is used */
		if(poison[i] == 0){
			info[i].sleep_time = 0;
			info[i].awake = 1;
			if(new_cpu_state[i] != cur_cpu_state[i]){
				cur_cpu_state[i] = new_cpu_state[i];
				set_freq(i,new_cpu_state[i]);
			}
		} else {
			info[i].sleep_time++;
			info[i].awake = 0;
		}
	}
}


