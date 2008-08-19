
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
#ifndef _SEEKER_SAMPLER_H_
#define _SEEKER_SAMPLER_H_
#include <seeker-headers.h>

#define NUM_EXTRA_COUNTERS 1
#define MAX_COUNTERS_PER_CPU NUM_COUNTERS + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS

enum {SAMPLE_DEF, SEEKER_SAMPLE, PIDTAB_ENTRY};

typedef struct {
	unsigned char num_counters;
	unsigned char counters[MAX_COUNTERS_PER_CPU];
	unsigned int masks[MAX_COUNTERS_PER_CPU];
} seeker_sample_def_t;

typedef struct {
	unsigned int cpu;
	unsigned long long cycles;
	unsigned int pid;
	unsigned long long counters[MAX_COUNTERS_PER_CPU];
} seeker_sample_t; 

typedef struct {
	unsigned int pid;
	char name[16];
	unsigned long long instr_sum;
	unsigned long long total_cycles;
	unsigned long long cpu_cycles;
} pidtab_entry_t;

typedef struct {
	int type;
	union {
		seeker_sample_def_t seeker_sample_def;
		seeker_sample_t seeker_sample;
		pidtab_entry_t pidtab_entry;
	} u;
} seeker_sampler_entry_t;

#if !defined(KERNEL_VERSION)
# define KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE + 1)
#endif

/* Error and warn hash defines kern meaning is increased on purpose... */
#define error(str,a...) printk(KERN_EMERG "SEEKER ERROR[%s : %d]: " str "\n",__FILE__,__LINE__, ## a)
#define warn(str,a...) printk(KERN_ERR "SEEKER WARN[%s : %d]: " str "\n",__FILE__,__LINE__, ## a)
#define debug(str,a...) printk(KERN_INFO "SEEKER DEBUG[%s : %d]: " str "\n",__FILE__,__LINE__, ## a)

#endif
