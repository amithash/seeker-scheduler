#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpufreq cpufreq governor");
MODULE_LICENSE("GPL");

static DEFINE_PER_CPU(short int,max_state);
static DEFINE_PER_CPU(short int,cur_state);
#define MAX_STATE(cpu) (per_cpu(max_state,(cpu)))
#define CUR_STATE(cpu) (per_cpu(cur_state,(cpu)))

static struct timer_list state_change_timer;
static int total_cpus = NR_CPUS;

static int last_cpu = 0;
static int interval = 1;

void state_change(unsigned long param)
{
	info("CPU: %d CurState=%d",last_cpu,CUR_STATE(last_cpu));
	last_cpu = (last_cpu + 1) % total_cpus;
	CUR_STATE(last_cpu) = (CUR_STATE(last_cpu)+1)%MAX_STATE(last_cpu);
	set_freq(last_cpu,CUR_STATE(last_cpu));
	mod_timer(&state_change_timer,jiffies+(interval*HZ));
}

static int init_test_scpufreq(void)
{
	int i;
	total_cpus = num_online_cpus();
	
	for(i=0;i<total_cpus;i++){
		MAX_STATE(i) = get_max_states(i);
		CUR_STATE(i) = 0;
		set_freq(i,0);
	}
	setup_timer(&state_change_timer,state_change,0);
	mod_timer(&state_change_timer,jiffies+(20*HZ));
	return 0;
}

static void exit_test_scpufreq(void)
{
	del_timer_sync(&state_change_timer);
}


module_param(interval,int, 0444);
MODULE_PARM_DESC(interval, "Set the interval at which to change.");

module_init(init_test_scpufreq);
module_exit(exit_test_scpufreq);

