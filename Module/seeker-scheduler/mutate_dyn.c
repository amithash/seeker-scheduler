/******************************************************************************\
 * FILE: mutate_dyn.c
 * DESCRIPTION: Mutator implementing the memorization based dynamic programming
 * implementiation to solve the delta mutation problem as a multiple-choice
 * knapsack. Presented in:
 *
 * Multiple Choice Knapsack Functions
 * James C. Bean
 * Department of Industrial and Operations Engineering
 * The University of Michigan
 * Ann, Arbor, MI 48109-2117
 * January 4 1988
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include <linux/sched.h>
#include <linux/cpumask.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "state.h"
#include "load.h"
#include "log.h"
#include "nrtasks.h"
#include "mutate.h"

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

extern struct proc_info info[NR_CPUS];

/* Mutator interval */
extern u64 interval_count;

/********************************************************************************
 * 			    External functions					*
 ********************************************************************************/

void wake_up_procs(int req_cpus);

void retire_procs(int req_cpus, int *put_to_sleep, int *cpu_awake_proxy);


/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

static struct state_desc new_states[MAX_STATES];

static int cpu_awake_proxy[NR_CPUS];

static int put_to_sleep[NR_CPUS];

/* Selected states for cpus */
static int new_cpu_state[NR_CPUS];

/* Computed demand for each state */
static int demand[MAX_STATES];


/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

static void delta_based_demand_transform(int cpus, int delta)
{
	int can_win[NR_STATES] = {0};
	int i,j;
	int left, right;
	int friend;

	/* Evaluate states which can never win under delta */
	for(j = 0; j < total_states; j++){
		can_win[j] = 0;
		for(i=0;i<cpus;i++){
			if(ABS((cur_cpu_state[cpu_awake_proxy[i]] - j)) <= delta){
				can_win[j] = 1;
				break;
			}
		}
	}

	/* Transfer demand of states which cannot win to nearby states */
	for(j = 0; j < total_states; j++){
		if(can_win[j] == 1 || demand[j] == 0)
			continue;

		left = right = -1;
		for(i = j; i < total_states; i++){
			if(can_win[i] == 1){
				right = i;
				break;
			}
		}
		for(i = j; i >= 0; i--){
			if(can_win[i] == 1){
				left = i;
				break;
			}
		}
		if(left == -1) {
			friend = right;
		} else if(right == -1) {
			friend = left;
		} else if(ABS(left - j) == ABS(right - j)){
			friend = demand[left] > demand[right] ? left : right;
		} else {
			friend = ABS(left - j) < ABS(right - j) ? left : right;	
		}
		demand[friend] += demand[j];
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
static void mck(int n, int m, int w)
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
			wt[i][j] = ABS(j - cur_cpu_state[cpu_awake_proxy[i]]);
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
				new_cpu_state[cpu_awake_proxy[k-1]] = dyna[k][b].sol;
				b = b - wt[k-1][dyna[k][b].sol];
				break;
			}
			b--;
		}
	}
}


/********************************************************************************
 * mem_dynamic_prog - The mutator called every mutator interval.
 * @delta - The delta of the system chosen at module insertion. 
 * @Side effects - Changes cur_cpu_state and states field during which the states
 * 		   will be incosistent. 
 *
 * Chooses the layout based on constraints like delta and demand. Will also set
 * the hints to 0 so the next interval will be fresh. The function is rather 
 * big, so it is explained inline. 
 ********************************************************************************/
void mem_dynamic_prog(int delta)
{
	struct log_block *p = NULL;
	unsigned int i, j;
	int total_cpus_req = 0;
	int total = 0;

	interval_count++;

	/* Init structure */
	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
	}

	total_cpus_req = get_tasks_load();

	/* demand is hint + 1 as the algo cannot stand 0 demand! */
	for (j = 0; j < total_states; j++) {
		demand[j] = hint_get(j) + 1;
	}

	/* Suicide if req procs is 0 */
	if(total_cpus_req == 0){
		total_cpus_req = 1;
	}
	total = total_cpus_req;

	/* Wake up/put to sleep req procs */
	wake_up_procs(total_cpus_req);
	retire_procs(total_cpus_req,put_to_sleep,cpu_awake_proxy);

	delta_based_demand_transform(total, delta);

	/* Perform the dynamic programming algo to solve this as a MCKP */
	mck(total, total_states, delta);

	/* Set up the states */
	for(i = 0; i < total_online_cpus; i++){
		if(info[i].awake == 0)
		      continue;
		new_states[new_cpu_state[i]].cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
	}

	/* Commit states */
	write_seqlock(&states_seq_lock);
	for (j = 0; j < total_states; j++) {
		states[j].cpumask = new_states[j].cpumask;
		states[j].cpus = new_states[j].cpus;
		hint_clear(j);
	}
	write_sequnlock(&states_seq_lock);

	for (i = 0; i < total_online_cpus; i++) {
		if(put_to_sleep[i] == 1){
			set_freq(i, 0);
		}
		if(info[i].awake == 0){
			info[i].sleep_time++;
			continue;
		}
		if (new_cpu_state[i] != cur_cpu_state[i]) {
			set_freq(i, new_cpu_state[i]);
		}
	}

	/* Update debug log. by now, cur_cpu_state will be updated. */
	p = get_log();
	if(!p) {
		goto exit_debug;
	}
	p->entry.type = LOG_MUT;
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
	put_log(p);
}

