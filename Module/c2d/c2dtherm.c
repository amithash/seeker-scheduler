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

#include "c2dtherm.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to access the per-core temperature sensor of the Core2Duo");

int temperature[NR_CPUS] = {0};
int TjMax[NR_CPUS] = {0};

int read_temp(void){
	u32 low,high;
	//read the temperature
	int cpu = smp_processor_id();

	rdmsr(IA32_THERM_STATUS,low,high);
	if(unlikely((low & THERM_VALID_MASK) == 0)){
		return -1;
	}
	else{
		low = 0x7F & (low >> 16); // only 7 bits please...
		temperature[cpu] = TjMax[cpu] - low;
		return 0;
	}
}
EXPORT_SYMBOL_GPL(read_temp);

int get_temp(int cpu){
	return temperature[cpu];
}
EXPORT_SYMBOL_GPL(get_temp);

void therm_init_msrs(void){
	u32 low,high;
	int cpu = smp_processor_id();
	temperature[cpu] = 0;
	
	/* get the value of TjMax 
	 * There is an undocumented MSR 0xEE
	 * when bit 30 of this is set then
	 * it is assumed that Tj_Max is 100
	 * degree celsius, else it has to be
	 * assumed that Tj_Max is 85 degree 
	 * celsius.
	 * reference:
	 * http://softwarecommunity.intel.com/isn/Community/en-US/forums/thread/30222546.aspx
	 *
	 */

	rdmsr(0xEE,low,high);
	if(low & 0x40000000){ // bit 30 if the MSR 0xEE
		// is set. TjMax is 100 degree C
		TjMax[cpu] = 100;
	}
	else{
		TjMax[cpu] = 85;
	}

	//perform the first readout.
	read_temp();
}
EXPORT_SYMBOL_GPL(therm_init_msrs);

//must be called from on_each_cpu
static int __init therm_init(void){
	therm_init_msrs();
	return 0;
}

static void __exit therm_exit(void){
	;
}

module_init(therm_init);
module_exit(therm_exit);

