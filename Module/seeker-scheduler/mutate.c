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
#include "nrtasks.h"

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

#if MUTATOR_TYPE == APPROXIMATE_DIRECTION_BASED_MUTATOR
static struct state_desc new_states[MAX_STATES];

/* demand field for each state */
static int demand_field[MAX_STATES];

/* Selected states for cpus */
static int new_cpu_state[NR_CPUS];

/* State matrix used to evaluate new_cpu_state */
static int state_matrix[NR_CPUS][MAX_STATES];

/* state weight: summed along columns of state matrix */
static int state_weight[MAX_STATES];

/* The procs which wins if the indexed state wins */
static int winning_procs[MAX_STATES];

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

/* Vector: 0 - if a proc is selected, 1 otherwise */
static int selected_cpus[NR_CPUS];

/* proxy demand source. if a demand is partly from another
 * state, then this contains the index to that, else to itself
 */
static int proxy_source[MAX_STATES];

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/
inline int procs(int hints, int total, int total_load);

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * transition_direction - Compute direction or transtion
 * @return - 1 for high, -1 for low, 0 for balance.
 *
 * Assumes that cur_cpu_state is valid and the demand vector is computed. 
 * returns the transition direction. 
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
	int left = (((direction * direction) - direction + 2) * total_states) >> 1;
	int right = (((direction * direction) + direction + 2) * total_states) >> 1;

	for (i = 0; i < total_online_cpus; i++) {
		/* 0 to L[i]-delta */
		for(j = 0; j < (new_cpu_state[i] - delta); j++){
			state_matrix[i][j] = 0;
		}
		/* L-Delta to L[i] */
		for( ; j < new_cpu_state[i]; j++){
			state_matrix[i][j] =  left + j - new_cpu_state[i];
		}

		/* L[i] */
		state_matrix[i][j++] = (total_states << 1);

		/* L[i] to L[i] + delta */
		for( ; j < total_states && j <= (new_cpu_state[i] + delta); j++ ){
			state_matrix[i][j] = right + new_cpu_state[i] - j;	
		}

		/* L[i] + delta to T */
		for( ; j < total_states; j++) {
			state_matrix[i][j] = 0;
		}

	}
}

/********************************************************************************
 * update_state_weight - updates the state_weight vector
 *
 * sums along every column of state_matrix and stores the result in
 * state_weight. Of course assumes that the state_matrix is updated
 * before this is called.
 ********************************************************************************/
void update_state_weight(void)
{
	int i,j;
	for(j = 0; j < total_states; j++) {
		state_weight[j] = 0;
		for(i=0;i < total_online_cpus; i++){
			state_weight[j] += state_matrix[i][j];
		}
	}
}

/********************************************************************************
 * get_left_distance - get closest state on the left with some state weight
 * @pos - the current state with NO state weight.
 *
 * Get the state on the left (lower) to this state whose state weight is 
 * not zero. -1 if none is found. 
 * Note:
 * a. state_weight is already updated.
 * b. This SHOULD only be used if the weight of pos is 0.
 ********************************************************************************/
int get_left_distance(int pos)
{
	int i;
	int ret = -1;
	for (i = pos - 1; i >= 0; i--){
		if(state_weight[i] != 0){
			ret = i;
			break;
		}
	}
	return ret;
}

/********************************************************************************
 * get_left_distance - get closest state on the right with some state weight
 * @pos - the current state with NO state weight.
 *
 * Get the state on the right (higher) to this state whose state weight is 
 * not zero. -1 if none is found. 
 * Note:
 * a. state_weight is already updated.
 * b. This SHOULD only be used if the weight of pos is 0.
 ********************************************************************************/
int get_right_distance(int pos)
{
	int i;
	int ret = -1;

	for (i = pos + 1; i < total_states; i++) {
		if(state_weight[i] != 0){
			ret = i;
			break;
		}
	}
	return ret;
}

/********************************************************************************
 * closest - returns the closest among left and right.
 * @pos - the current state
 * @left - the state on the left 
 * @right - the state on the right
 * @return the state which is closest. -1 if both are negative.
 *
 * Take the left and right vectors and return the closest among them.
 * left and right are return values of get_left_distance and get_right_distance
 * respectively.
 ********************************************************************************/
