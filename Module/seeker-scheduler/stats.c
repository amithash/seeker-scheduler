#include <linux/kernel_stat.h>
#include <linux/percpu.h>
#include <linux/jiffies.h>
#include <linux/time.h>

struct idle_info_t{
	unsigned int prev_idle_time;
};

static DEFINE_PER_CPU(struct idle_info_t,idle_info);

extern int change_interval;

#define CPU_INFO(cpu) (&per_cpu(idle_info,(cpu)))

void init_idle_logger(void)
{
	int i;
	int cpus = num_online_cpus();
	for(i=0;i<cpus;i++){
		CPU_INFO(i)->prev_idle_time = kstat_cpu(i).cpustat.idle + kstat_cpu(i).cpustat.iowait;
	}
}

unsigned int get_cpu_load(int cpu)
{
	unsigned total_time;
	unsigned int cur_idle_time = kstat_cpu(cpu).cpustat.idle + kstat_cpu(cpu).cpustat.iowait;
	unsigned int this_time = cur_idle_time - CPU_INFO(cpu)->prev_idle_time;
	CPU_INFO(cpu)->prev_idle_time = cur_idle_time;
	total_time = msecs_to_jiffies(1000 * change_interval);

	if(this_time >= total_time)
		return 0;

	return (8 * (total_time - this_time)) / total_time;
}


