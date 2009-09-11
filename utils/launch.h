/**************************************************************************
 * Copyright 2009 Amithash Prasad                                         *
 *                                                                        *
 * This file is part of multi-launcher                                    *
 *                                                                        *
 * multi-launcher is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/

#ifndef _LAUNCH_H_
#define _LAUNCH_H_ 1

/* change the keyword to something else if you want to */
#ifndef KEYWORD
#define KEYWORD "core"
#endif

/* this is the macro to call sched_setaffinity() */
#define SET_AFFINITY(x) do { \
cpu_set_t cpuset; \
unsigned long *aff = (unsigned long *)&cpuset; \
memset(&cpuset, 0, sizeof(cpuset)); \
*aff = x; \
assert(sched_setaffinity(0, sizeof(const cpu_set_t), \
                          (const cpu_set_t *)&cpuset) == 0); \
} while (0)

/********************************************************************************
 * 			Global Definations 					*
 ********************************************************************************/

const char CORE[20] = KEYWORD;

struct struct_app {
	int core;
	char prog[300];
	char cmd[300];
	double elapsed;
};

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

double launch(char *cmd);
double in_sec(struct timeval *start, struct timeval *end);
void copy_cmd(char *cmd, char *prog, int max);
int get_options(int *kill_slow, int argc, char *argv[]);
int count_num_apps(int kill_slow_flag, int argc, char *argv[]);
int get_apps(struct struct_app *a, int kill_slow_flag, int number_apps,
	     int argc, char *argv[]);
void print_usage(void);
void help(void);

#endif
