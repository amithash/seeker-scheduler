/******************************************************************************\
 * FILE: log_public.h
 * DESCRIPTION: The public header of the log interface (Public as in, userspace
 * can include this without being depenndent on the kernel headers.
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 * Copyright 2006 Tipp Mosely                                                  *
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

#ifndef _LOG_PUBLIC_H_
#define _LOG_PUBLIC_H_

#include <seeker_cpufreq.h>  /* Contains define for MAX_STATES */

/* Seeker-scheduler sample data */
enum {LOG_SCH, LOG_MUT, LOG_PID, LOG_STATE};

typedef struct {
	unsigned long long interval;
	unsigned int pid;
	unsigned int cpu;
	unsigned int state;
	unsigned long long cycles;
	unsigned int state_req;
	unsigned int state_given;
	unsigned int ipc;
	unsigned long long inst;
} log_scheduler_t;

typedef struct {
	int cpu;
	int state;
	unsigned long residency_time;
} log_state_t;

typedef struct {
	unsigned long long interval;
	unsigned int count;
	unsigned int cpus_req[MAX_STATES];
	unsigned int cpus_given[MAX_STATES];
} log_mutator_t;

typedef struct {

	char name[16];
	unsigned int pid;
} log_pid_t;

typedef struct {
	int type;
	union{
		log_scheduler_t sch;
		log_mutator_t mut;
		log_pid_t tpid;
		log_state_t state;
	}u;
} log_t;

#endif
