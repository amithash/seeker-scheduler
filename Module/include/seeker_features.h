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
#ifndef _SEEKER_FEATURES_H_
#define _SEEKER_FEATURES_H_


#define APPROXIMATE_DIRECTION_BASED_MUTATOR 100
#define DYNAMIC_PROGRAMMING_BASED_MUTATOR 200

#define LADDER_SCHEDULING 100
#define ADAPTIVE_LADDER_SCHEDULING 200
#define SELECT_SCHEDULING 300

/********************************************************************************
 * 			  SCHEDULER DEBUGGING 					*
 ********************************************************************************/

#ifdef DEBUG
	/* Uncomment the next line to enable scheduler logging */
	// #define SCHED_DEBUG 1
#endif

/********************************************************************************
 * 			  SELECT MUTATOR 					*
 ********************************************************************************/

/* Uncomment one to choose a mutator */
// #define MUTATOR_TYPE DYNAMIC_PROGRAMMING_BASED_MUTATOR
#define MUTATOR_TYPE APPROXIMATE_DIRECTION_BASED_MUTATOR

/********************************************************************************
 * 			  SELECT SCHEDULER 					*
 ********************************************************************************/

/* Uncomment one to choose scheduler */
#define SCHEDULER_TYPE LADDER_SCHEDULING
// #define SCHEDULER_TYPE ADAPTIVE_LADDER_SCHEDULING
// #define SCHEDULER_TYPE SELECT_SCHEDULING


#ifndef MUTATOR_TYPE
#error "Select at least one mutator in include/features.h"
#endif

#ifndef SCHEDULER_TYPE
#error "Select at least one scheduler in include/features.h"
#endif

#endif
