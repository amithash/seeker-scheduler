#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <seeker.h>

#include "state.h"
#include "scpufreq.h"
#include "debug.h"

#define ABS(i) ((i) >= 0 ? (i) : (-1*(i)))
#define div(a,b) ((a) / (b)) + (((a)%(b))<<1 >= (b) ? 1 : 0)

extern int max_allowed_states[NR_CPUS];
extern int cur_cpu_state[NR_CPUS];
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

void choose_layout(int dt)
{
	int cpus;
	int hint[MAX_STATES];
	int count;
	int i,j;
	int total = 0;
	int demand[MAX_STATES];
	short cpus_bitmask[NR_CPUS]={0};
	int req_cpus = 0;
	int delta = dt;
	int load = 0;
	struct debug_block *p = NULL;

	/* RAM is not a problem, cpu cycles are.
	 * so use actual values here rather than
	 * NR_CPUS or MAX_STATES
	 */
	
	interval_count++;
	cpus = num_online_cpus();
	count = get_total_states();
	req_cpus = 0;
	
	p = get_debug();
	if(p){
		p->entry.type = DEBUG_MUT;
		p->entry.u.mut.interval = interval_count;
		p->entry.u.mut.count = count;
	}

	/* Total Hint */
	
	for(i=0;i<count;i++){
		total += hint[i];
		if(p)
			p->entry.u.mut.hint[i] = hint[i];
	}
	for(i=0;i<cpus;i++)
		load += weighted_cpuload(i) >= SCHED_LOAD_SCALE ? 1 : 0;

	/* Num of cpus required for this state 
	 * SUM(demand[]) could be < cpus. 
	 * Make sure to bring down their states. */
	for(j=0;j<count;j++){
		req_cpus += demand[j] = procs(hint[j],total,cpus,load);
		debug("required cpus for state %d = %d",j,demand[j]);
	}

	debug("req_cpus=%d\n",req_cpus);
	if(req_cpus > cpus)
		req_cpus = cpus;

	/* Each state computes the cost of each cpu 
	 * cost = |cpu_cur_state - state |*/
	short cost[MAX_STATES][NR_CPUS];
	for(i=0;i<count;i++){
		for(j=0;j<cpus;j++){
			cost[i][j] = abs(cur_cpu_state[j]-i);
		}
	}

	/* Generate a PDF Matrix */

	/* Pick delta points out of random from this */

	/* After choosing cpus, set the states description
	 * to the new cpumask of that state, the number of
	 * cpus etc, so that assigncpu does not have to do
	 * much work.
	 */

	if(p){
		for(i=0;i<cpus;i++)
			p->entry.u.mut.cpustates[i] = cur_cpu_state[i];
		debug_link(p);
	}
}

