#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>

#include "quanta.h"
#include "state.h"
#include "mutate.h"

u64 interval_jiffies;
void state_change(unsigned long param);

extern int change_interval;
static struct timer_list state_change_timer;

extern int delta;


void destroy_timer(void)
{
	del_timer_sync(&state_change_timer);
}

void state_change(unsigned long param)
{
//	choose_layout(delta);
	mod_timer(&state_change_timer, jiffies + interval_jiffies*HZ);
}
EXPORT_SYMBOL_GPL(state_change);


int create_timer(void)
{
	interval_jiffies = change_interval * HZ;
	debug("Interval set to every %d jiffies",interval_jiffies);
	setup_timer(&state_change_timer,state_change,0);
	warn("State change addr  = %lx",(unsigned long)state_change_timer.function);
	mod_timer(&state_change_timer,jiffies + interval_jiffies);
	return 0;
}


