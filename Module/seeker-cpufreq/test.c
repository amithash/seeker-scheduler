#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>
#include <fpmu.h>
#include <pmu.h>
#include <seeker_cpufreq.h>

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpufreq cpufreq governor");
MODULE_LICENSE("GPL");

#define PMU_RECY_EVTSEL 0x76
#define PMU_RECY_MASK 0x00

short int max_state[NR_CPUS];
short int cur_state[NR_CPUS];
static struct timer_list state_change_timer;
static struct timer_list clk_estimate_timer;
static int total_cpus = NR_CPUS;

static unsigned long long total_clocks[NR_CPUS];
static unsigned long long last_jiffies = 0;
static unsigned int count[NR_CPUS];
static int ctr[NR_CPUS];
static int first = 0;

void update_stats(int cpu)
{
	#if NUM_FIXED_COUNTERS > 0
	fcounter_read();
	total_clocks[cpu] += (get_fcounter_data(1,cpu) * HZ )/ (jiffies - last_jiffies);
	fcounter_clear(1);
	#else
	counter_read();
	total_clocks[cpu] += (get_counter_data(ctr[cpu],cpu) * HZ) / (jiffies - last_jiffies);
	counter_clear(ctr[cpu]);
	#endif
	count[cpu]++;
}
void init_stats(int cpu)
{
	total_clocks[cpu] = 0;
	count[cpu] = 0;
}

void print_stats(int cpu)
{
	unsigned long long avg_clk = total_clocks[cpu] / count[cpu];
	info("CurState=%d HZ = %lld\n",cur_state[cpu],avg_clk);
}


void clk_estimate(unsigned long param)
{
	int cpu = get_cpu();
	update_stats(cpu);
	last_jiffies = jiffies;
	mod_timer(&clk_estimate_timer,jiffies+(HZ));
	put_cpu();
}

void state_change(unsigned long param)
{
	int i;
	int cpu = get_cpu();
	if(first == 0){
		first = 1;
	} else {
		print_stats(cpu);
	}
	print_stats(cpu);
	init_stats(cpu);
	for(i=0;i<total_cpus;i++){
		set_freq(i,cur_state[i]);
		cur_state[i] = (cur_state[i]+1)%max_state[i];
	}
	last_jiffies = jiffies;
	mod_timer(&clk_estimate_timer,jiffies+(HZ));
	mod_timer(&state_change_timer,jiffies+(10*HZ));
	first = 1;
	put_cpu();
}

void configure_counters(void)
{
#if NUM_FIXED_COUNTERS > 0
	fcounters_enable(0);
	fcounter_clear(1);
#else
	int cpu = smp_processor_id();
	ctr[cpu] = counter_enable(PMU_RECY_EVTSEL,PMU_RECY_MASK,0);
	counter_clear(ctr[cpu]);
#endif
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
	ON_EACH_CPU((void *)configure_counters,NULL,1,1);

	setup_timer(&clk_estimate_timer,clk_estimate,0);
	setup_timer(&state_change_timer,state_change,0);
	mod_timer(&state_change_timer,jiffies+(10*HZ));
	return 0;
}

static void exit_test_scpufreq(void)
{
	del_timer_sync(&clk_estimate_timer);
	del_timer_sync(&state_change_timer);
}
module_init(init_test_scpufreq);
module_exit(exit_test_scpufreq);

