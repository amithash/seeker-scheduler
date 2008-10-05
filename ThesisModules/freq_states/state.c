#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


#include "state.h"


short cpu_state[NR_CPUS];
short max = 0;

int freq_delta(int delta)
{

