/*****************************************************
 * Copyright 2008 Amithash Prasad                    *
 *                                                   *
 * This file is part of Seeker                       *
 *                                                   *
 * Seeker is free software: you can redistribute     *
 * it and/or modify it under the terms of the        *
 * GNU General Public License as published by        *
 * the Free Software Foundation, either version      *
 * 3 of the License, or (at your option) any         *
 * later version.                                    *
 *                                                   *
 * This program is distributed in the hope that      *
 * it will be useful, but WITHOUT ANY WARRANTY;      *
 * without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE. See the GNU General Public License       *
 * for more details.                                 *
 *                                                   *
 * You should have received a copy of the GNU        *
 * General Public License along with this program.   *
 * If not, see <http://www.gnu.org/licenses/>.       *
 *****************************************************/

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

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* state.c: state descriptors */
extern struct state_desc states[MAX_STATES];

/* state.c: total states for each cpu */
extern unsigned int total_states;

/* main.c: value returned by num_online_cpus() */
extern int total_online_cpus;

/* state.c: States for each cpu */
extern int max_allowed_states[NR_CPUS];

/* state.c: current state of each cpu */
extern int cur_cpu_state[NR_CPUS];

/* state.c: seq lock for states */
extern seqlock_t states_seq_lock;


/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

static struct state_desc new_states[MAX_STATES];

/* demand field for each state */
static int demand_field[MAX_STATES];

/* Selected states for cpus */
static int new_cpu_state[NR_CPUS];

/* State matrix used to evaluate new_cpu_state */
static int state_matrix[NR_CPUS][MAX_STATES];

/* Computed demand for each state */
static int demand[MAX_STATES];

/* Mutator interval */
u64 interval_count;

/* sleep_time - intervals the cpu has been sleeping 
 * awake - 1 if cpu is awake, 0 otherwise 
 */
struct proc_info {
	unsigned int sleep_time;
	unsigned int awake;
};

/* Proc info for each cpu */
static struct proc_info info[NR_CPUS];

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

inline int procs(int hints, int total, int total_load);

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/


/********************************************************************************
 * procs - compute total processors required given hints and total
 * @hints - the demand 
 * @total - sum of all demands for all states.
 * @total_load - integer load of system (number of cpus 
 * @Side Effects - None
 * @return - Total processors required for the state with hint = hints
 *
 * Take hints and compute procs = (hints / total) * total_load
 ********************************************************************************/
inline int procs(int hints, int total, int total_load)
{
	int ans;
	if (hints == 0)
		return 0;
	if (hints == total)
		return total_load;

	ans = div((hints * total_load), total);
	return ans < 0 ? 0 : ans;
}


/********************************************************************************
 * required_load - adjusts load
 * @total_load - load in load units.
 * @return - adjusted load in num processors
 * @Side Effects - None
 *
 * Takes load (in load units) and adjusts it by: rounds it up as long as the 
 * rounded number does not cross total_online_cpus. if load is an exact integer,
 * then it is incremented (To accomadate higher load demands) and returns 1 if
 * the load is 0. So, at least 1 cpu is avaliable for new tasks. 
 ********************************************************************************/
inline int required_load(int total_load)
{
	int ret = LOAD_TO_UINT(total_load);
	if (ret < total_online_cpus) {
		/* Round to nearest integer */
		if ((total_load & 7) > 3)
			ret++;
		/* if it is on the dot, then more
		 * cpus are required. So, dub the load
		 * up by one This also takes care of 
		 * 0 load. */
		else if ((total_load & 7) == 0)
			ret++;
		else if (ret == 0)
			ret++;
	}
	return ret;
}

/********************************************************************************
 * init_mutator - initialize mutator local structures.
 * @Side Effects - For each cpu, sleep_time is set to 0 and is set to be awake. 
 *
 * Inits mutator structures. 
 ********************************************************************************/
void init_mutator(void)
{
	int i;
	for (i = 0; i < NR_CPUS; i++) {
		info[i].sleep_time = 0;
		info[i].awake = 1;
	}
}

