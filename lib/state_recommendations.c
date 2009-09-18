/******************************************************************************\
 * FILE: state_recommendations.c
 * DESCRIPTION: This is the syscall wrapper library. Gives the numbers a face.
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

#include <stdio.h>
#include <unistd.h>

#include "state_recommendations.h"

#ifdef __LP64__
#define __NR_seeker 295
#else
#define __NR_seeker 333
#endif

int low_freq(void)
{
	return syscall(__NR_seeker, 0, 0);
}

int mid_freq(void)
{
	return syscall(__NR_seeker, 0, 1);
}

int high_freq(void)
{
	return syscall(__NR_seeker, 0, 2);
}

int continue_dynamically(void)
{
	return syscall(__NR_seeker, 1, 0);
}

int ignore_task(void)
{
	return syscall(__NR_seeker, 2, 0);
}
