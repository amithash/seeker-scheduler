/******************************************************************************\
 * FILE: map_ipc.h
 * DESCRIPTION: API to call to map IPC to functionality.
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

#ifndef _MAP_IPC_H_
#define _MAP_IPC_H_

#define LADDER_SCHEDULING 0
#define SELECT_SCHEDULING 1
#define ADAPTIVE_LADDER_SCHEDULING 2
#define DISABLE_SCHEDULING 3

int evaluate_ipc(int ipc, int cur_state, int *step, int *state_req);

#endif
