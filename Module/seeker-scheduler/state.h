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

#ifndef __STATE_H_
#define __STATE_H_

/* init defines. */
#define ALL_HIGH 1
#define BALANCE 2
#define ALL_LOW 3
#define STATIC_LAYOUT 4
#define NO_CHANGE 5

/* contains various parameters associated
 * to a CPU state. 
 */
struct state_desc {
	short state;
	cpumask_t cpumask;
	short cpus;
	unsigned int demand;
};

/* if val is 0, states are not sane,
 * val is 1 indicates states are sane.
 * val can only be changed by taking
 * the lock. 
 */
struct state_sane_t {
	short val;
	spinlock_t lock;
};

/********************************************************************************
 * 				States API 					*
 ********************************************************************************/

void hint_inc(int state);
void hint_dec(int state);
int init_cpu_states(unsigned int how);
void mark_states_inconsistent(void);
void mark_states_consistent(void);
int is_states_consistent(void);

#endif
