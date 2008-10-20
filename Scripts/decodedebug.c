/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
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

#include <seeker.h>


int
main(int argc, char **argv, char **envp){
	int i;
	int num_counters = -1;
	unsigned long long total_cycles[NR_CPUS] = {0};
	int first_sample[NR_CPUS] = {1};
	const int bufsize = sizeof(debug_t);
	char buf[bufsize];
	while( fread(buf, 1, bufsize, stdin) == bufsize ) {
		debug_t *entry = (debug_t *)(buf);
		switch(entry->type) {
			debug_scheduler_t *schDef;
			debug_mutator_t *mutDef;
			case DEBUG_SCH:
				schDef = (debug_scheduler_t *)(&entry->u);
				printf("s");
				printf(",%d,%d,%d\n",schDef->interval,schDef->pid,schDef->cpumask);
				break;
			case DEBUG_MUT:
				mutDef = (debug_mutator_t *)(&entry->u);
				printf("m,h");
				for(i=0;i<mutDef->count;i++){
					printf(",%d",mutDef->hint[i]);
				}
				printf(",s");
				for(i=0;i<NR_CPUS;i++){
					printf(",%d",mutDef->cpustates[i]);
				}
				printf("\n");
				break;
		}
	}
  
	return EXIT_SUCCESS;
}

