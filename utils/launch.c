/******************************************************************************\
 * FILE: launch.c
 * DESCRIPTION: The implementation of a untility to run a/set application(s)
 * on specific cores. Also prints the actual running time of each application.
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
#include <sched.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#include "launch.h"

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/********************************************************************************
 * main - the main function. Does the job.
 * @argc - total arguments
 * @argv - the arguments
 ********************************************************************************/
int main(int argc, char *argv[])
{

	int i, number_apps = 0, app_index = 0, app_number = 0;
	char *return_gr;
	int just = 0;
	pid_t pid;
	int *pids;
	char temp[20];
	char out[100];
	pid_t wpid;
	pid_t parent_pid;
	pid_t my_pid;
	int child_status;
	int kill_slow_children = 0;
	struct struct_app *apps;

	if (get_options(&kill_slow_children, argc, argv))
		print_usage();

	number_apps = count_num_apps(kill_slow_children, argc, argv);

	apps =
	    (struct struct_app *)calloc(number_apps, sizeof(struct struct_app));

	if (get_apps(apps, kill_slow_children, number_apps, argc, argv))
		print_usage();

	pids = (int *)calloc(number_apps, sizeof(int));

	app_number = -1;

	parent_pid = getpid();
	setpgid(parent_pid, parent_pid - 1);

	/* Fork number_apps children, each child has its
	 * app_number, when it breaks out. Children have
	 * pid set to 0. Only the parent will have a non
	 * -0 value.
	 */
	for (i = 0; i < number_apps; i++) {
		app_number++;
		pid = fork();
		if (pid != 0) {
			pids[app_number] = pid;
		} else {
			my_pid = getpid();
			setpgid(my_pid, my_pid);
			break;
		}
	}

	if (pid == 0) {
		/* child */
#ifdef DEBUG
		printf("I am child number %d\n", app_number);
#endif
		/* Set the thread's affinity to core */
		SET_AFFINITY(apps[app_number].core);
		/* Execute and get total execution time */
		apps[app_number].elapsed = launch(apps[app_number].prog);
		printf("\"%s\" %f\n", apps[app_number].cmd,
		       apps[app_number].elapsed);
#ifdef DEBUG
		printf("running: %s on core %d\n", apps[app_number].prog,
		       apps[app_number].core);
#endif
		exit(0);
	} else {
		/* Parent */

		/* Wait for the first child to exit */
		wpid = wait(&child_status);

		/* Print first child's exit status */
		if (WIFEXITED(child_status)) {
			fprintf(stderr, "process with pid = %d just finished "
				"successfully with status = %d\n", (int)wpid,
				WEXITSTATUS(child_status));
		}

		/* If we slower tasks must be killed (kill_slow_children = 1),
		 * then send the term signal to all the children's group.
		 */
		if (kill_slow_children) {
			for (i = 0; i < number_apps; i++) {
				if (pids[i] == wpid)
					continue;
				fprintf(stderr,
					"Killing all process with gid = %d\n",
					pids[i]);
				killpg(pids[i], SIGTERM);
			}
		}

		/* Wait for the remaining children. */
		for (i = 0; i < number_apps - 1; i++) {
			wpid = wait(&child_status);
			if (WIFEXITED(child_status)) {
				fprintf(stderr,
					"process with pid = %d just finished "
					"successfully with status = %d\n",
					(int)wpid, WEXITSTATUS(child_status));
			}
		}

	}

	return 0;
}

/********************************************************************************
 * print_usage - Print usage and exit;
 * @Side Effects - Exits the program
 *
 * Print the usage for launch and exit.
 ********************************************************************************/
void print_usage(void)
{
	printf("Usage: wrapper [-k] %s <cpu_id_1> <application_1> ", CORE);
	printf("[%s <cpu_id_2> <application_2> ....]\n", CORE);
	printf("try wrapper -h for help\n");
	exit(1);
}

/********************************************************************************
 * help - Print help and exit
 * @Side Effects - Exits the program 
 *
 * Prints the help screen and exits. 
 ********************************************************************************/
void help(void)
{
	printf("Usage: wrapper [-k] %s <cpumask1> <application_1> ", CORE);
	printf("[%s <cpumask2> <application_2> ....]\n\n", CORE);
	printf("-k \t\t\t\t[Optional] Kill all apps as soon as any one ");
	printf("of them exits, wrapper waits for all by default\n");
	printf("%s \t\t\t\tKeyword seperating the applications\n", CORE);
	printf("cpu_id_x ->\t\t\tThe cpumask on which"
	       " you want to execute application_x\n");
	printf("application_x ->\t\tThe applicaton you want to execute\n");
	printf("[ -- ] means optional\n\n");
	printf("Note that wrapper cannot handle I/O Redirects (<, >, &>)\n");
	printf("cleanly enclose the application with the I/O Redirects "
	       "in single quotes\n\n");
	exit(1);
}

