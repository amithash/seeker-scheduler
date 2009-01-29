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
#include "seeker_cpufreq.h"
#include "debug.h"

/* Works for a max of 32 processors */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

/* IPC of 8 Corrospondents to IPC of 1.0. */
#define IPC(inst,cy) div(((inst)<<3),(cy))
#define IPC_0_000 0
#define IPC_0_125 1
#define IPC_0_250 2
#define IPC_0_375 3
#define IPC_0_500 4
#define IPC_0_625 5
#define IPC_0_750 6
#define IPC_0_875 7
#define IPC_1_000 8

#define IPC_HIGH IPC_0_750
#define IPC_LOW  IPC_0_500


extern struct state_desc states[MAX_STATES];
extern int total_states;
extern u64 interval_count;
extern int cur_cpu_state[NR_CPUS];
extern int static_layout;
extern int low_state;
extern int high_state;
extern int mid_state;
extern int disable_scheduling;

void put_mask_from_stats(struct task_struct *ts)
{
	int new_state = -1;
	struct debug_block *p = NULL;
	int i;
	short ipc = 0;
	int state = 0;
	int this_cpu;
	int state_req = 0;
	cpumask_t mask = CPU_MASK_NONE;
	
#ifdef SEEKER_PLUGIN_PATCH
	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */

	if(ts->inst < INST_THRESHOLD)
		return;
#endif

	this_cpu = smp_processor_id();

#ifdef SEEKER_PLUGIN_PATCH
	switch(ts->fixed_state){
	case 0: state_req = state = low_state;
		break;
	case 1: state_req = state = mid_state;
		break;
	case 2: state_req = state = high_state;
		break;
	default:
		ipc = IPC(ts->inst,ts->re_cy);
		if(ts->cpustate != cur_cpu_state[this_cpu])
			ts->cpustate = cur_cpu_state[this_cpu];
	
		state_req = state = ts->cpustate;
		break;
	}
#endif
	/*up*/
	if(ipc >= IPC_HIGH){
		if(state < total_states-1)
			state_req = state+1;
		else
			state_req = total_states-1;

		for(i=state+1;i<total_states;i++){
			if(states[i].cpus > 0){
				new_state = i;
				break;
			}
		}
	}
	/*down*/
	if(ipc <= IPC_LOW){
		if(state > 0)
			state_req = state-1;
		else
			state_req = 0;

		for(i=state-1;i>=0;i--){
			if(states[i].cpus > 0){
				new_state = i;
				break;
			}
		}
	}
	/* Do not worry about incrementing hint. 
	 * for static layouts. They will not be used.
	 */
	if(static_layout == 0)
		hint_inc(state_req);
	
	/* If IPC_LOW < IPC < IPC_HIGH maintain this state */
	if(new_state == -1)
		new_state = state;

	if(new_state != state){
		/* There is no way to do this atomically except to
		 * hold a lock which just screws performance. 
		 * (Note this section is not only contended with mutate, but
		 * also by other schedules on other CPUS!)
		 * So do it non-atomically, by first getting the mask of the
		 * state required, then assign it to the cpus_allowed ONLY
		 * if the states are consistant AND the mask is NOT empty! 
		 * else leave it as it is */
		mask = states[new_state].cpumask;

		/* if we are using a static_layout there is no need to be so paranoid! */
		if(!static_layout && is_states_consistent() && !cpus_empty(mask)){
			/* Do not touch cpumask if scheduling is disabled */
			if(!disable_scheduling)
				ts->cpus_allowed = mask;
			#ifdef SEEKER_PLUGIN_PATCH
			ts->cpustate = new_state;
			#endif
		} else {
			return;
		}
	}

	p = get_debug();
	if(p){
		p->entry.type = DEBUG_SCH;
		#ifdef SEEKER_PLUGIN_PATCH
		p->entry.u.sch.interval = ts->interval;
		p->entry.u.sch.inst = ts->inst;
		#endif
		p->entry.u.sch.ipc = ipc;
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.state_req = state_req;
		p->entry.u.sch.state_given = new_state;
		p->entry.u.sch.cpu = this_cpu;
	}
	put_debug(p);
#ifdef SEEKER_PLUGIN_PATCH

	/* Start over. Forget the IPC... */
	ts->interval = interval_count;
	ts->inst = 0;
	ts->ref_cy = 0;
	ts->re_cy = 0;
#endif
}

