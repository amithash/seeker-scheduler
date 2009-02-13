
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <seeker.h>
#include "tsc_intf.h"

unsigned long long tsc_val[NR_CPUS] = {0};

unsigned long long get_tsc_cycles(void)
{
	int cpu = get_cpu();
	unsigned long long ret = 0;
	unsigned long long val = native_read_tsc();
	ret = val - tsc_val[cpu];
	tsc_val[cpu] = val;
	put_cpu();
	return ret;
}

static void init_tsc(void *info)
{
	tsc_val[smp_processor_id()] = native_read_tsc();
}

int init_tsc_intf(void)
{
	if(ON_EACH_CPU(init_tsc,NULL,1,1) < 0) {
		error("Cound not initialize tsc_intf.");
		return -1;
	}
	return 0;
}

