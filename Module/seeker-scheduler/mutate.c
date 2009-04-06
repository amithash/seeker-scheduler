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

#define MIN_REQUESTS 4

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

static int poison[NR_CPUS][MAX_STATES];

static int load_average = 0;
static int last_load = 1;
static int step = 0;
#define EVALUATE_LOAD 10
#define DEMAND_SCALE MAX_STATES

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

inline int procs(int hints, int total, int total_load);

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

int transition_direction(void)
{
	int req_level = 0;
	int pre_level = 0;
	int i;

	for(i=0;i<total_states;i++){
		req_level += demand[i] * (i+1);
	}
	for(i=0;i<total_online_cpus;i++){
		if(info[i].sleep_time > 0)
			continue;
		pre_level += cur_cpu_state[i] + 1;
	}
	if(req_level > pre_level)
		return 1;
	if(req_level < pre_level)
		return -1;
	return 0;
}

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
inline unsigned int required_load(unsigned int total_load)
{
	unsigned int ret = LOAD_TO_UINT(total_load);
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
void update_state_matrix(int delta, int direction)
{
	int i = 0;
	int j = 0;
	int k = 0;

	for (i = 0; i < total_online_cpus; i++) {
		state_matrix[i][new_cpu_state[i]] = total_states * 2;
		for (j = new_cpu_state[i] + 1, k = 1; j < total_states; j++, k++) {
			if (k > delta){
				state_matrix[i][j] = 0;
			} else {
				if(direction > 0){
					state_matrix[i][j] = ((total_states*2) - k);
				} else {
					state_matrix[i][j] = (total_states - k);
				}
			}
		}

		for (j = new_cpu_state[i] - 1, k = 1; j >= 0; j--, k++) {
			if (k > delta){
				state_matrix[i][j] = 0;
			} else {
				if(direction < 0){
					state_matrix[i][j] = ((total_states*2) - k);
				} else {
					state_matrix[i][j] = total_states - k;
				}
			}
		}
	}
}

/********************************************************************************
 * update_demand_field - update the demand field for a friend count.
 * @friend_count - (total_states/2)-1
 * @Side Effects - Updates the demand field matrix.
 *
 * Each and every column j state gives itself the current demand + a share which 
 * is demand + (demand/2). Then gives (demand/2)-distance to each and every
 * column where distance = |i-j|<friend_count;
 * And friends are helped only if they are broke. 
 ********************************************************************************/
void update_demand_field(void)
{
	int i;
	for(i = 0; i < total_states; i++) {
		demand_field[i] = demand[i];
	}
}
#if 0
void update_demand_field(int friend_count)
{
	int i, j, k;
	int share;
	for (i = 0; i < total_states; i++) {
		demand_field[i] = 1;
	}
	for (i = 0; i < total_states; i++) {
		share = demand[i] >> 1;
		demand_field[i] += (demand[i] + share);
		for (j = i + 1, k = 1;
		     j < total_states && (share - k) > 0 && k <= friend_count;
		     j++, k++) {
			if(demand[j] != 0)
				break;
			demand_field[j] += (share - k);
		}
		for (j = i - 1, k = 1;
		     j >= 0 && (share - k) > 0 && k <= friend_count; j--, k++) {
			if(demand[j] != 0)
				break;
			demand_field[j] += (share - k);
		}
	}
}
#endif

/********************************************************************************
 * per_state_kernel - sum over state column and return best proc.
 * @state - (In) The state (Column indicator)
 * @poison - (In) array indicating free processors. 
 * @d  - (In) demand field element for state - state;
 * @proc - (Out) Elected processor for this state.
 * @proc_val - (Out) Value of the elected processor
 * @low_proc_val - (Out) Value of the losing processor. 
 * @Return - Sum along the state column. 
 * @Side Effects - None
 *
 * Compute the value of each state column and return it. In this column,
 * elect a processor with the largest value 
 * cell_value * poison[i] * demand - sleep_time
 * poison - Because we do not want to elect a selected processor.
 * sleep_time - because we want to select a processor who has slept the most.
 * and the lowest value of cell_value * demand is computed.
 *
 * This is used by mutator_kernel.
 ********************************************************************************/
int per_state_kernel(int state, int d, int *proc,
		     int *proc_val, unsigned int *low_proc_val)
{
	int i;
	int j = state;		/* Avoid mentioning state everywhere */
	int sum = 0;
	int val = 0;

	*proc = -1;
	*proc_val = 0;
	*low_proc_val = NR_CPUS * MAX_STATES;

	for (i = 0; i < total_online_cpus; i++) {
		sum += state_matrix[i][j];
		if(poison[i][j] == 1)
			continue;
		val = (state_matrix[i][j] * d) > info[i].sleep_time ? 
			(state_matrix[i][j] * d) - info[i].sleep_time : 0;

		if (val > *proc_val) {
			*proc_val = val;
			*proc = i;
		} else if (val < *low_proc_val) {
			*low_proc_val = val;
		}
	}
	return sum * d;
}

/********************************************************************************
 * mutator_kernel - the base kernel of the mutator.
 * @poison - (In) Array of poison bits
 * @winner - (Out) winning state
 * @Return - winning processor.
 * @Side Effects - None
 *
 * Sum along the states column and multiply the row vector with the demand
 * field vector. Choose the element with the highest value, and return the
 * index (Processor) and the winning state (Winner).
 * If there exists for the winning position, select the one with highest
 * best_proc_value Even after which if there is a tie, the highest among
 * the lowest value of each column is slected.
 *
 * The above is computed in the per_state_kernel.
 ********************************************************************************/
int mutator_kernel(int *winner)
{
	int sum;
	int j;
	int best_proc = 0;
	int best_proc_value = 0;
	unsigned int best_low_proc_value = -1;
	unsigned int winner_best_proc = 0;
	int winner_best_proc_value = 0;
	int winner_val = 0;
	unsigned int winner_best_low_proc_value = 0;

	for (j = 0; j < total_states; j++) {
		/* Sum the cost over all rows */
		/* Do here to make the longest sleeping processor to sleep more 
		 * does this conserve more power? */

		sum = per_state_kernel(j, demand_field[j],
				       &best_proc, &best_proc_value,
				       &best_low_proc_value);

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
		*winner = j;
		winner_val = sum;
		winner_best_proc = best_proc;
		winner_best_proc_value = best_proc_value;
		winner_best_low_proc_value = best_low_proc_value;
	}

	if (winner_val > 0)
		return winner_best_proc;
	else
		return -1;
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
	unsigned int winner_best_proc = 0;
	int total_demand = 0;
	int cpu_selected[NR_CPUS];
	int total_iter = 0;
	int total_provided_cpus = 0;
	int total_selected_cpus = 0;
	int direction;

	interval_count++;
	if (delta < 1)
		return;

	/* Compute the system load, and initialize 
	 * new_cpu_state to the current as no change has been made
	 */
	for (i = 0; i < total_online_cpus; i++) {
		for(j=0;j<total_states;j++){
			poison[i][j] = 0;
		}
		cpu_selected[i] = 0;
		new_cpu_state[i] = cur_cpu_state[i];
		load = ADD_LOAD(load, get_cpu_load(i));
	}

	/* Get load in terms of number of processors */
	load = required_load(load);
	if(step < EVALUATE_LOAD){
		step++;
		load_average += load;
		load = last_load;
	} else {
		load = div(load_average,EVALUATE_LOAD);
		last_load = load;
		step = 0;
		load_average = 0;
	}

	debug("load of system = %d", load);

	/* Compute the total = sum of hints */
	for (j = 0; j < total_states; j++) {
		total += hint_get(j);
		debug("State: %d ; Demand: %d", j, hint_get(j));
	}

	/* Compute CPU's demanded for each state */
	for (j = 0; j < total_states; j++) {
		cpus_demanded[j] = demand[j] =
		    procs(hint_get(j), total, load);
		total_demand += demand[j];
	}
	if (total_demand != load)
		total_demand = load;

	direction = transition_direction();

	/* First lets find "total_demand" processors and wake them up */
	for(i=0; i<total_demand; i++){
		int awake_total = 0;
		unsigned int min_sleep_time = -1;
		unsigned int wake_up_proc = -1;
		for(j=0; j < total_online_cpus && awake_total < total_demand; j++){
			if(info[j].sleep_time == 0){
				awake_total++;
				continue;
			}
			if(info[j].sleep_time < min_sleep_time){
				wake_up_proc = j;
				min_sleep_time = info[j].sleep_time;
			}
		}
		if(awake_total >= total_demand)
			break;
		if(wake_up_proc < total_online_cpus){
			info[wake_up_proc].sleep_time = 0;
			info[wake_up_proc].awake = 1;
		}
	}
				
	/* Now for each delta to spend, hold an auction */
	while (total > MIN_REQUESTS && load > 0 && delta > 0
	       && total_iter < total_online_cpus) {
		winner = 0;
		winner_best_proc = 0;
		total_iter++;

		debug("Iteration %d", total_iter);

		/* Compute the state matrix */
		update_state_matrix(delta,direction);

		/* Compute the demand field */
		update_demand_field();

		winner_best_proc = mutator_kernel(&winner);

		/* A winning val of 0 indicated a failed auction.
		 * all contenstents are broke. Go home loosers.*/
		if (winner_best_proc < 0 || winner_best_proc >= total_online_cpus){
			debug("Auction ended with invalid best proc!");
			break;
		}

		if (winner < 0 || winner >= total_states){
			debug("Auction ended with invalid state");
			break;
		}

		debug("Winner is state %d choosing cpu %d", winner,
		      winner_best_proc);

		/* Now the winning state, reduces its demand */
		if (demand[winner] > 0) {
			demand[winner]--;
			total_demand--;
		}

		/* The best processor is best_proc */
		/* Poison the choosen processor element */
		poison[winner_best_proc][winner] = 1;
		cpu_selected[winner_best_proc] = 1;
		total_selected_cpus++;

		/* Subtract that from the delta */
		delta -= ABS(cur_cpu_state[winner_best_proc] - winner);

		/* Assign the new cpus state to be the winner */
		new_cpu_state[winner_best_proc] = winner;

		/* Continue the auction if delta > 0  or till all cpus are allocated */
	}

 
	for(i=0; i < total_online_cpus; i++){
		if(total_selected_cpus >= load)
			break;
		if(cpu_selected[i] == 1)
			continue;
		if(info[i].sleep_time == 0){
			total_selected_cpus++;
			cpu_selected[i] = 1;
		}
	}

	debug("End of auction");

	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
	}

	for (i = 0; i < total_online_cpus; i++) {
		if (cpu_selected[i] == 0)
			continue;
		// new_cpu_state[i] = 0;
		new_states[new_cpu_state[i]].cpus++;
		total_provided_cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
	}
	if (total_provided_cpus == 0) {
		new_states[0].cpus++;
		cpu_set(0, new_states[0].cpumask);
		new_cpu_state[0] = 0;
		cpu_selected[0] = 1;
	}

	write_seqlock(&states_seq_lock);
	for (j = 0; j < total_states; j++) {
		states[j].cpumask = new_states[j].cpumask;
		states[j].cpus = new_states[j].cpus;
		hint_clear(j);
	}
	write_sequnlock(&states_seq_lock);

	for (i = 0; i < total_online_cpus; i++) {
		/* CPU is used */
		if (cpu_selected[i] == 1) {
			info[i].sleep_time = 0;
			if (info[i].awake == 0) {
				debug("awaking processor %d", i);
			}
			info[i].awake = 1;
			if (new_cpu_state[i] != cur_cpu_state[i]) {
				debug
				    ("Requesting cpu %d to change state from %d to %d",
				     i, cur_cpu_state[i], new_cpu_state[i]);
				set_freq(i, new_cpu_state[i]);
			}
		} else if (info[i].awake == 0) {
			info[i].sleep_time++;
		} else {
			debug("Putting cpu %d to sleep", i);
			if (cur_cpu_state[i] != 0)
				set_freq(i, 0);
			info[i].awake = 0;
			info[i].sleep_time = 1;
		}
	}
	/* Update debug log. by now, cur_cpu_state will be updated. */
	p = get_debug();
	if(!p) {
		goto exit_debug;
	}
	p->entry.type = DEBUG_MUT;
	p->entry.u.mut.interval = interval_count;
	p->entry.u.mut.count = total_states;
	for ( j = 0; j < total_states; j++ ) {
		p->entry.u.mut.cpus_req[j] = cpus_demanded[j];
		p->entry.u.mut.cpus_given[j] = 0;
	}
	for ( i = 0; i < total_online_cpus; i++ ) {
		p->entry.u.mut.cpus_given[cur_cpu_state[i]]++;
	}
exit_debug:
	put_debug(p);
}

