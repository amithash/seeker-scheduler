
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include <seeker.h>
#include "../generic.h"

#define P_ASSERT_EXIT(t,i) if(!(t)) {perror((i)); exit(EXIT_FAILURE);}
#define BUFFER_SIZE (4096*sizeof(log_t))

static char buf[BUFFER_SIZE];
static FILE *infile, *outfile;
char infile_name[] = "/dev/seeker_log";

char outfile_prefix[100], outfile_name[100];
int count = 0;
char count_c[20];
int do_sample = 1;

void usage(char *argv[])
{
	printf("Usage:\n");
	printf("\t%s LogFormat SampleTime\n", argv[0]);
	printf("\t%s LogFormat\t(SampleTime is assumed as 1 second)\n",
	       argv[0]);
	printf
	    ("\t%s\t(LogFormat is assumed to be /var/log/seeker, "
	     "SampleTime is assumed to be 1 second\n",
	     argv[0]);
}

void do_exit(void)
{
	printf("Exiting debugd\n");
	fflush(outfile);
	fclose(outfile);
	exit(EXIT_SUCCESS);
}

void catchSig()
{

	sigset_t mask;
	sigset_t oldMask;

	signal(SIGUSR1, catchSig);
	//Mask off other signals while servicing this one
	sigfillset(&mask);
	//oldMask contains old mask value which can be restored
	//Not really required to do it manually as OS does it for us.
	sigprocmask(SIG_SETMASK, &mask, &oldMask);
	debug("Got your message, writing to next file!!!");
	do_sample = 0;
}

void catchTerm()
{

	sigset_t mask;
	sigset_t oldMask;

	signal(SIGTERM, catchTerm);
	//Mask off other signals while servicing this one
	sigfillset(&mask);
	//oldMask contains old mask value which can be restored
	//Not really required to do it manually as OS does it for us.
	sigprocmask(SIG_SETMASK, &mask, &oldMask);
	debug("I am terminating!!!!");
	fclose(infile);
	do_exit();
}

unsigned int usleep_wrapper(unsigned int time)
{
	int a = usleep(time);
	return a;
}

int main(int argc, char **argv)
{
	int sleep_time;
	float sec_to_sleep;
	int use_usleep = 1;
	size_t bytes_read;
	struct timeval tv;
	pid_t pid;
	unsigned int (*seeker_sleep) (unsigned int);

	info("Seeker scheduler debug daemon started!");

	if (argc < 3) {
		if (argc == 2) {
			if (strcmp(argv[1], "--help") == 0
			    || strcmp(argv[1], "-h") == 0) {
				usage(argv);
				return 0;
			}
			/* Only 1 argument, time is assumed as 1 second */
			sec_to_sleep = 1;
			strcpy(outfile_prefix, argv[1]);
		} else {
			sec_to_sleep = 1;
			strcpy(outfile_prefix, "/var/log/seeker");
		}
	} else if (argc == 3) {
		sec_to_sleep = atof(argv[1]);
		strcpy(outfile_prefix, argv[2]);
	} else {
		usage(argv);
		return -1;
	}

	if (sec_to_sleep <= 0)
		sec_to_sleep = 1;

	if (sec_to_sleep >= 1.0) {
		sleep_time = (int)(sec_to_sleep + 0.5);	// round to the nearst integer.
		seeker_sleep = &sleep;
	} else {
		sleep_time = (int)(sec_to_sleep * 1000000);	// That many micro seconds...
		seeker_sleep = &usleep_wrapper;
	}

	strcpy(outfile_name, outfile_prefix);
	sprintf(count_c, "%d", count);
	strcat(outfile_name, count_c);

	P_ASSERT_EXIT(infile = fopen(infile_name, "r"), infile_name);
	if (access(outfile_name, F_OK) == 0) {
		error("file exists: %s", outfile_name);
		exit(EXIT_FAILURE);
	}

	P_ASSERT_EXIT(outfile = fopen(outfile_name, "a"), outfile_name);

	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	umask(0);

	signal(SIGUSR1, catchSig);
	signal(SIGTERM, catchTerm);

	while (1) {
		if (do_sample) {
			while ((bytes_read =
				fread(buf, 1, BUFFER_SIZE, infile)) > 0) {
				if (ferror(infile)) {
					do_exit();
				}
				bytes_read =
				    fwrite(buf, 1, bytes_read, outfile);
				if (ferror(outfile)) {
					do_exit();
				}
			}
			seeker_sleep(sleep_time);
			if (ferror(infile)) {
				do_exit();
			}
		} else {
			fclose(outfile);
			fclose(infile);
			count++;
			strcpy(outfile_name, outfile_prefix);
			sprintf(count_c, "%d", count);
			strcat(outfile_name, count_c);
			if (access(outfile_name, F_OK) == 0) {
				error("file exists: %s",
					outfile_name);
				exit(EXIT_FAILURE);
			}
			P_ASSERT_EXIT(outfile =
				      fopen(outfile_name, "a"), outfile_name);
			P_ASSERT_EXIT(infile =
				      fopen(infile_name, "r"), infile_name);

			do_sample = 1;
		}
	}

	return EXIT_SUCCESS;
}


