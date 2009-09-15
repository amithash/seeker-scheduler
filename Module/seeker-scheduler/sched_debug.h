/******************************************************************************\
 * FILE: sched_debug.h
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

#ifndef __SCHED_DEBUG_H_
#define __SCHED_DEBUG_H_

void init_sched_debug_logger(void);
void exit_sched_debug_logger(void);

/* Interval to print messages */
#define SCHED_DEBUG_LOGGER_INTERVAL (HZ/10)

/* Length of buffer. Increase this if you increase the above */
#define SCHED_DEBUG_LEN 1024

#ifdef SCHED_DEBUG
extern char debug_string[SCHED_DEBUG_LEN];
extern spinlock_t sched_debug_logger_lock;
#endif

/* You cannot call printk in any of the scheduler related functions. 
 * So this is a little hack to that. Call sched_debug instead
 * NOTE: here is a list of things to know.
 *   1. It tries to get a spin lock. so do not nest it in other critical sections.
 *   2. If you do not have to use it, do not use it.. use debug instead.
 *   3. Keep messages short and sweet not an autobiography. 
 */

#if defined(SCHED_DEBUG) 
#define sched_debug(str,a...) do{ 								\
					int __len = strlen(debug_string) + 1;			\
					char __tmp_str[100];		 			\
					__len += sprintf(__tmp_str,"SAD: " str "\n",## a);    	\
					if( __len < SCHED_DEBUG_LEN ){				\
						spin_lock(&sched_debug_logger_lock);		\
						strcat(debug_string,__tmp_str);			\
						spin_unlock(&sched_debug_logger_lock);		\
					}							\
				} while(0)
#else
#define sched_debug(str,a...) 
#endif

#endif
