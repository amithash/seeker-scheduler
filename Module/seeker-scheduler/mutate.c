#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>

#include "state.h"
#include "scpufreq.h"
#include "debug.h"

#define ABS(i) ((i) >= 0 ? (i) : (-1*(i)))
#define div(a,b) ((a) / (b)) + (((a)%(b))<<1 >= (b) ? 1 : 0)

extern int total_online_cpus;
extern int max_allowed_states[NR_CPUS];
extern int cur_cpu_state[NR_CPUS];
extern unsigned int max_state_in_system;
int state_matrix[NR_CPUS][MAX_STATES];
extern struct state_desc states[MAX_STATES];

u64 interval_count;

inline int procs(int hints,int total, int proc, int total_load);

inline int procs(int hints,int total, int proc, int total_load)
{
	if(hints == 0)
		return 0;
	if(hints == total)
		return total_load;
	return div((hints * total_load),total);
}

void update_state_matrix(int *cpu_state, int delta)
{
	int i,j,l;
	for(i=0;i<total_online_cpus;i++){
		l=0;
		for(j=cpu_state[i];j<max_state_in_system;j++){
			if(l>delta)
				state_matrix[i][j] = 0;
			else
				state_matrix[i][j] = (max_state_in_system-l)*(max_state_in_system-l);
			l++;
		}
		l=1;
		for(j=cpu_state[i]-1;j>=0;j--){
			state_matrix[i][j] = (max_state_in_system-l)*(max_state_in_system-l);
			l++;
		}
	}
}

void choose_layout(int delta)
{
	int total = 0;
	int demand[MAX_STATES];
	int load = 0;
	struct debug_block *p = NULL;
	unsigned int i,j;
	unsigned int winner=0;
	unsigned int winner_val = 0;
	unsigned int winner_best_proc = 0;
	unsigned int winner_best_proc_value = 0;
	unsigned int winner_best_low_proc_value = 0;
	unsigned int best_proc = 0;
	unsigned int best_proc_value = 0;
	unsigned int best_low_proc_value = 0;
	int sum;
	short poison[NR_CPUS] = {1};
	int new_cpu_state[NR_CPUS] = {-1};
	cpumask_t mask;

	interval_count++;

	/* Create a state matrix such that, the cell which
	 * indicates a processors current state, gets the highest
	 * value = iax_state_in_system^2, and parabolically decreases on either side.
	 */
	for(i=0;i<total_online_cpus;i++){
		new_cpu_state[i] = cur_cpu_state[i];
		load += weighted_cpuload(i) >= SCHED_LOAD_SCALE ? 1 : 0;
	}

	update_state_matrix(new_cpu_state,delta);
	
	p = get_debug();
	if(p){
		p->entry.type = DEBUG_MUT;
		p->entry.u.mut.interval = interval_count;
		p->entry.u.mut.count = max_state_in_system;
	}

	/* Total Hint */
	
	for(i=0;i<max_state_in_system;i++){
		total += states[i].demand;
		if(p)
			p->entry.u.mut.hint[i] = states[i].demand;
	}

	/* Num of cpus required for this state 
	 * SUM(demand[]) could be < cpus. 
	 * Make sure to bring down their states. */
	for(j=0;j<max_state_in_system;j++){
		demand[j] = procs(states[j].demand,total,total_online_cpus,load);
		debug("required cpus for state %d = %d",j,demand[j]);
	}

	/* Now for each delta to spend, hold an auction */
	while(delta > 0){
		winner = 0;
		winner_val = 0;
		winner_best_proc = 0;
		winner_best_proc_value = 0;
		winner_best_low_proc_value = 0;

		/* There is an optimization here, so do not get confused.
		 * Technically, each column in the state matrix is supposed
		 * to be multiplied by the demand. But that is done here,
		 * as the demand decreases once won.
		 */

		/* For each state, */
		for(i=0;i<max_state_in_system;i++){
			sum = 0;
			best_proc = 0;
			best_proc_value = 0;
			best_low_proc_value = -1;

			/* Sum the cost over all rows */
			for(j=0;j<total_online_cpus;j++){
				if(state_matrix[j][i] * poison[j] > best_proc_value){
					best_proc_value = state_matrix[j][i] * poison[j];
					best_proc = j;
				} else if(state_matrix[j][i] < best_low_proc_value){
					best_low_proc_value = state_matrix[j][i];
				}
				sum += state_matrix[j][i];
			}
			sum = sum * (demand[i]+1);

			/* if this is the max, make a note of the 
			 * potential winner */
			if(sum > winner_val){
				winner = i;
				winner_val = sum;
				winner_best_proc= best_proc;
				winner_best_proc_value = best_proc_value;
				winner_best_low_proc_value = best_low_proc_value;
			} else if(sum == winner_val){
				if(best_proc_value > winner_best_proc_value){
					winner = i;
					winner_val = sum;
					winner_best_proc= best_proc;
					winner_best_proc_value = best_proc_value;
					winner_best_low_proc_value = best_low_proc_value;
				} else if(best_proc_value == winner_best_proc_value){
					if(best_low_proc_value > winner_best_low_proc_value){
						winner = i;
						winner_val = sum;
						winner_best_proc= best_proc;
						winner_best_proc_value = best_proc_value;
						winner_best_low_proc_value = best_low_proc_value;
					}
				}
			}
		}
		/* A winning val of 0 indicated a failed auction.
		 * all contenstents are broke. Go home loosers.*/
		if(winner_val <= 0)
			break;

		/* Now the winning state, reduces its demand */
		demand[winner]--;
	
		/* The best processor is best_proc */
		/* Poison the choosen processor */
		poison[winner_best_proc] = 0;

		/* Subtract that from the delta */
		delta -= abs(cur_cpu_state[winner_best_proc] - winner);

		/* Assign the new cpus state to be the winner */
		new_cpu_state[winner_best_proc] = winner;

		/* If the new state of the CPU is different,
		 * change the state matrix to reflect it */
		if(cur_cpu_state[winner_best_proc] != winner){
			update_state_matrix(new_cpu_state,delta);
		}

		/* Continue the auction if delta > 0 */
	}	

	/* XXX Explicit locking is required. 
	 * Not done right now. This can cause certain
	 * apps processorless.
	 */
	for(i=0;i<max_state_in_system;i++){
		states[i].cpus = 0;
		cpus_clear(states[i].cpumask);
	}


	for(i=0;i<total_online_cpus;i++){
		/* XXX This violates DELTA. But this 
		 * also makes sure that unused processors 
		 * are in the lowest cpu state */
		if(poison[i] == 1)
			new_cpu_state[i] = 0;
		
		if(new_cpu_state[i] != cur_cpu_state[i]){
			cur_cpu_state[i] = new_cpu_state[i];
			set_freq(i,new_cpu_state[i]);
		}
		states[cur_cpu_state[i]].cpus++;
		mask = cpumask_of_cpu(i);
		cpus_or(states[cur_cpu_state[i]].cpumask,
			states[cur_cpu_state[i]].cpumask,
			mask);
		if(p)
			p->entry.u.mut.cpustates[i] = cur_cpu_state[i];
	}

	if(p)
		debug_link(p);
}


