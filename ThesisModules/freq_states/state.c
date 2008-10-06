#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


#include "state.h"
#include "../freq_schedule/hint.h"
#include "../freq_schedule/estimate.h"

int freq_delta(int delta)
{
	int *cpu_state;

	get_state_of_cpu(cpu_state);
	/* Update state of cpu */
	put_state_of_cpu();
}



