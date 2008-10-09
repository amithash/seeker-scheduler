
u32 interval_jiffies;
struct timer_list state_change_timer;


void destroy_timer(void)
{
	del_timer_sync(&state_change_timer);
}

int create_timer(void)
{
	interval_jiffies = change_interval * HZ;
	debug("Interval set to every %d jiffies",interval_jiffies);
	init_timer(&state_change_timer);
	state_change_timer.function = &state_change;
	state_change(0);
	return 0;
}
void state_change(unsigned long param)
{
	debug("State change now.");
	mod_timer(&state_change_timer, jiffies + interval_jiffies);
}

