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

#define NR_STATES MAX_STATES
#define MAX_DELTA (NR_CPUS * (MAX_STATES - 1))

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

#if MUTATOR_TYPE == DYNAMIC_PROGRAMMING_BASED_MUTATOR
#warning "You are using the experimental dynamic programming based mutator"
static struct state_desc new_states[MAX_STATES];

/* Selected states for cpus */
static int new_cpu_state[NR_CPUS];

/* Computed demand for each state */
static int demand[MAX_STATES];

/* Mutator interval */
u64 interval_count;

/* sleep_time - intervals the cpu has been sleeping 
 * awake - 1 if cpu is awake, 0 otherwise 
 */
struct proc_info {
	unsigned int sleep_time;
	unsigned int awake_time;
	unsigned int awake;
};

/* Proc info for each cpu */
static struct proc_info info[NR_CPUS];

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

void init_mutator(void)
{
	int i;
	for (i = 0; i < NR_CPUS; i++) {
		info[i].sleep_time = 0;
		info[i].awake_time = 1;
		info[i].awake = 1;
	}
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


/* Implementation of the dynamic programming solution
 * for the multiple knap sack problem whose algorithm
 * is provided in:
 *
 * Multiple Choice Knapsack Functions
 * James C. Bean
 * Department of Industrial and Operations Engineering
 * The University of Michigan
 * Ann, Arbor, MI 48109-2117
 * January 4 1988
 *
 * Number of classes = n
 * Differences:
 * 1. All classes have equal number of elements = m;
 * 2. The value of element x_ij (jth element in class i)
 * has a value val_j => the value of all elements 
 * x_ij ( 1 <= i <= n ) are equal. 
 */
void mck(int n, int m, int w)
{
	int i;
	int j;
	int b;
	int k;
	int max;
	int ind;
	int demand1, demand2;
	struct struct_f {
		int f;
		int sol;
	};
	static struct struct_f dyna[NR_CPUS+1][MAX_DELTA+1];
	static int wt[NR_CPUS][NR_STATES];

	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			wt[i][j] = ABS(j - cur_cpu_state[i]);
		}
	}

	for(i=0;i<=n;i++){
		for(j = 0; j <= w; j++){
			dyna[i][j].f = 0;
			dyna[i][j].sol = -1;
		}
	}

	/* init first proc */
	for(b = 0; b <= w; b++){
		max = -1;
		ind = -1;
		for(j=0;j<m;j++){
			if(wt[0][j] <= b && max < demand[j]){
				max = demand[j];
				ind = j;
			}
		}
		if(b > 0 && dyna[1][b-1].f > max){
			dyna[1][b].f = dyna[1][b-1].f;
		} else {
			dyna[1][b].f = max;
		}
		if(max > 0){
			dyna[1][b].sol = ind;
		}
	}

	/* do for all! */
	for(k = 2; k <= n; k++){
		for(b = 0; b <= w; b++){
			max = -1;
			ind = -1;
			for(j = 0; j < m; j++){
				demand1 = (b - wt[k-1][j]) >= 0 ? dyna[k-1][b-wt[k-1][j]].f : 0;
				demand2 = demand1 + demand[j];
				if(wt[k-1][j] <= b && demand1 > 0 && max < demand2){
					max = demand2;
					ind = j;
				}
			}
			if(b > 0 && dyna[k][b-1].f > max){
				dyna[k][b].f = dyna[k][b-1].f;
			} else if(max > 0){
				dyna[k][b].f = max;
				dyna[k][b].sol = ind;
			}
		}
	}

	/* backtrack */
	b = w;
	for(k = n; k > 0; k--){
		while(b >= 0){
			if(dyna[k][b].sol >= 0){
				new_cpu_state[k-1] = dyna[k][b].sol;
				b = b - wt[k-1][dyna[k][b].sol];
				break;
			}
			b--;
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
	struct debug_block *p = NULL;
	unsigned int i, j;
	int total_cpus_req = 0;
	int cpus_req[MAX_STATES];
	int total = 0;
	int cpu_to_sleep[NR_CPUS];
	int cpus_given = 0;

	interval_count++;

	total_cpus_req = get_tasks_load();

	/* Compute the total = sum of hints */
	for (j = 0; j < total_states; j++) {
		demand[j] = hint_get(j) + 1;
		total += (demand[j] - 1);
	}

	for (j = 0; j < total_states; j++) {
		cpus_req[j] = procs(demand[j]-1,total,total_cpus_req);
	}

	if(total_cpus_req == 0){
		total_cpus_req = 1;
		cpus_req[0] = 1;
	}

	wake_up_procs(total);

	mck(total_online_cpus, total_states, delta);
 
	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
	}

	for (i = 0; i < total_online_cpus; i++) {
		if(new_states[new_cpu_state[i]].cpus > cpus_req[new_cpu_state[i]]){
			new_cpu_state[i] = 0;
			cpu_to_sleep[i] = 1;
			continue;
		}
		cpu_to_sleep[i] = 0;
		new_states[new_cpu_state[i]].cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
		cpus_given++;
	}
	if(cpus_given == 0){
		cpu_to_sleep[0] = 0;
		new_states[0].cpus = 1;
		cpu_set(0, new_states[0].cpumask);
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
		if (new_cpu_state[i] != cur_cpu_state[i]) {
			set_freq(i, new_cpu_state[i]);
		}
		if(cpu_to_sleep[i]){
			info[i].awake = 0;
			info[i].awake_time = 0;
			info[i].sleep_time++;
		} else {
			info[i].awake = 1;
			info[i].sleep_time = 0;
			info[i].awake_time++;
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
