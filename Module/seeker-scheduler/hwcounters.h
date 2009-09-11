/*****************************************************
 * Copyright 2009 Amithash Prasad                    *
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

#ifndef _HW_COUNTERS_H_
#define _HW_COUNTERS_H_

/* Definations of the evtsel and mask 
 * for instructions retired,
 * real unhalted cycles
 * reference unhalted cycles 
 * for the AMD as it does not have fixed counters
 */
#define PMU_INST_EVTSEL 0xC0
#define PMU_INST_MASK 0x00
#define PMU_RECY_EVTSEL 0x76
#define PMU_RECY_MASK 0x00

void clear_counters(int cpu);
void enable_pmu_counters(void *info);
int configure_counters(void);
void exit_counters(void);
void read_counters(int cpu);

#endif
