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


int main(int argc, char **argv, char **envp)
{
	int i;
	int num_counters = -1;
	unsigned long long total_cycles[NR_CPUS] = {0};
	int first_sample[NR_CPUS] = {1};
	int debug = 0;
	const int bufsize = sizeof(seeker_sampler_entry_t);
	const int debug_bufsize = sizeof(debug_t);
	char buf[bufsize];
	char debug_buf[debug_bufsize];

	if(argc > 1){
		if(strcmp(argv[1],"-d") == 0){
			goto debug_start;
		}
	}

	fprintf(stderr, "seeker!\n");

	while( fread(buf, 1, bufsize, stdin) == bufsize ) {
		seeker_sampler_entry_t *entry = (seeker_sampler_entry_t *)(buf);
		switch(entry->type) {
			seeker_sample_def_t *sampleDef;
			seeker_sample_t *sample;
			pidtab_entry_t *pidEntry;
			case SAMPLE_DEF:
				sampleDef = (seeker_sample_def_t *)(&entry->u);
				printf("d");
				num_counters = sampleDef->num_counters;
				for( i = 0; i < sampleDef->num_counters; i++ ) {
					printf(",%d,0x%x", sampleDef->counters[i], sampleDef->masks[i]);
				}
				printf("\n");
				break;
			case SEEKER_SAMPLE:
				sample = (seeker_sample_t *)(&entry->u);
				if( first_sample[sample->cpu] ) {
					first_sample[sample->cpu] = 0;
					break;
				}
				/* s, cpu, pid, scycles, tcycles{cpu},  */
				printf("s,");

				total_cycles[sample->cpu] += sample->cycles;
				printf("%u,%u,", sample->cpu, sample->pid);
				printf("%llu,", sample->cycles);
				printf("%llu",total_cycles[sample->cpu]);
				for( i = 0; i < num_counters; i++ ) {
					printf(",%u", sample->counters[i]);
				}
				printf("\n");
				break;
			case PIDTAB_ENTRY:
				pidEntry = (pidtab_entry_t *)(&entry->u);
				printf("p,%d,%s,%llu\n", pidEntry->pid, pidEntry->name, pidEntry->total_cycles); 
				break;
		}
	}
	goto out;
debug_start:
	fprintf(stderr, "debug!\n");
	while( fread(debug_buf, 1, debug_bufsize, stdin) == debug_bufsize ) {
		debug_t *entry = (debug_t *)(debug_buf);
		switch(entry->type) {
			debug_scheduler_t *schDef;
			debug_mutator_t *mutDef;
			debug_pid_t *pidDef;
			case DEBUG_SCH:
				schDef = (debug_scheduler_t *)(&entry->u);
				printf("s");
				printf(",%d,%d,%d,%ld,%1.4f,%d,%d\n",schDef->interval,
								schDef->pid,
								schDef->cpu,
								schDef->inst,
								((float)schDef->ipc)/8.0,
								schDef->state_req,
								schDef->state_given
								);

				break;
			case DEBUG_PID:
				pidDef = (debug_pid_t *)(&entry->u);
				printf("p");
				pidDef->name[15] = '\0';
				printf(",%d,%s\n",pidDef->pid,pidDef->name);
				break;
			case DEBUG_MUT:
				mutDef = (debug_mutator_t *)(&entry->u);
				printf("m,%ld,r",mutDef->interval);
				for(i=0;i<mutDef->count;i++){
					printf(",%d",mutDef->cpus_req[i]);
				}
				printf(",g");
				for(i=0;i<mutDef->count;i++){
					printf(",%d",mutDef->cpus_given[i]);
				}
				printf("\n");
				break;
			default:
				fprintf(stderr,"WTF?!!\n");
		}
	}
out:
  
	return EXIT_SUCCESS;
}

