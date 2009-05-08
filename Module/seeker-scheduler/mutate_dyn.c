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

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/
void init_mutator(void)
{
	return;
}

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
	for(b = 1; b <= w; b++){
		max = -1;
		ind = -1;
		for(j=0;j<m;j++){
			if(wt[0][j] <= b && max < demand[j]){
				max = demand[j];
				ind = j;
			}
		}
		if(dyna[1][b-1].f > max){
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
		for(b = 1; b <= w; b++){
			max = -1;
			ind = -1;
			for(j = 0; j < m; j++){
				demand1 = (b - wt[k-1][j]) > 0 ? dyna[k-1][b-wt[k-1][j]].f : 0;
				demand2 = demand1 + demand[j];
				if(wt[k-1][j] <= b && demand1 > 0 && max < demand2){
					max = demand2;
					ind = j;
				}
			}
			if(dyna[k][b-1].f > max){
				dyna[k][b].f = dyna[k][b-1].f;
			} else {
				dyna[k][b].f = max;
				dyna[k][b].sol = ind;
			}
		}
	}

	/* backtrack */
	b = w;
	for(k = n; k > 0; k--){
		while(b > 0){
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

	interval_count++;

	/* Compute the total = sum of hints */
	for (j = 0; j < total_states; j++) {
		demand[j] = hint_get(j) + 1;
	}

	mck(total_online_cpus, total_states, delta);
 
	for (j = 0; j < total_states; j++) {
		new_states[j].cpus = 0;
		cpus_clear(new_states[j].cpumask);
	}

	for (i = 0; i < total_online_cpus; i++) {
		new_states[new_cpu_state[i]].cpus++;
		cpu_set(i, new_states[new_cpu_state[i]].cpumask);
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
