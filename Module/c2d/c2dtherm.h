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


#ifndef _C2DTHERM_H_
#define _C2DTHERM_H_

#include <asm/types.h>

/********** Constants ********************************************************/

#define IA32_THERM_STATUS 0x0000019C

#define THERM_VALID_MASK 0x80000000

/********* Extern Vars *******************************************************/

extern int temperature[NR_CPUS];
extern int TjMax[NR_CPUS];

/********** Function Prototypes **********************************************/

void therm_init_msrs(void);
int read_temp(void);
int get_temp(int);

#endif

