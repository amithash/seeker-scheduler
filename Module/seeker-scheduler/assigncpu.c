#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/irq.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include <seeker.h>

#include "assigncpu.h"
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

extern struct state_desc states[MAX_STATES];
extern int max_state_in_system;
extern u64 interval_count;
extern spinlock_t states_lock;

void put_mask_from_stats(struct task_struct *ts)
{
	int new_state = -1;
	struct debug_block *p = NULL;
	int i;
	short ipc = 0;
	int state = 0;
	cpumask_t mask;

	if(spin_is_locked(&states_lock)){
		debug("States is locked... returning");
		return;
	}


#ifdef SEEKER_PLUGIN_PATCH
	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */
	if(ts->inst < INST_THRESHOLD)
		return;

	state = ts->cpustate;
	if(unlikely(state < 0 || state >= max_state_in_system))
		state = 0;

	if(ts->interval != interval_count){
		ts->interval = interval_count;
		ts->inst = 0;
		ts->ref_cy = 0;
		ts->re_cy = 0;
	}

	cpus_clear(mask);
	ipc = IPC(ts->inst,ts->re_cy);
	debug("IPC = %d",ipc);
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
		mask = states[new_state].cpumask;

		if(cpus_empty(mask)){
			debug("mask empty...");
			return;
		}
		#ifdef SEEKER_PLUGIN_PATCH
		ts->cpustate = new_state;
		#endif
		if(spin_is_locked(&states_lock))
			return;
		
		ts->cpus_allowed = mask;
//		set_tsk_need_resched(ts); /* Lazy */
//		set_cpus_allowed(ts,mask); /* Unlazy */
	}

	p = get_debug();
	if(p){
		p->entry.type = DEBUG_SCH;
		#ifdef SEEKER_PLUGIN_PATCH
		p->entry.u.sch.interval = ts->interval;
		#endif
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.cpumask = CPUMASK_TO_UINT(ts->cpus_allowed);
	}
	put_debug(p);
}


