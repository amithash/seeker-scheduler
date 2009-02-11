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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <seeker.h>

#define IA32_MISC_ENABLE 0x00000000
#define EST_BIT 16
#define EST_MASK (1 << (EST_BIT-1))

static int init_enable_speedstep(void)
{
	u32 low,high;
	rdmsr(IA32_MISC_ENABLE,low,high);
	if(!(low & EST_MASK)){
		info("EST Has been disabled enabling it");
		low = low | EST_MASK;
		wrmsr(IA32_MISC_ENABLE,low,high);
		rdmsr(IA32_MISC_ENABLE,low,high);
		if(!(low & EST_MASK)){
			info("EST was enabled, but it still seems to be disabled! Too bad!");
		}
	} else {
		info("EST was found to be enabled");
	}
	return 0;
}

static void exit_enable_speedstep(void)
{
	;
}

module_init(init_enable_speedstep);
module_exit(exit_enable_speedstep);


