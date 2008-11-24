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
#ifndef _FPMU_H_
#define _FPMU_H_
void fpmu_init_msrs(void);

void fcounter_clear(u32 counter);
void fcounter_read(void);
u64 get_fcounter_data(u32 counter, u32 cpu_id);
void fcounters_disable(void);
void fcounters_enable(u32 os);
int fpmu_configure_interrupt(int ctr, u32 low, u32 high);
int fpmu_enable_interrupt(int ctr);
int fpmu_disable_interrupt(int ctr);
int fpmu_clear_ovf_status(int ctr);
int fpmu_is_interrupt(int ctr);

#endif

