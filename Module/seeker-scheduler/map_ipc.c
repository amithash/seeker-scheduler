/******************************************************************************\
 * FILE: map_ipc.c
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

#include <linux/kernel.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "ipc.h"
#include "map_ipc.h"
#include "search_state.h"

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

extern int total_states;

extern int scheduling_method;


/********************************************************************************
 * 				Local Macros					*
 ********************************************************************************/

/* LADDER: The HIGH IPC Threshold */
#define IPC_HIGH IPC_0_750

/* LADDER: The LOW IPC Threshold */
#define IPC_LOW  IPC_0_500

/* SELECT: IPC Thresholds */
#define MIN_IPC_4 (IPC_1_000 + IPC_0_125)
#define MIN_IPC_3 (IPC_0_750)
#define MIN_IPC_2 (IPC_0_500)
#define MIN_IPC_1 (IPC_0_250)
#define MIN_IPC_0 0

/* macro to perform saturating increment with an exclusive limit */
#define sat_inc(state,ex_limit) ((state) < ex_limit-1 ? (state)+1 : ex_limit-1)

/* macro to performa a saturating decrement with an inclusive limit */
#define sat_dec(state,inc_limit) ((state) > inc_limit ? (state)-1 : inc_limit)

/* macro to do a sat sum on integers (including negative nums)
 * with exclusive high and inclusive low
 */
#define sat_sum(a,b,inc_low, ex_high) (((a) + (b)) >= (ex_high) ? \
                                      (ex_high)-1 :               \
                                                                  \
                                      (((a)+(b)) < (inc_low) ?    \
                                       (inc_low) :                \
                                       ((a)+(b))))


static int min_states[MAX_STATES] = {MIN_IPC_0, MIN_IPC_1, 
				     MIN_IPC_2, MIN_IPC_3,
				     MIN_IPC_4};


int ladder_evaluation(int ipc, int cur_state, int *state_req)
{
  int new_state;
  
			/*up */
	if (ipc >= IPC_HIGH) {
		*state_req = sat_inc(cur_state, total_states);
		new_state = search_state_up(cur_state,*state_req);
	} else if (ipc <= IPC_LOW) {
		*state_req = sat_dec(cur_state, 0);
		new_state = search_state_down(cur_state, *state_req);
	} else {
		*state_req = cur_state;
		new_state = search_state_closest(cur_state, *state_req);
	}

  return new_state;
}

int adaptive_ladder_evaluation(int ipc, int cur_state, int *step, int *state_req)
{
  int new_state;
	if (ipc >= IPC_HIGH) {
    if(*step > 0) {
			*state_req = sat_sum(cur_state, *step ,0, total_states);
			*step = sat_inc(*step,total_states);
      new_state = search_state_up(cur_state, *state_req);
		} else if(*step == 0){
      *state_req = cur_state;
      *step = 1;
      new_state = search_state_closest(cur_state, *state_req);
    } else {
			*state_req = cur_state;
			*step = 0;
      new_state = search_state_closest(cur_state, *state_req);
		}
  } else if (ipc <= IPC_LOW) {
		if(*step < 0){
			*state_req = sat_sum(cur_state, *step, 0, total_states);
			*step = sat_dec(*step, (-1*(int)total_states));
      new_state = search_state_down(cur_state, *state_req);
		} else if(*step == 0){
      *state_req = cur_state;
      *step = -1;
      new_state = search_state_closest(cur_state, *state_req);
    } else {
			*state_req = cur_state;
			*step = 0;
      new_state = search_state_closest(cur_state, *state_req);
		}
	} else {
		*state_req = cur_state;
		new_state = search_state_closest(cur_state,*state_req);
		*step = 0;
	}

  return new_state;
}

int select_evaluation(int ipc, int cur_state, int *state_req)
{
  int i;
  int new_state;

	for(i = total_states - 1; i >= 0; i++){
		if(ipc >= min_states[i]){
			*state_req = i;
			break;
		}
	}
	new_state = search_state_closest(cur_state, *state_req);

  return new_state;
}
int error_thrown = 0;

int evaluate_ipc(int ipc, int cur_state, int *step, int *state_req)
{
  int new_state = cur_state;
  switch(scheduling_method){
  	case LADDER_SCHEDULING: 
  		new_state = ladder_evaluation(ipc,cur_state,state_req);
		break;
	case ADAPTIVE_LADDER_SCHEDULING:
  		new_state = adaptive_ladder_evaluation(ipc,cur_state,step,state_req);
		break;
	case SELECT_SCHEDULING:
  		new_state = select_evaluation(ipc, cur_state, state_req);
		break;
	default:
		if(!error_thrown){
			error("INVALID SCHEDULING method: %d",scheduling_method);
			error_thrown = 1;
		}
  }
  return new_state;
}

