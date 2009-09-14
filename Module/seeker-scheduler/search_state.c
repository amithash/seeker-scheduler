/******************************************************************************\
 * FILE: search_state.c
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

#include <linux/cpu.h>

#include <seeker.h>

#include "state.h"
#include "nrtasks.h"
#include "search_state.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* state.c: description of each state */
extern struct state_desc states[MAX_STATES];

extern int total_states;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * state_free - Is state free?
 * @state - State to check.
 * @Return - 1 if state is free, 0 otherwise.
 *
 * Returns 1 if state is free to execute on. 
 ********************************************************************************/
inline int is_state_free(int old, int state)
{
	if(old == state && states[state].cpus > 0 && 
		get_state_tasks_exself(state) < states[state].cpus)
		return 1;
	if(states[state].cpus > 0 && get_state_tasks(state) < states[state].cpus)
		return 1;

	return 0;
}

/********************************************************************************
 * lowest_loaded_state - Returns the state which is least loaded.
 * @Return - The state which is loaded the least.
 *
 * Search for a state which is the least loaded. 
 ********************************************************************************/
int lowest_loaded_state(void)
{
	int min_load = 100000;
	int min_state = 0;
	int min_found = 0;
	int this_load;
	int i;

	for(i=0;i<total_states;i++){
		if(states[i].cpus == 0)
			continue;

		this_load = get_state_tasks(i) - states[i].cpus;
		if(min_found == 0){
			min_load = this_load;
			min_state = i;
			min_found = 1;
			continue;
		}
		if(this_load < min_load){
			min_load = this_load;
			min_state = i;
		} 
	}
	if(min_found == 0)
		return -1;

	return min_state;
}
		

/********************************************************************************
 * get_lower_state - Get a state lower than current state.
 * @state - The current state
 * @Return - An avaliable state preferrably lower than state
 * @Side Effects - None
 *
 * Returns the closest avaliable state lower than 'state'. If none are found, 
 * return 'state' or a higher state closest to 'state' whatever is avaliable
 ********************************************************************************/
int search_state_down(int old_state, int req_state)
{
	int new_state = req_state;
	int i;
	for (i = new_state; i >= 0; i--) {
		if (is_state_free(old_state,i))
			return i;
	}
	for (i = new_state + 1; i < total_states; i++) {
		if (is_state_free(old_state,i))
			return i;
	}
	return lowest_loaded_state();
}

/********************************************************************************
 * get_higher_state - Get a state higher than current state.
 * @state - The current state
 * @Return - An avaliable state preferrably higher than state
 * @Side Effects - None
 *
 * Returns the closest avaliable state higher than 'state'. If none are found, 
 * return 'state' or a lower state closest to 'state' whatever is avaliable.
 ********************************************************************************/
inline int search_state_up(int old_state, int req_state)
{
	int new_state = req_state;
	int i;
	for (i = new_state; i < total_states; i++) {
		if (is_state_free(old_state,i))
			return i;
	}
	for (i = new_state - 1; i >= 0; i--) {
		if (is_state_free(old_state,i))
			return i;
	}
	return lowest_loaded_state();
}

/********************************************************************************
 * get_closest_state - Get a state closest to the current state.
 * @state - The current state
 * @Return - An avaliable state closest to 'state'
 * @Side Effects - None
 *
 * Returns 'state' if avaliable, or a state closest to 'state'
 ********************************************************************************/
inline int search_state_closest(int old_state, int req_state)
{
	int state1, state2;
	int ret_state;
	if (is_state_free(old_state,req_state))
		return req_state;
	state1 = search_state_down(old_state,req_state);
	state2 = search_state_up(old_state,req_state);
	if(state1 == -1 && state2 == -1)
		return lowest_loaded_state();
	if(state1 == -1)
		return state2;
	if(state2 == -1)
		return state1;
	ret_state = ABS(req_state - state1) < ABS(req_state - state2) ? state1 : state2;

	return ret_state;
}

