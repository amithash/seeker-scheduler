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

#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <seeker.h>

/* Node of the linear linked list of the debug buffer */
struct debug_block {
	debug_t entry;
	struct debug_block *next;
};

/********************************************************************************
 * 				Debug API 					*
 ********************************************************************************/

int debug_init(void);
void debug_exit(void);

/* Get a block. User has to check for 
 * !NULL before using the return value 
 * If the return value is not NULL, a 
 * spin lock is held */
struct debug_block *get_debug(void);

/* Release the spin lock held by get debug.
 * If p is NULL, then do nothing */
void put_debug(struct debug_block *p);

void debug_free(struct debug_block *p);

#endif

