#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpufreq cpufreq governor");
MODULE_LICENSE("GPL");

short int max_state[NR_CPUS];
short int cur_state[NR_CPUS];
static struct timer_list state_change_timer;
static int total_cpus = NR_CPUS;

static int last_cpu = 0;

void print_stats(int cpu)
{
	info("CPU: %d CurState=%d",cpu,cur_state[cpu]);
}


void state_change(unsigned long param)
{
	int i;
	int cpu = get_cpu();
	info("cpu for state change = %d",cpu);
	if(first == 0){
		first = 1;
	} else {
		print_stats(cpu);
	}
	init_stats(cpu);
	for(i=0;i<total_cpus;i++){
		cur_state[i] = (cur_state[i]+1)%max_state[i];
		set_freq(i,cur_state[i]);
	}
	del_timer_sync(&clk_estimate_timer);
	mod_timer(&clk_estimate_timer,jiffies+(HZ));
	mod_timer(&state_change_timer,jiffies+(20*HZ));
	put_cpu();
}

static int init_test_scpufreq(void)
{
	int i;
	total_cpus = num_online_cpus();
	
	for(i=0;i<total_cpus;i++){
		max_state[i] = get_max_states(i);
		count[i] = 0;
		total_clocks[i] = 0;
	}
	setup_timer(&clk_estimate_timer,clk_estimate,0);
	setup_timer(&state_change_timer,state_change,0);
	mod_timer(&state_change_timer,jiffies+(20*HZ));
	return 0;
}

static void exit_test_scpufreq(void)
{
	del_timer_sync(&clk_estimate_timer);
	del_timer_sync(&state_change_timer);
}
module_init(init_test_scpufreq);
module_exit(exit_test_scpufreq);

