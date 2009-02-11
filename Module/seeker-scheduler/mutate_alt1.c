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
#include "debug.h"

/* NO LONGER DEVELOPED. WILL BE DELETED */

extern struct state_desc states[MAX_STATES];

/* Runtime params decided at load time. */
extern unsigned int total_states;
extern int total_online_cpus;

extern int cur_cpu_state[NR_CPUS];
static int new_cpu_state[NR_CPUS];

u64 interval_count;

struct proc_info {
	unsigned int sleep_time;
	unsigned int awake;
};

static struct proc_info info[NR_CPUS];

inline int procs(int hints, int total, int total_load);

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
		info[i].awake = 1;
	}
}

void choose_layout(int delta)
{
	int cpus_demanded[MAX_STATES];
	int i, j;
	int load = 0;
	int work_required = 0;
	int work_done = 0;
	int winning_cpu;
	int winning_cpu_state = 0;
	int total = 0;
	struct debug_block *p = NULL;
	int low = 0;
	int high = 0;
	int stop = 0;

	interval_count++;

	for (i = 0; i < total_online_cpus; i++) {
		new_cpu_state[i] = cur_cpu_state[i];
		load += (weighted_cpuload(i) >= SCHED_LOAD_SCALE ? 1 : 0);
		work_done += cur_cpu_state[i];
	}
	for (j = 0; j < total_states; j++) {
		total += states[j].demand;
	}
	for (j = 0; j < total_states; j++) {
		cpus_demanded[j] = procs(states[j].demand, total, load);
		work_required += (cpus_demanded[j] * j);
		stop += cpus_demanded[j];
		debug("required cpus for state %d = %d", j, cpus_demanded[j]);
	}

	while (delta > 0 && stop > 0) {
		stop--;
		winning_cpu = 0;
		winning_cpu_state = new_cpu_state[0];

		/* Need a higher cpu find lowest and increase it by a state */
		if (work_required > work_done) {
			/* There is a possibility of infinite
			 * oscillations (Atleast till delta of course)
			 * but we do not want that, so check if the other
			 * code section has been executed */
			if (high == 1)
				break;
			low = 1;
			for (i = 1; i < total_online_cpus; i++) {
				if (new_cpu_state[i] < winning_cpu_state) {
					winning_cpu_state = new_cpu_state[i];
					winning_cpu = i;
				}
			}
			if (new_cpu_state[winning_cpu] < (total_states - 1)) {
				new_cpu_state[winning_cpu]++;
				delta--;
				work_done++;
			}
		} else if (work_required < work_done) {	/* Lower */
			/* same as above */
			if (low == 1)
				break;
			high = 1;
			for (i = 1; i < total_online_cpus; i++) {
				if (new_cpu_state[i] > winning_cpu_state) {
					winning_cpu_state = new_cpu_state[i];
					winning_cpu = i;
				}
			}
			if (new_cpu_state[winning_cpu] > 0) {
				new_cpu_state[winning_cpu]--;
				work_done--;
				delta++;
			}
		} else {
			/* They are equal, best case reached,
			 * break */
			break;
		}
	}

	/* Record the changes in the debug log */
	p = get_debug();
	if (p) {
		p->entry.type = DEBUG_MUT;
		p->entry.u.mut.interval = interval_count;
		p->entry.u.mut.count = total_states;
	}
	mark_states_inconsistent();
	for (j = 0; j < total_states; j++) {
		states[j].cpus = 0;
		cpus_clear(states[j].cpumask);
		if (p) {
			p->entry.u.mut.cpus_req[j] = cpus_demanded[j];
			p->entry.u.mut.cpus_given[j] = 0;
		}
		states[j].demand = 0;
	}

	for (i = 0; i < total_online_cpus; i++) {
		if (p)
			p->entry.u.mut.cpus_given[new_cpu_state[i]]++;
		states[new_cpu_state[i]].cpus++;
		cpu_set(i, states[new_cpu_state[i]].cpumask);
	}
	mark_states_consistent();
	put_debug(p);
	/* This is purposefully put in a different loop 
	 * due to the intereference with put_debug();
	 */
	for (i = 0; i < total_online_cpus; i++) {
		if (new_cpu_state[i] != cur_cpu_state[i]) {
			cur_cpu_state[i] = new_cpu_state[i];
			set_freq(i, new_cpu_state[i]);
		}
	}
}


