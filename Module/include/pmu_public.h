/******************************************************************************\
 * FILE: hwcounters.h
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

#ifndef __PMU_PUBLIC_H_
#define __PMU_PUBLIC_H_

#if defined(ARCH_C2D)
#	define NUM_COUNTERS 2
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define NUM_COUNTERS 4
#else
#	define NUM_COUNTERS 0
#error "Architecture not supported."
#endif

#if defined(ARCH_C2D)
#	define NUM_FIXED_COUNTERS 3
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define NUM_FIXED_COUNTERS 0
#else
#error "Architecture not supported."
#endif

#endif