int closest(int pos, int left, int right)
{
	if(left == -1 && right == -1){
		return -1;
	}
	if(left == -1){
		return right;
	}
	if(right == -1){
		return left;
	}
	return (pos - left) > (right - pos) ? right : left;
}

/********************************************************************************
 * update_demand_field - update the demand field vector. 
 * @dir - the direction of transition.
 * 
 * initializes the demand field to equal demand, and proxy_source to itself. 
 * Then it checks if any of the states with demand have 0 weight. If true, 
 * based on the demand, finds the state to which it needs to offer its demand
 * and updates proxy_source. 
 *
 * If dir = 1, then the one on the left is preferably chosen
 * dir = -1 the one on the right is chosen
 * dir = 0 the closest is chosen. 
 ********************************************************************************/
void update_demand_field(int dir)
{
	int i;
	int left;
	int right;
	int friend = -1;
	for(i = 0; i < total_states; i++) {
		demand_field[i] = demand[i];
		proxy_source[i] = i;
	}

	for(i = 0; i < total_states; i++) {
		if(demand_field[i] != 0 && state_weight[i] == 0){
			left = get_left_distance(i);
			right = get_right_distance(i);
			if(left == -1 && right == -1){
				continue;
			} if(dir == 1 && left != -1){
				friend = left;
			} else if(dir == 1){
				friend = right;
			} else if(dir == 0 && left == -1){
				friend = right;
			} else if(dir == 0 && right == -1){
				friend = left;
			} else if(dir == 0){
				friend = closest(i,left,right);
			} else if(dir == -1 && right != -1){
				friend = right;
			} else if(dir == -1){
				friend = left;
			}
			if(friend == -1){
				continue;
			}
			/* Closest member inherits the demand */
			demand_field[friend] += demand[i];
			demand_field[i] = 0;
			proxy_source[friend] = i;

		}
	}
}

/********************************************************************************
 * update_winning_procs - update the winning procs vector
 *
 * assumes selected_cpus and state_matrix are updated. for each column,
 * it selects the cell with the max value and whose position in selected_cpus
 * is not 0. and then updates winning_procs. 
 ********************************************************************************/
void update_winning_procs(void)
{
	int i,j;
	int max_val;
	int max;
	int val;
	for(j = 0; j < total_states; j++) {
		max = -1;
		max_val = 0;
		for(i = 0; i < total_online_cpus; i++){
			val = selected_cpus[i] * state_matrix[i][j];

			if(val > max_val){
				max_val = val;
				max = i;
			}
		}
		winning_procs[j] = max;
	}
}

/********************************************************************************
 * get_winning_state - return state to transition to.
 *
 * return the state with the max state_weight * demand_field, 
 * if contended, then the one with the higher winning_proc is chosen.
 ********************************************************************************/
int get_winning_state(void)
{
	int j;
	int max_weight = 0;
	int max_proc = 0;
	int max = -1;
	int weight; 
	for(j = 0; j < total_states; j++) {
		weight = state_weight[j] * demand_field[j];

		if(weight > max_weight){
			max_weight = weight;
			max = j;
			max_proc = state_matrix[winning_procs[j]][j];
			continue;
		} 
		if( weight == max_weight){
			if(state_matrix[winning_procs[j]][j] > max_proc){
				max_weight = weight;
				max = j;
				max_proc = state_matrix[winning_procs[j]][j];
			}
		}
	}
	return max;
}

/********************************************************************************
 * wake_up_procs - wake up total_demand processors, if asleep.
 * @total_demand - total processors required.
 *
 * first count the number of awake processors, if greater than or equal to
 * the required (total_demand) then return.
 * Else, iteratively wake up the proc with min sleep_time 
 ********************************************************************************/
