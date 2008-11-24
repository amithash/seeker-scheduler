#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>

#include <seeker.h>

#include "state.h"
#include "seeker_cpufreq.h"
#include "assigncpu.h"
#include "mutate.h"

/* Temp note:
 * The #if 0's sections are the working tested part.
 * The #else's are the new better untested part.
 * Once you test the better part, remove these #if 0 
 * sections AND this comment!
 */

extern int total_online_cpus;
int max_state_possible[NR_CPUS] = {0};
unsigned int max_state_in_system = 0;
int cur_cpu_state[NR_CPUS] = {0};
struct state_desc states[MAX_STATES];
#if 0
static DEFINE_SPINLOCK(states_lock);
static unsigned long states_irq_flags;
#else
struct state_sane_t state_sane;
#endif


void hint_inc(int state)
{
	states[state].demand++;
//	atomic_inc((void *)&(states[state].demand));
}

void hint_dec(int state)
{
	states[state].demand--;
//	atomic_dec((void *)&(states[state].demand));
}
void mark_states_inconsistent(void)
{
#if 0
	if(spin_is_locked(&states_lock))
		warn("Recursive spin lock avoided");
	else
		spin_lock_irqsave(&states_lock,states_irq_flags);
#else
	unsigned long irq_flags;
	spin_lock_irqsave(&(state_sane.lock),irq_flags);
	state_sane.val = 0;
	spin_unlock_irqrestore(&(state_sane.lock),irq_flags);
#endif
}
void mark_states_consistent(void)
{
#if 0
	if(spin_is_locked(&states_lock))
		spin_unlock_irqrestore(&states_lock,states_irq_flags);
	else
		warn("Recursive spin unlock avoided");
#else
	unsigned long irq_flags;
	spin_lock_irqsave(&(state_sane.lock),irq_flags);
	state_sane.val = 1;
	spin_unlock_irqrestore(&(state_sane.lock),irq_flags);

#endif
}

int is_states_consistent(void)
{
#if 0
	return !spin_is_locked(&states_lock);
#else
	int val;
	unsigned long irq_flags;
	spin_lock_irqsave(&(state_sane.lock),irq_flags);
	val = state_sane.val;
	spin_unlock_irqrestore(&(state_sane.lock),irq_flags);
	return val;
#endif
}


int init_cpu_states(unsigned int how)
{
	int i;
#if 0
	spin_lock_init(&states_lock);
#else
	spin_lock_init(&(state_sane.lock));
	state_sane.val = 1; /* Mark states as sane */
	/* Actually it is not going to be, but the timer
	 * is not going to be initialized and the probes
	 * are going to be registered only after this function
	 * has completed successfully, and hence it is safe to
	 * mark them as sane, */
#endif

	for(i=0;i<total_online_cpus;i++){
		max_state_possible[i] = get_max_states(i);
		info("Max state for cpu %d = %d",i,max_state_possible[i]);
		if(max_state_in_system < max_state_possible[i])
			max_state_in_system = max_state_possible[i];
	}
	for(i=0;i<max_state_in_system;i++){
		states[i].state = i;
		states[i].cpus = 0;
		states[i].demand = 0;
		cpus_clear(states[i].cpumask);
	}

	switch(how){
		case ALL_HIGH:
			states[max_state_in_system-1].cpus = total_online_cpus;
			for(i=0;i<total_online_cpus;i++){
				cpu_set(i,states[max_state_in_system-1].cpumask);
				cur_cpu_state[i] = max_state_possible[i]-1;
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		case ALL_LOW:
			states[0].cpus = total_online_cpus;
			for(i=0;i<total_online_cpus;i++){
				cpu_set(i,states[0].cpumask);
				cur_cpu_state[i] = 0;
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		case BALANCE:
			states[max_state_in_system-1].cpus = total_online_cpus >> 1;
			states[0].cpus = total_online_cpus - (total_online_cpus>>1);
			for(i=0;i<states[max_state_in_system-1].cpus;i++){
				cpu_set(i,states[0].cpumask);
				cur_cpu_state[i] = 0;
				set_freq(i,cur_cpu_state[i]);
			}
			for(;i<total_online_cpus;i++){
				cpu_set(i,states[max_state_in_system-1].cpumask);
				cur_cpu_state[i] = max_state_possible[i]-1;
				set_freq(i,cur_cpu_state[i]);
			}
			break;
		default:
			states[max_state_in_system-1].cpus = total_online_cpus;
			for(i=0;i<total_online_cpus;i++){
				cpu_set(i,states[max_state_in_system-1].cpumask);
				cur_cpu_state[i] = max_state_possible[i]-1;
				set_freq(i,cur_cpu_state[i]);
			}
			break;
	}
	return 0;
}

