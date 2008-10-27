#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/irq.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "state.h"
#include "scpufreq.h"
#include "debug.h"

/* Works for a max of 32 processors */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

/* IPC of 8 Corrospondents to IPC of 1.0. */
#define IPC(inst,cy) ((cy)>0) ? ((inst) << 3)/(cy) : 0
#define IPC_0_000 0
#define IPC_0_125 1
#define IPC_0_250 2
#define IPC_0_375 3
#define IPC_0_500 4
#define IPC_0_625 5
#define IPC_0_750 6
#define IPC_0_875 7
#define IPC_1_000 8

/* Keep the threshold at 1M Instructions
 * This removes artifcats from IPC and 
 * removes IPC Computation for small tasks
 */
#define INST_THRESHOLD 1000000

extern struct state_desc states[MAX_STATES];
extern int max_state_in_system;
extern u64 interval_count;

void put_mask_from_stats(struct task_struct *ts)
{
	int new_state = -1;
	struct debug_block *p = NULL;
	int i;
	short ipc = 0;
	cpumask_t mask;

	/* Do not do anything to the init task! */
	if(ts->pid == 0)
		return;


#ifdef SEEKER_PLUGIN_PATCH
	int state;
	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */
	if(ts->inst < INST_THRESHOLD)
		return;
	if(ts->interval == interval_count)
		state = ts->cpustate;
	else{
		state = -1;
		ts->interval = interval_count;
		ts->inst = 0;
		ts->ref_cy = 0;
		ts->re_cy = 0;
	}

#else
	int state = 0;
#endif
	cpus_clear(mask);

#ifdef SEEKER_PLUGIN_PATCH
	ipc = IPC(ts->inst,ts->re_cy);
#endif
	/*up*/
	if(ipc >= IPC_0_750){
		for(i=state+1;i<max_state_in_system;i++){
			if(states[i].cpus > 0){
				new_state = i;
				break;
			}
			if(i-state > 2)
				break;
		}
	}
	/*down*/
	if(ipc <= IPC_0_500){
		for(i=state-1;i>=0;i--){
			if(states[i].cpus > 0){
				new_state = i;
				break;
			}
			if(state-i > 2)
				break;
		}
	}


	hint_inc(new_state);

	if(new_state != state){
		if(new_state == -1)
			new_state = state;
		cpus_or(mask,mask,states[new_state].cpumask);
		/* Done here without a lock. If cpumask has become
		 * empty in the process, then return.
		 */
		if(cpus_empty(mask))
			return;
		set_cpus_allowed(ts,mask);
	}

	/* Update hint */

	p = get_debug();
	if(p){
		p->entry.type = DEBUG_SCH;
		#ifdef SEEKER_PLUGIN_PATCH
		p->entry.u.sch.interval = ts->interval;
		#endif
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.cpumask = CPUMASK_TO_UINT(ts->cpus_allowed);
	}
}


