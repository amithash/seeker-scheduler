
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
#ifndef __INTR_H_
#define __INTR_H_


struct struct_int_callbacks{
	int (*enable_interrupts)(int);
	int (*disable_interrupts)(int);
	int (*configure_interrupts)(int, u32, u32);
	int (*clear_ovf_status)(int);
	int (*is_interrupt)(int);
};

extern struct struct_int_callbacks int_callbacks;

void do_timer_sample(unsigned long param);
#ifdef LOCAL_PMU_VECTOR
void configure_enable_interrupts(void);
void configure_disable_interrupts(void);
void enable_apic_pmu(void);
#endif

#endif
