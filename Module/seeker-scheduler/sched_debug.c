/******************************************************************************\
 * FILE: sched_debug.c
 * DESCRIPTION: As printk's are banned in the scheduler (In fact adding them
 * will cause a crash) this interface provdes a scheme to print debug messages
 * which allow prints which are just copied to a buffer and printed by a timer.
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

void sched_debug_logger(struct work_struct *w);

/********************************************************************************
 * 				global_variables				*
 ********************************************************************************/
#ifdef SCHED_DEBUG
/* temp storage for sched_debug messages */
char debug_string[SCHED_DEBUG_LEN] = "";

/* Spin lock to change debug_string safely */
DEFINE_SPINLOCK(sched_debug_logger_lock);

/* Flag indicating that logger has started */
int sched_debug_logger_started = 0;

/* Work which safely prints the work */
static DECLARE_DELAYED_WORK(sched_debug_logger_work, sched_debug_logger);
#endif

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/
#ifdef SCHED_DEBUG

#warning "SCHED_DEBUG is enabled, performance will be compromised"
/********************************************************************************
 * sched_debug_logger - The sched_debug logging function.
 * @w - The work calling this routine, not used.
 * 
 * Prints out the contents of the sched_debug debug buffer and sets 
 * its length to 0. This is done safely with a spin lock held. 
 ********************************************************************************/
void sched_debug_logger(struct work_struct *w)
{
	spin_lock(&sched_debug_logger_lock);
	printk("%s", debug_string);
	debug_string[0] = '\0';
	spin_unlock(&sched_debug_logger_lock);
	if (sched_debug_logger_started)
		schedule_delayed_work(&sched_debug_logger_work,
				      SCHED_DEBUG_LOGGER_INTERVAL);
}

/********************************************************************************
 * init_sched_debug_logger - initialize the sched_debug logging subtask.
 *
 * Initialize the locks and worker thread used for the sched_debug
 * logging subtask.
 ********************************************************************************/
void init_sched_debug_logger(void)
{
	sched_debug_logger_started = 1;
	spin_lock_init(&sched_debug_logger_lock);
	init_timer_deferrable(&sched_debug_logger_work.timer);
	schedule_delayed_work(&sched_debug_logger_work,
			      SCHED_DEBUG_LOGGER_INTERVAL);
	return;
}

/********************************************************************************
 * exit_sched_debug_logger - Cleanup the sched_debug logging subtask.
 *
 * Cancels all pending sched_debug logging tasks which are pending. 
 * Then unlocks the spin lock if it is still held.
 ********************************************************************************/
void exit_sched_debug_logger(void)
{
	if (sched_debug_logger_started) {
		sched_debug_logger_started = 0;
		cancel_delayed_work(&sched_debug_logger_work);
	}
	if (spin_is_locked(&sched_debug_logger_lock))
		spin_unlock(&sched_debug_logger_lock);
}

#else

/* sched_debug logging interface are just stubs when debug is not enabled */

void sched_debug_logger(struct work_struct *w)
{
	return;
}

void init_sched_debug_logger(void)
{
	return;
}

void exit_sched_debug_logger(void)
{
	return;
}
#endif
