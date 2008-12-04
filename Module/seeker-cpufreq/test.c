#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpufreq cpufreq governor");
MODULE_LICENSE("GPL");

#define IA32_MPERF 0x000000E7
#define IA32_APERF 0x000000E8

short int max_state[NR_CPUS];
short int cur_state[NR_CPUS];
static struct timer_list state_change_timer;
static struct timer_list clk_estimate_timer;
static int total_cpus = NR_CPUS;

static u64 last_jiffies = 0;
static unsigned long long total_clocks[NR_CPUS];
static unsigned int count[NR_CPUS];
static int first = 0;
static unsigned long long last_tsc = 0;
static 

void update_stats(int cpu)
{
	u64 val,tscv;
	static u64 aperf = 0;
	static u64 mperf = 0;
	aperf = native_read_msr(IA32_APERF) - aperf;
	mperf = native_read_msr(IA32_MPERF) - mperf;
	info("APERF=%lld MPERF=%lld",aperf,mperf);
	aperf = native_read_msr(IA32_APERF);
	mperf = native_read_msr(IA32_MPERF);
	if(last_tsc == 0){
		last_tsc = tscv;
		return;
	} else {
		val = tscv - last_tsc;
	}
	last_tsc = tscv;
	total_clocks[cpu] += (val * HZ )/ (jiffies - last_jiffies);
	count[cpu]++;
}
void init_stats(int cpu)
{
	total_clocks[cpu] = 0;
	count[cpu] = 0;
}

void print_stats(int cpu)
{
	unsigned long long avg_clk;
	if(count[cpu] == 0)
		return;
	avg_clk = total_clocks[cpu] / count[cpu];
	info("CurState=%d HZ = %lld",cur_state[cpu],avg_clk);
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

