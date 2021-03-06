/******************************************************************************\
 * FILE: state.h
 * DESCRIPTION: Provides API to manipulate states and also to maintain the
 * performance state demand.
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

#ifndef __STATE_H_
#define __STATE_H_

/* contains various parameters associated
 * to a CPU state. 
 */
struct state_desc {
	short state;
	cpumask_t cpumask;
	short cpus;
	atomic_t demand;
};

/********************************************************************************
 * 				States API 					*
 ********************************************************************************/

void demand_inc(int state);
void demand_dec(int state);
int demand_get(int state);
void demand_clear(int state);

int init_cpu_states(void);
void exit_cpu_states(void);
void states_copy(struct state_desc *dest, struct state_desc *src);
void start_state_logger(void);
void stop_state_logger(void);

#endif