void wake_up_procs(int total_demand)
{
	int awake_total = 0;
	unsigned int min_sleep_time;
	unsigned int wake_up_proc;
	int i,j;
	/* First count awake processors */
	for (i = 0; i < total_online_cpus; i++){
		awake_total += info[i].awake;
	}
	if(awake_total >= total_demand){
		return;
	}
	for(i=0; i<total_demand; i++){
		awake_total = 0;
		min_sleep_time = UINT_MAX;
		wake_up_proc = UINT_MAX;
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
		if(wake_up_proc < total_online_cpus){
			awake_total++;
			info[wake_up_proc].sleep_time = 0;
			info[wake_up_proc].awake = 1;
		}
		if(awake_total >= total_demand)
			break;
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
	int load = 0;
	struct debug_block *p = NULL;
	unsigned int i, j;
	int winner = 0;
	int winner_proc = 0;
	int total_demand = 0;
	int total_iter = 0;
	int total_provided_cpus = 0;
	int total_selected_cpus = 0;
	int direction;

	interval_count++;

	/* Initialize selected cpus and new_cpu_state */
	for (i = 0; i < total_online_cpus; i++) {
		selected_cpus[i] = 1;
		new_cpu_state[i] = cur_cpu_state[i];
	}

	load = get_tasks_load();
	debug("load of system = %d", load);

	/* Compute the total = sum of hints */
	for (j = 0; j < total_states; j++) {
		total += hint_get(j);
	}

	/* Compute CPU's demanded for each state */
	for (j = 0; j < total_states; j++) {
		demand[j] = procs(hint_get(j), total, load);
		total_demand += demand[j];
	}

	total_demand = load;

	wake_up_procs(total_demand);

	direction = transition_direction();

	/* Now for each delta to spend, hold an auction */
	for(total_iter = 0; total_iter < load; total_iter++){

		if(total < MIN_REQUESTS || load <= 0 || delta <= 0 || total_demand <= 0){
			break;
		}
		debug("Iteration %d", total_iter);

		/* Compute the state matrix */
		update_state_matrix(delta,direction);

		update_state_weight();

		update_winning_procs();

		/* Compute the demand field */
		update_demand_field(direction);

		winner = get_winning_state();

		if (winner < 0 || winner >= total_states){
			debug("Auction ended with invalid state");
			break;
		}

		winner_proc = winning_procs[winner];

		if (winner_proc < 0 || winner_proc >= total_online_cpus){
			debug("Auction ended with invalid best proc!");
			break;
		}

		/* This proc should not be assigned again */
		selected_cpus[winner_proc] = 0;

		demand[proxy_source[winner]]--;
		total_demand--;

		debug("Winner is state %d choosing cpu %d", winner,
		      winner_proc);


		total_selected_cpus++;

		/* Subtract that from the delta */
		delta -= ABS(cur_cpu_state[winner_proc] - winner);

		/* Assign the new cpus state to be the winner */
		new_cpu_state[winner_proc] = winner;

		/* Continue the auction if delta > 0  or till all cpus are allocated */
	}

 
	for(i=0; i < total_online_cpus; i++){
		if(total_selected_cpus >= load)
			break;
		if(selected_cpus[i] == 0)
			continue;
		if(info[i].sleep_time == 0){
			total_selected_cpus++;
			selected_cpus[i] = 0;
		}
	}

	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
	}

	for (i = 0; i < total_online_cpus; i++) {
		if (selected_cpus[i] == 1)
			continue;
		new_states[new_cpu_state[i]].cpus++;
		total_provided_cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
	}
	if (total_provided_cpus == 0) {
		new_states[0].cpus++;
		cpu_set(0, new_states[0].cpumask);
		new_cpu_state[0] = 0;
		selected_cpus[0] = 0;
		new_cpu_state[0] = 0;
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
		if (selected_cpus[i] == 0) {
			info[i].sleep_time = 0;
			if (info[i].awake == 0) {
				debug("waking up processor %d", i);
			}
			info[i].awake = 1;
			if (new_cpu_state[i] != cur_cpu_state[i]) {
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
		p->entry.u.mut.cpus_req[j] = demand[j];
		p->entry.u.mut.cpus_given[j] = 0;
	}
	for ( i = 0; i < total_online_cpus; i++ ) {
		p->entry.u.mut.cpus_given[cur_cpu_state[i]]++;
	}
exit_debug:
	put_debug(p);
}
#endif
