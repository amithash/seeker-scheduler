/******************************************************************************\
 * FILE: test_recommendations.c
 * DESCRIPTION: This is an _example_ usage of the usage of the 
 * state_recommendation library used to instruct seeker scheduler about the
 * nature of the clock speed required.
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
#include "state_recommendations.h"

int main(void)
{
	int i, j, k;
	unsigned long long sum = 0;
	/* Hi cpu bound kernel */
	high_freq();
	for (i = 0; i < 10000000; i++) {
		sum += i;
	}
	continue_dynamically();
	return 0;
}