/********************************************************************************
 * launch - Launch a command
 * @cmd - The string of the command to execute.
 * @return execution time.
 * @Side Effects - None
 *
 * Launchs a program using system call, and returns the execution
 * time in seconds.
 ********************************************************************************/
double launch(char *cmd)
{
	struct timeval start, end;
	int ret;
	gettimeofday(&start, NULL);
	ret = system(cmd);
	if (ret != 0)
		fprintf(stderr, "ERROR: %s failed with error code %d\n", cmd,
			ret);
	gettimeofday(&end, NULL);
	return in_sec(&start, &end);
}

/********************************************************************************
 * in_sec - convert timevals to a double val in seconds.
 * @start - The timeval taken at the start.
 * @end - The timeval taken at the end.
 * @return - end-start in seconds. 
 * @Side Effects - None.
 *
 * Convert start and end to double and then returns the difference which
 * is the elapsed time from start to end.
 ********************************************************************************/
double in_sec(struct timeval *start, struct timeval *end)
{
	double st, ed;
	st = (double)start->tv_sec + ((double)start->tv_usec / 1000000.0);
	ed = (double)end->tv_sec + ((double)end->tv_usec / 1000000.0);
	return ed - st;
}

/********************************************************************************
 * copy_cmd - Copy just the command from a  string.
 * @cmd - (out) output command.
 * @prog - (in) input command execution including parameters 
 * @max - The max size to copy, usually the size of the two buffers.
 *
 * Copies just the command part from prog to cmd. Thus leaving out parameters
 * and IO redirection instructions. 
 ********************************************************************************/
void copy_cmd(char *cmd, char *prog, int max)
{
	int i;
	for (i = 0; i < max; i++) {
		if (prog[i] == ' ' || prog[i] == '\0') {
			cmd[i] = '\0';
			break;
		}
		cmd[i] = prog[i];
	}
}

/********************************************************************************
 * get_options - get options passed by the command line.
 * @kill_slow - (out) the pointer to the kill_slow flag.
 * @argc - (in) number of arguments passed to launch.
 * @argv - (in) arguments passed to launch.
 * @Return - 0 on success -1 on failure.
 * @Side Effects - May call help() and exit if -h is one of the parameters.
 *
 * Get the options from the command line parameters. If -k is provided,
 * kill_slow is changed. If -h is provided, help() is directly called. 
 ********************************************************************************/
int get_options(int *kill_slow, int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		help();
	if (strcmp(argv[1], "-k") == 0 || strcmp(argv[1], "--kill-slow") == 0)
		*kill_slow = 1;
	if (argc < 4)
		return -1;
	return 0;
}

/********************************************************************************
 * count_num_apps - Count the total number of apps provided.
 * @kill_slow_flag - This is from get_options.
 * @argc - total parameters provided to launch.
 * @argv - Parameters provided to launch.
 * @return - Total apps to run.
 * @Side Effects - None.
 *
 * Count the number of applications to run from the command line parameters.
 *
 ********************************************************************************/
int count_num_apps(int kill_slow_flag, int argc, char *argv[])
{
	int num_apps = 0;
	int i;

	for (i = 1 + kill_slow_flag; i < argc; i++) {
		if (strcmp(argv[i], CORE) == 0) {
			num_apps++;
		}
	}
	return num_apps;
}

/********************************************************************************
 * get_apps - get applications to execute.
 * @a - (out) the array to populate. 
 * @kill_slow_flag - Set by get_options
 * @number_apps - Total number of applications.
 * @argc - Total parameters provided to launch.
 * @argv - parameters provided to launch.
 * @Return - 0 on success, -1 on failure. 
 * @Side Effects - None
 *
 * Extract the applications to execute and populate the array of struct_app
 * with this information.
 ********************************************************************************/
int get_apps(struct struct_app *a, int kill_slow_flag, int number_apps,
	     int argc, char *argv[])
{
	int app_index = -1;
	int just = 0;
	int i;
	char *return_gr;

	for (i = 1 + kill_slow_flag, app_index = -1; i < argc; i++) {
		if (strcmp(argv[i], CORE) == 0) {
			just = 1;
			app_index++;
		} else if (just == 1) {
			just++;
			a[app_index].core = atoi(argv[i]);
		} else if (just >= 2) {
			just++;
			return_gr = strcat(a[app_index].prog, argv[i]);
			return_gr = strcat(a[app_index].prog, " ");
		} else {
			return -1;
		}
	}
	if (just <= 2 || app_index != number_apps - 1) {
		return -1;
	}

	for (i = 0; i < number_apps; i++) {
		copy_cmd(a[i].cmd, a[i].prog, 300);
	}
	return 0;
}
