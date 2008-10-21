#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "state.h"
#include "scpufreq.h"
#include "hint.h"
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

int state_of_cpu[NR_CPUS] = {0};
extern struct state_desc states[MAX_STATES];

/* Many people can read the ds.
 * but have to stay off reading 
 * if it is being written
 */
rwlock_t state_of_cpu_lock = RW_LOCK_UNLOCKED;

/* MUST be called by the init module */
void init_system(void)
{
//	rwlock_init(&state_of_cpu_lock);
}

/* freq_state module MUST Call this
 * to get the state of cpu ds
 */
void get_state_of_cpu(void)
{
//	write_lock(&state_of_cpu_lock);
}
EXPORT_SYMBOL_GPL(get_state_of_cpu);

/* Once freq_state is done, it MUST
 * call this to release the lock.
 */
void put_state_of_cpu(void)
{
//	write_unlock(&state_of_cpu_lock);
}
EXPORT_SYMBOL_GPL(put_state_of_cpu);

void put_mask_from_stats(struct task_struct *ts)
{
	int new_state;
	struct debug_block *p = NULL;
	int i;
	short ipc = 0;
	short max_states = get_total_states();

#ifdef SEEKER_PLUGIN_PATCH
	int state = ts->cpustate;
	/* Do not try to estimate anything
	 * till INST_THRESHOLD insts are 
	 * executed. Hopefully avoids messing
	 * with short lived tasks.
	 */
	if(ts->inst < INST_THRESHOLD)
		return;
#else
	int state = 0;
#endif

	new_state = state;

#ifdef SEEKER_PLUGIN_PATCH
	ipc = IPC(ts->inst,ts->re_cy);
#endif
	/*up*/
	if(ipc >= IPC_0_750){
		for(i=state+1;i<max_states;i++){
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

	if(new_state == state){
		goto exit;
	}

	/* Update hint */
	hint_dec(state);
	hint_inc(new_state);
	cpus_clear(ts->cpus_allowed);
	cpus_or(ts->cpus_allowed,
		ts->cpus_allowed,
		states[new_state].cpumask);
	exit:
	p = get_debug();
	if(p){
		p->entry.type = DEBUG_SCH;
		#ifdef SEEKER_PLUGIN_PATCH
		p->entry.u.sch.interval = ts->interval;
		#endif
		p->entry.u.sch.pid = ts->pid;
		p->entry.u.sch.cpumask = CPUMASK_TO_UINT(ts->cpus_allowed);
		debug_link(p);
	}
}


