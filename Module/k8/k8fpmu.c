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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>

#include "k8fpmu.h"

/* K8 Architectures do not have fixed counters which are avaliable only for
 * the Intel Core Microarchitecture. And hence just to support that, this
 * driver is just a stub.
 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the fixed performance monitoring counters of the Core2Duo");


//must be called using on_each_cpu
inline void fcounter_clear(u32 counter){
	;
}
EXPORT_SYMBOL_GPL(fcounter_clear);

//must be called using on_each_cpu
void fcounter_read(void){
	;
}
EXPORT_SYMBOL_GPL(fcounter_read);

//use this to get the counter data
u64 get_fcounter_data(u32 counter, u32 cpu_id){
	return 0;
}
EXPORT_SYMBOL_GPL(get_fcounter_data);

//must be called using on_each_cpu
inline void fcounters_disable(void){
	;
}
EXPORT_SYMBOL_GPL(fcounters_disable);

//must be called using on_each_cpu
void fcounters_enable(u32 os) {
	;
}
EXPORT_SYMBOL_GPL(fcounters_enable);

//must be called from on_each_cpu
inline void fpmu_init_msrs(void) {
	;
}
EXPORT_SYMBOL_GPL(fpmu_init_msrs);

//must be called from on_each_cpu
static int __init fpmu_init(void){
	fpmu_init_msrs();
	return 0;
}

static void __exit fpmu_exit(void){
	;
}

module_init(fpmu_init);
module_exit(fpmu_exit);

