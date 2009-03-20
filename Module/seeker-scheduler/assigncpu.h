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

#ifndef __ASSIGNCPU_H_
#define __ASSIGNCPU_H_

void put_mask_from_stats(struct task_struct *ts);
void initial_mask(struct task_struct *ts);
void init_assigncpu_logger(void);
void exit_assigncpu_logger(void);

/* Keep the threshold at 1M Instructions
 * This removes artifcats from IPC and 
 * removes IPC Computation for small tasks
 */
#define INST_THRESHOLD 10000000

/* Works for a max of 32 processors */
#define CPUMASK_TO_UINT(x) (*((unsigned int *)&(x)))

/* Macro to access 'seeker' added members in
 * task_struct (TS). This is used, to avoid
 * the ugly #define SEEKER_PLUGIN_PATCH
 * everywhere, just so that I can test it on
 * an unpatched kernel for compilation errors! 
 */
#ifdef SEEKER_PLUGIN_PATCH
#define TS_MEMBER(ts,member)	ts->member
#else
#define TS_MEMBER(ts,member)	(ts)->flags
#endif

/* You cannot call printk in any of the scheduler related functions. 
 * So this is a little hack to that. Call assigncpu_debug instead
 * NOTE: here is a list of things to know.
 *   1. It tries to get a spin lock. so do not nest it in other critical sections.
 *   2. If you do not have to use it, do not use it.. use debug instead.
 *   3. Keep messages short and sweet not an autobiography. 
 */

#ifdef DEBUG
#define assigncpu_debug(str,a...) do{ 								\
					int __len = strlen(debug_string) + 1;			\
					char __tmp_str[100];		 			\
					__len += sprintf(__tmp_str,"SAD: " str "\n",## a);    	\
					if( __len < ASSIGNCPU_DEBUG_LEN ){			\
						spin_lock(&assigncpu_logger_lock);		\
						strcat(debug_string,__tmp_str);			\
						spin_unlock(&assigncpu_logger_lock);		\
					}							\
				} while(0)
#else
#define assigncpu_debug(str,a...) do{;}while(0)
#endif

#endif
