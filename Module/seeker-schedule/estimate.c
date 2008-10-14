#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include "../../Module/scpufreq.h"
#include "hint.h"

int state_of_cpu[NR_CPUS] = {0};

/* Many people can read the ds.
 * but have to stay off reading 
 * if it is being written
 */
rwlock_t state_of_cpu_lock = RW_LOCK_UNLOCKED;

/* MUST be called by the init module */
void init_system(void)
{
	rwlock_init(&state_of_cpu_lock);
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
	unsigned int pot_states[NR_CPUS];
	int cpus = num_online_cpus();
	int cpu = smp_processor_id();
	cpumask_t mask,tmp_mask;
	int state = 0;
	int new_state = 0;
	unsigned int new_select = -1;
	int i;

	cpus_clear(mask);
	cpus_clear(tmp_mask);
	/*XXX FIXME 
	 * I am doing something very simple. 
	 * Checking for IPC >= 1.
	 * Because I do not want to divide.
	 */
	/* XXX FIXME
	 * Find out what to use... cy_re or cy_ref 
	 */

	read_lock(&state_of_cpu_lock);
	state = state_of_cpu[cpu];
	read_unlock(&state_of_cpu_lock);

#ifdef SEEKER_PLUGIN_PATCH
	if(ts->inst >= ts->ref_cy){
#endif
		new_state = state + 1;
		if(new_state >= MAX_STATES)
			new_state--;
#ifdef SEEKER_PLUGIN_PATCH
	}
#endif

#ifdef SEEKER_PLUGIN_PATCH
	if(ts->inst <= (ts->ref_cy >> 1)){
#endif
		new_state = state - 1;
		if(new_state < 0)
			new_state = 0;
#ifdef SEEKER_PLUGIN_PATCH
	}
#endif
	
	/* No change needed or possible. 
	 * just continue 
	 */

	if(new_state == state)
		return;

	/* Update hint */
	hint_dec(state);
	hint_inc(new_state);

	for(i=0;i<cpus;i++){
		pot_states[i] = (state_of_cpu[i] - state) * \
			(state_of_cpu[i]-state);

		if(pot_states[i] == 0)
			tmp_mask = cpumask_of_cpu(i);
			cpus_or(mask,mask,tmp_mask);
	}
	/* Targetted cpus with such a state exists */

	if(!cpus_empty(mask)){
		cpus_clear(ts->cpus_allowed);
		cpus_or(ts->cpus_allowed,ts->cpus_allowed,mask);
		return;
	}

	/* Nope, take min */
	for(i=0;i<cpus;i++){
		if(new_select > pot_states[i]){
			new_select = pot_states[i];
			mask = cpumask_of_cpu(i);
		}
		else if(new_select == pot_states[i]){
			tmp_mask = cpumask_of_cpu(i);
			cpus_or(mask,mask,tmp_mask);
		}
	}
	if(!cpus_empty(mask)){
		cpus_clear(ts->cpus_allowed);
		cpus_or(ts->cpus_allowed,ts->cpus_allowed,mask);
		return;
	}
	/* If mask is sill empty, Leave the cpus_allowed
	 * allowed. This will never happen, but even if it
	 * does at worst, the processes does not benifit from
	 * the dynamic scheduling, rather than make it processor
	 * -less
	 */
}


