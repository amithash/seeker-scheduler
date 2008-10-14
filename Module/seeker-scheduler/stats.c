
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/types.h>

#include "../../Module/seeker.h"
#include "stats.h"

#ifndef SEEKER_PLUGIN_PATCH
#define NOPATCH
#endif

u64 interval_count = 0;

/* Seeker is responsible of calling this */
void update_stats(struct task_struct *t, 
		 u64 inst,
		 u64 re_cy,
		 u64 ref_cy)
{
#ifndef NOPATCH
	if(t->interval == interval_count){
		t->inst   += inst;
		t->re_cy  += re_cy;
		t->ref_cy += ref_cy;
	}
	else{
		t->interval = interval_count;
		t->inst     = inst;
		t->re_cy    = re_cy;
		t->ref_cy   = ref_cy;
	}
#endif
}
EXPORT_SYMBOL_GPL(update_stats);

void init_stats(struct task_struct *t)
{
#ifndef NOPATCH
	t->interval = interval_count;
	t->inst    = 0;
	t->re_cy   = 0;
	t->ref_cy  = 0;
#endif
}

void increment_interval(void)
{
	interval_count++;
}
EXPORT_SYMBOL_GPL(increment_interval);

