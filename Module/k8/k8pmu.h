/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
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
#ifndef _K8PMU_H_
#define _K8PMU_H_

#include <asm/types.h>

/********** Constants ********************************************************/

#define NUM_COUNTERS 4
#define NUM_EVTSEL 4
#define EVTSEL_RESERVED_BITS 0x00200000

#define PERF_EVTSEL0 0xC0010000
#define PERF_EVTSEL1 0xC0010001
#define PERF_EVTSEL2 0xC0010002
#define PERF_EVTSEL3 0xC0010003

#define PERF_CTR0 0xC0010004
#define PERF_CTR1 0xC0010005
#define PERF_CTR2 0xC0010006
#define PERF_CTR3 0xC0010007

/********** Structure Definitions ********************************************/

typedef struct {
	u32 ev_select:8;
	u32 ev_mask:8;
	u32 usr_flag:1;
	u32 os_flag:1;
	u32 edge:1;
	u32 pc_flag:1;
	u32 int_flag:1;
	u32 rsvd:1;
	u32 enabled:1;
	u32 inv_flag:1;
	u32 cnt_mask:8;
	u32 addr;
} evtsel_t;


typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
	u32 evtsel_num;
	u32 enabled;
	u32 event_num;
} counter_t;

/********* Extern Vars *******************************************************/

extern evtsel_t evtsel[NR_CPUS][NUM_COUNTERS];
extern counter_t counters[NR_CPUS][NUM_COUNTERS];
extern char* evtsel_names[NUM_COUNTERS];
extern char* counter_names[NUM_COUNTERS];

/********** Function Prototypes **********************************************/

void pmu_init_msrs(void);

inline u32 evtsel_read(u32 evtsel_num);
inline void evtsel_clear(u32 evtsel_num);
inline void evtsel_write(u32 evtsel_num);

void counter_clear(u32 counter);
void counter_read(void);
u64 get_counter_data(u32 counter, u32 cpu_id);
void counter_disable(int counter);
int counter_enable(u32 event_num, u32 ev_mask, u32 os);

#endif

