/******************************************************************************\
 * FILE: load.h
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

#ifndef _STATS_H_
#define _STATS_H_

/********************************************************************************
 * 				Stats API 					*
 ********************************************************************************/

/* add two load types */
#define ADD_LOAD(a,b) ((a) + (b))

/* multiply two load types */
#define MUL_LOAD(a,b) (((a) * (b)) >> 3)

/* Multiply a load type with an uint */
#define MUL_LOAD_UINT(a,b) ((a) * (b))

/* Divide two load types */
#define DIV_LOAD(a,b) (((a)<<3)/(b))

/* Divide a load type by an uint */
#define DIV_LOAD_UINT(a,b) ((((a)<<3)/(b))>>3)

/* Convert a load type to an uint */
#define LOAD_TO_UINT(a) ((a)>>3)

/* Convert an uint to a load type */
#define UINT_TO_LOAD(a) ((a)<<3)

#define LOAD_0_000 0
#define LOAD_0_125 1
#define LOAD_0_250 2
#define LOAD_0_375 3
#define LOAD_0_500 4
#define LOAD_0_625 5
#define LOAD_0_750 6
#define LOAD_0_875 7
#define LOAD_1_000 8
#define LOAD_1_125 (LOAD_1_000 + LOAD_0_125)


void init_idle_logger(void);
unsigned int get_cpu_load(int cpu);

#endif
