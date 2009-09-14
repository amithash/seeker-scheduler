/******************************************************************************\
 * FILE: sched_debug.c
 * DESCRIPTION: 
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>

#include <seeker.h>

#include "sched_debug.h"


/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

void assigncpu_logger(struct work_struct *w);

/********************************************************************************
 * 				global_variables				*
 ********************************************************************************/
#ifdef SCHED_DEBUG
/* temp storage for assigncpu messages */
char debug_string[ASSIGNCPU_DEBUG_LEN] = "";

/* Spin lock to change debug_string safely */
DEFINE_SPINLOCK(assigncpu_logger_lock);

/* Flag indicating that logger has started */
int assigncpu_logger_started = 0;

/* Work which safely prints the work */
static DECLARE_DELAYED_WORK(assigncpu_logger_work, assigncpu_logger);
#endif

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/
#ifdef SCHED_DEBUG
/********************************************************************************
 * assigncpu_logger - The assigncpu logging function.
 * @w - The work calling this routine, not used.
 * 
 * Prints out the contents of the assigncpu debug buffer and sets 
 * its length to 0. This is done safely with a spin lock held. 
 ********************************************************************************/
void assigncpu_logger(struct work_struct *w)
{
	spin_lock(&assigncpu_logger_lock);
	printk("%s",debug_string);
	debug_string[0] = '\0';
	spin_unlock(&assigncpu_logger_lock);
	if(assigncpu_logger_started)
		schedule_delayed_work(&assigncpu_logger_work, ASSIGNCPU_LOGGER_INTERVAL);	
}

/********************************************************************************
 * init_assigncpu_logger - initialize the assigncpu logging subtask.
 *
 * Initialize the locks and worker thread used for the assigncpu
 * logging subtask.
 ********************************************************************************/
void init_assigncpu_logger(void)
{
	assigncpu_logger_started = 1;
	spin_lock_init(&assigncpu_logger_lock);
	init_timer_deferrable(&assigncpu_logger_work.timer);
	schedule_delayed_work(&assigncpu_logger_work, ASSIGNCPU_LOGGER_INTERVAL);	
	return;
}

/********************************************************************************
 * exit_assigncpu_logger - Cleanup the assigncpu logging subtask.
 *
 * Cancels all pending assigncpu logging tasks which are pending. 
 * Then unlocks the spin lock if it is still held.
 ********************************************************************************/
void exit_assigncpu_logger(void)
{
	if(assigncpu_logger_started){
		assigncpu_logger_started = 0;
		cancel_delayed_work(&assigncpu_logger_work);
	}
	if(spin_is_locked(&assigncpu_logger_lock))
		spin_unlock(&assigncpu_logger_lock);
}

#else

/* assigncpu logging interface are just stubs when debug is not enabled */

void assigncpu_logger(struct work_struct *w)
{
	return;
}
void init_assigncpu_logger(void)
{
	return;
}
void exit_assigncpu_logger(void)
{
	return;
}
#endif

