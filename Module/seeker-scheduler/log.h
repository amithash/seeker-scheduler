/******************************************************************************\
 * FILE: log.h
 * DESCRIPTION: Provides API to use the seeker logging interface.
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

#ifndef __LOG_H_
#define __LOG_H_

#include <log_public.h>

/* Node of the linear linked list of the debug buffer */
struct log_block {
	log_t entry;
	struct log_block *next;
};

/********************************************************************************
 * 				Debug API 					*
 ********************************************************************************/

int log_init(void);
void log_exit(void);

/* Get a block. User has to check for 
 * !NULL before using the return value 
 * If the return value is not NULL, a 
 * spin lock is held */
struct log_block *get_log(void);

/* Release the spin lock held by get debug.
 * If p is NULL, then do nothing */
void put_log(struct log_block *p);

#endif
