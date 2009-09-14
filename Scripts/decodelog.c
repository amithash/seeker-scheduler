/**************************************************************************
 * Copyright 2009 Amithash Prasad                                         *
 * Copyright 2006 Tipp Mosely                                             *
 *                                                                        *
 * This file is part of Seeker                                            *
 *                                                                        *
 * Seeker is free software: you can redistribute it and/or modify         *
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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <log_public.h>
#include "../generic.h"

int main(int argc, char **argv, char **envp)
{
	int i;
	int num_counters = -1;
	unsigned long long total_cycles[NR_CPUS] = { 0 };
	int first_sample[NR_CPUS] = { 1 };
	int debug = 0;
	const int debug_bufsize = sizeof(log_t);
	char debug_buf[debug_bufsize];

	while (fread(debug_buf, 1, debug_bufsize, stdin) == debug_bufsize) {
		log_t *entry = (log_t *) (debug_buf);
		switch (entry->type) {
			log_scheduler_t *schDef;
			log_mutator_t *mutDef;
			log_pid_t *pidDef;
			log_state_t *stateDef;
		case LOG_SCH:
			schDef = (log_scheduler_t *) (&entry->u);
			printf("s");
			printf(",%llu,%d,%d,%llu,%1.4f,%d,%d,%d,%llu\n",
			       schDef->interval, schDef->pid, schDef->cpu,
			       schDef->inst, ((float)schDef->ipc) / 8.0,
			       schDef->state_req, schDef->state_given,
			       schDef->state, schDef->cycles);
			break;
		case LOG_PID:
			pidDef = (log_pid_t *) (&entry->u);
			printf("p");
			pidDef->name[15] = '\0';
			printf(",%d,%s\n", pidDef->pid, pidDef->name);
			break;
		case LOG_MUT:
			mutDef = (log_mutator_t *) (&entry->u);
			printf("m,%llu,r", mutDef->interval);
			for (i = 0; i < mutDef->count; i++) {
				printf(",%d", mutDef->cpus_req[i]);
			}
			printf(",g");
			for (i = 0; i < mutDef->count; i++) {
				printf(",%d", mutDef->cpus_given[i]);
			}
			printf("\n");
			break;
		case LOG_STATE:
			stateDef = (log_state_t *) &(entry->u);
			printf("t,%d,%d,%lu\n",stateDef->cpu,
			      		     stateDef->state,
					     stateDef->residency_time);
			break;
		default:
			debug("WTF?!!\n");
		}
	}
	return EXIT_SUCCESS;
}
