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

#ifndef _C2DTSC_H_
#define _C2DTSC_H_

#include <asm/types.h>

/********** MSR's ************************************************************/

#define IA32_TIME_STAMP_COUNTER 0x000000010

/********** Structure Definitions ********************************************/

typedef struct {
	u32 low;
	u32 high;
	u32 last_low;
	u32 last_high;
}tstamp_t;

/********* Extern Vars *******************************************************/

extern tstamp_t time_stamp[NR_CPUS];


/********** Function Prototypes **********************************************/

void tsc_init_msrs(void);
void read_time_stamp(void);
u64 get_time_stamp(u32 cpu_id);
u64 get_last_time_stamp(u32 cpu_id);

#endif

