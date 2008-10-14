#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "../scpufreq.h"
#include "hint.h"


int hint[MAX_STATES] = {0};

int get_hint(int *ht)
{
	int i;
	int total = get_total_states();
	for(i=0;i<=total;i++){
		ht[i] = hint[i];
	}
	return total;
}


void clear_hint(void)
{
	int i;
	for(i=0;i<MAX_STATES;i++)
		hint[i] = 0;
}
EXPORT_SYMBOL_GPL(clear_hint);

int hint_count(void)
{
	int i,count=0;
	int total = get_total_states();
	for(i=0;i<total;i++)
		if(hint[i] > 0)
			count++;
	return count;
}
EXPORT_SYMBOL_GPL(hint_count);

void hint_inc(int state)
{
	atomic_inc((void *)&(hint[state]));
}
EXPORT_SYMBOL_GPL(hint_inc);

void hint_dec(int state)
{
	atomic_dec((void *)&(hint[state]));
}
EXPORT_SYMBOL_GPL(hint_dec);

