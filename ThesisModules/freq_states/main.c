
/*****************************************************
 * Copyright 2008 Amithash Prasad                    *
 *                                                   *
 * This file is part of Seeker                       *
 *                                                   *
 * Seeker is free software: you can redistribute     *
 * it and/or modify it under the terms of the        *
 * GNU General Public License as published by        *
 * the Free Software Foundation, either version      *
 * 3 of the License, or (at your option) any         *
 * later version.                                    *
 *                                                   *
 * This program is distributed in the hope that      *
 * it will be useful, but WITHOUT ANY WARRANTY;      *
 * without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE. See the GNU General Public License       *
 * for more details.                                 *
 *                                                   *
 * You should have received a copy of the GNU        *
 * General Public License along with this program.   *
 * If not, see <http://www.gnu.org/licenses/>.       *
 *****************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <asm/hw_irq.h>
#include <linux/timer.h>


#include "../../Module/seeker.h"
#include "../../Module/scpufreq.h"

#include "state.h"

MODULE_AUTHOR("Amithash Prasad <amithash.prasad@colorado.edu>");
MODULE_DESCRIPTION("module to peridocally change the cpu configuration based on external parameters");
MODULE_LICENSE("GPL");

void state_change(unsigned long param);
int create_timer(void);
void destroy_timer(void);


int change_interval = 5; /* In seconds */
int change_type = 0;
int init = ALL_HIGH;

u32 interval_jiffies;
struct timer_list state_change_timer;

void state_change(unsigned long param)
{
	debug("State change now.");
	mod_timer(&state_change_timer, jiffies + interval_jiffies);
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

void destroy_timer(void)
{
	del_timer_sync(&state_change_timer);
}

static int __init state_init(void)
{
	init_cpu_states(init);
	create_timer();
	return 0;
}

static void __exit state_exit(void)
{
	destroy_timer();
}

module_init(state_init);
module_exit(state_exit);

module_param(change_interval,int,0444);
MODULE_PARM_DESC(change_interval, "Interval in seconds to try and change the global state (Default 5 seconds)");

module_param(init,int,0444);
MODULE_PARM_DESC(init,"Starting state of cpus: 1 - All high, 2 - half high, half low, 3 - All low");

module_param(change_type,int,0444);
MODULE_PARM_DESC(change_type,"Type of state machine to use 0,1,2,.. default:0");

