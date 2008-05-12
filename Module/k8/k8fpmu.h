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

#ifndef _K8FPMU_H_
#define _K8FPMU_H_

#include <asm/types.h>

/********** Constants ********************************************************/

#define NUM_FIXED_COUNTERS 0

/********* MSR's *************************************************************/


/********** Structure Definitions ********************************************/


/********* Extern Vars *******************************************************/


/********** Function Prototypes **********************************************/

void fpmu_init_msrs(void);

void fcounter_clear(u32 counter);
void fcounter_read(void);
u64 get_fcounter_data(u32 counter, u32 cpu_id);
void fcounters_disable(void);
void fcounters_enable(u32 os);

#endif
