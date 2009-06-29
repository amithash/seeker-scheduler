#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

/********************************************************************************
 * 				Function Declarations				*
 ********************************************************************************/

void state_change(struct work_struct *w);

/********************************************************************************
 * 				Global Prototyprs 				*
 ********************************************************************************/

/* max state of each cpu */
static DEFINE_PER_CPU(short int, max_state);

/* current state of each cpu */
static DEFINE_PER_CPU(short int, cur_state);

/* The state change work struct */
static DECLARE_DELAYED_WORK(state_change_timer, state_change);

/* contains total online cpus */
static int num_cpus = NR_CPUS;

/* The last cpu changed */
static int last_cpu = 0;

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

static int interval = 1;

/********************************************************************************
 * 				Macros						*
 ********************************************************************************/

/* get the max state for cpu */
#define MAX_STATE(cpu) (per_cpu(max_state,(cpu)))

/* get the cur state for cpu */
#define CUR_STATE(cpu) (per_cpu(cur_state,(cpu)))

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * state_change - the delayed work which changes cpu states.
 * @w - the work struct for state_change_timer (Not used)
 *
 * This is called every interval seconds. 
 * display the state of the last cpu modified. increment last_cpu to point
 * to the next cpu (circular increment). next do a circular increment of the
 * state of the cpu indicated by last_cpu, and finally change the frequency
 * of the freq by calling set_freq. 
 *******************************************************************************/
void state_change(struct work_struct *w)
{
	info("CPU: %d CurState=%d", last_cpu, CUR_STATE(last_cpu));
	last_cpu = (last_cpu + 1) % num_cpus;
	CUR_STATE(last_cpu) = (CUR_STATE(last_cpu) + 1) % MAX_STATE(last_cpu);
	set_freq(last_cpu, CUR_STATE(last_cpu));
	schedule_delayed_work(&state_change_timer, jiffies + (interval * HZ));
}

/*******************************************************************************
 * init_test_scpufreq - Init function for test_scpufreq
 * @return - 0 on success, error code otherwise.
 *
 * set the initial values for cur_state, the value for max_state of each cpu.
 * and finally reflect that by calling set_freq. Finally initialize the 
 * delayed work which is used as a timer. 
 *******************************************************************************/
static int init_test_scpufreq(void)
{
	int i;
	num_cpus = num_online_cpus();

	for (i = 0; i < num_cpus; i++) {
		MAX_STATE(i) = get_max_states(i);
		CUR_STATE(i) = 0;
		set_freq(i, 0);
	}
	init_timer_deferrable(&state_change_timer.timer);
	schedule_delayed_work(&state_change_timer, jiffies + (interval * HZ));
	return 0;
}

/*******************************************************************************
 * exit_test_scpufreq - exit routine for test_scpufreq
 *
 * remove all delayed work and exit cleanly. 
 *******************************************************************************/
static void exit_test_scpufreq(void)
{
	cancel_delayed_work(&state_change_timer);
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_param(interval, int, 0444);
MODULE_PARM_DESC(interval, "Set the interval at which to change.");

module_init(init_test_scpufreq);
module_exit(exit_test_scpufreq);

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpufreq cpufreq governor");
MODULE_LICENSE("GPL");