/********************************************************************************
 * update_state_matrix - updates state_matrix for the value of delta. 
 * @delta - the delta for which state_matrix must be updated.
 * @Side Effects - state_matrix is re-populated based on new_cpu_state and delta
 *
 * Takes in delta, the values in new_cpu_state and computes the state matrix 
 * for each row (cpu) the column values is set to be total_states-distance where
 * distance is the distance from column (cur_cpu_state) and 0 if that distance 
 * is greater than delta.
 ********************************************************************************/
void update_state_matrix(int delta)
{
	int i, j, k;
	for (i = 0; i < total_online_cpus; i++) {
		for (j = new_cpu_state[i], k = 0; j < total_states; j++, k++) {
			if (k > delta)
				state_matrix[i][j] = 0;
			else
				state_matrix[i][j] = (total_states - k);
		}

		for (j = new_cpu_state[i] - 1, k = 1; j >= 0; j--, k++) {
			if (k > delta)
				state_matrix[i][j] = 0;
			else
				state_matrix[i][j] = (total_states - k);
		}
	}
}

/* Create a demand field such that, each state
 * gets its share and also shares half of what it
 * gets with its friends = friend_count on either side
 * = (total_states/2)-1.
 */
/********************************************************************************
 * update_demand_field - update the demand field for a friend count.
 * @friend_count - (total_states/2)-1
 * @Side Effects - Updates the demand field matrix.
 *
 * Each and every column j state gives itself the current demand + a share which 
 * is demnad + (demand/2). Then gives (demand/2)-distance to each and every
 * column where distance = |i-j|<friend_count;
 ********************************************************************************/
void update_demand_field(int friend_count)
{
	int i, j, k;
	int share;
	for (i = 0; i < total_states; i++) {
		demand_field[i] = 1;
	}
	share = demand[i] >> 1;
	for (i = 0; i < total_states; i++) {
		demand_field[i] += (demand[i] + share);
		for (j = i + 1, k = 1;
		     j < total_states && (share - k) > 0 && k <= friend_count;
		     j++, k++) {
			demand_field[j] += (share - k);
		}
		for (j = i - 1, k = 1;
		     j >= 0 && (share - k) > 0 && k <= friend_count; j--, k++) {
			demand_field[j] += (share - k);
		}
	}
}

/********************************************************************************
 * choose_layout - The mutator called every mutator interval.
 * @delta - The delta of the system chosen at module insertion. 
 * @Side effects - Changes cur_cpu_state and states field during which the states
 * 		   will be incosistent. 
 *
 * Chooses the layout based on constraints like delta and demand. Will also set
 * the hints to 0 so the next interval will be fresh. The function is rather 
 * big, so it is explained inline. 
 ********************************************************************************/
