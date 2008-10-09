#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "../../Module/scpufreq.h"
#include "hint.h"


int hint[MAX_STATES] = {0};

int get_hint(int *ht, int count)
{
	int i;
	for(i=0;i<count&&i<MAX_STATES;i++){
		ht[i] = hint[i];
	}
	return i;
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
	for(i=0;i<MAX_STATES;i++)
		if(hint[i] > 0)
			count++;
	return count;
}
EXPORT_SYMBOL_GPL(hint_count);

void hint_inc(int state)
{
	hint[state]++;
}
EXPORT_SYMBOL_GPL(hint_inc);

void hint_dec(int state)
{
	hint[state]--;
}
EXPORT_SYMBOL_GPL(hint_dec);



