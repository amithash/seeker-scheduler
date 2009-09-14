
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