void choose_layout(int delta)
{
	int total = 0;
	int cpus_demanded[MAX_STATES];
	int load = 0;
	struct debug_block *p = NULL;
	unsigned int i, j;
	int winner = 0;
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
	int friends = (total_states >> 1) - 1;

	interval_count++;
	if (delta < 1)
		return;

	/* Compute the system load, and initialize 
	 * new_cpu_state to the current as no change has been made
	 */
	for (i = 0; i < total_online_cpus; i++) {
		poison[i] = 1;
		new_cpu_state[i] = cur_cpu_state[i];
		load = ADD_LOAD(load, get_cpu_load(i));
	}

	/* Get load in terms of number of processors */
	load = required_load(load);

	debug("load of system = %d", load);

	/* Compute the total = sum of hints */
	for (j = 0; j < total_states; j++) {
		total += states[j].demand;
	}

	/* Compute CPU's demanded for each state */
	for (j = 0; j < total_states; j++) {
		cpus_demanded[j] = demand[j] =
		    procs(states[j].demand, total, load);
		total_demand += demand[j];
	}

	/* Now for each delta to spend, hold an auction */
	while (delta > 0 && total_iter < total_online_cpus && total_demand > 0) {
		winner = 0;
		winner_val = 0;
		winner_best_proc = 0;
		winner_best_proc_value = 0;
		winner_best_low_proc_value = 0;
		total_iter++;

		debug("Iteration %d", total_iter);

		/* Compute the state matrix */
		update_state_matrix(delta);

		/* Compute the demand field */
		update_demand_field(friends);

		/* Sum along each column of state_matrix and multiply 
		 * it with the demand field for that particular column.
		 *
		 * Choose the maximum. And upon a tie, choose the one
		 * which has the largest element and also which has been
		 * awake the most. 
		 *
		 * If a tie, choose the one with the highest (minimum)
		 * along a column
		 */
		for (j = 0; j < total_states; j++) {
			sum = 0;
			best_proc = 0;
			best_proc_value = 0;
			best_low_proc_value = -1;

			/* Sum the cost over all rows */
			/* Do here to make the longest sleeping processor to sleep more 
			 * does this conserve more power? */
			for (i = 0; i < total_online_cpus; i++) {
				if ((state_matrix[i][j] * poison[i]) >
				    best_proc_value) {
					best_proc_value =
					    (state_matrix[i][j] * poison[i] *
					     demand_field[j]) -
					    info[i].sleep_time;
					best_proc = i;
				} else if (state_matrix[i][j] <
					   best_low_proc_value) {
					best_low_proc_value =
					    (state_matrix[i][j] *
					     demand_field[j]) -
					    info[i].sleep_time;
				}
				sum += (state_matrix[i][j] * poison[i]);
			}

			sum = sum * demand_field[j];
			debug("sum for state %d is %d with demand %d", j, sum,
			      demand_field[j]);

			/* Find the max sum and the sate, and its best proc 
			 * If there is contention for that, choose the one
			 * with the best proc, if there is contention for both,
			 * choose the one with the best lowest proc,
			 * if there is contention for that too, then first come
			 * first serve. */
			if (sum < winner_val)
				continue;
			if (sum > winner_val)
				goto assign;
			if (best_proc_value < winner_best_proc_value)
				continue;
			if (best_proc_value > winner_best_proc_value)
				goto assign;
			if (best_low_proc_value < winner_best_low_proc_value)
				continue;

assign:
			winner = j;
			winner_val = sum;
			winner_best_proc = best_proc;
			winner_best_proc_value = best_proc_value;
			winner_best_low_proc_value = best_low_proc_value;
		}
		/* A winning val of 0 indicated a failed auction.
		 * all contenstents are broke. Go home loosers.*/
		if (winner_val <= 0)
			break;

		debug("Winner is state %d choosing cpu %d", winner,
		      winner_best_proc);

		/* Now the winning state, reduces its demand */
		if (demand[winner] > 0) {
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

	/* Log with debug */
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_MUT;
		p->entry.u.mut.interval = interval_count;
		p->entry.u.mut.count = total_states;
	}

	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
		if (p) {
			p->entry.u.mut.cpus_req[j] = cpus_demanded[j];
			p->entry.u.mut.cpus_given[j] = 0;
		}
		new_states[j].demand = 0;
	}

	for (i = 0; i < total_online_cpus; i++) {
		if (poison[i] == 1)
			continue;
		// new_cpu_state[i] = 0;
		if (p)
			p->entry.u.mut.cpus_given[new_cpu_state[i]]++;
		new_states[new_cpu_state[i]].cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
	}
	put_debug(p);

	write_seqlock(&states_seq_lock);
	for(j = 0; j < total_states; j++){
		memcpy(&(states[j]),&(new_states[j]),sizeof(struct state_desc));
	}
	write_sequnlock(&states_seq_lock);
	/* This is purposefully put in a different loop 
	 * due to the intereference with put_debug();
	 * Do not try to be smart and merge this loop with 
	 * the above!
	 */
	for (i = 0; i < total_online_cpus; i++) {
		/* CPU is used */
		if (poison[i] == 0) {
			info[i].sleep_time = 0;
			info[i].awake = 1;
			if (new_cpu_state[i] != cur_cpu_state[i]) {
				cur_cpu_state[i] = new_cpu_state[i];
				set_freq(i, new_cpu_state[i]);
			}
		} else {
			info[i].sleep_time++;
			info[i].awake = 0;
		}
	}
}


