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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/moduleparam.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

#include "user.h"
#include "freq.h"
#include "interface.h"


/********************************************************************************
 * 				Module Parameters 				*
 ********************************************************************************/

/* parameter for user to specify states allowed */
int allowed_states[MAX_STATES];

/* Length of the allowed_states array */
int allowed_states_length = 0;

/********************************************************************************
 * 				Functions					*
 ********************************************************************************/

/*******************************************************************************
 * seeker_cpufreq_init - Init function for seeker_cpufreq
 * @return  - 0 if success, error code otherwise.
 * @Side Effects - freq_info is populated by reading from cpufreq_table.
 *
 * Initialize seeker_cpufreq's data structures and register self as a governor.
 *******************************************************************************/
static int __init seeker_cpufreq_init(void)
{
	register_self();

	init_user_interface();

	init_freqs();

	if(init_freqs()){
		deregister_self();
		return -ENODEV;
	}

	return 0;
}

/*******************************************************************************
 * seeker_cpufreq_exit - seeker_cpufreq's exit routine.
 *
 * Unregister self as a governor.
 *******************************************************************************/
static void __exit seeker_cpufreq_exit(void)
{
	exit_user_interface();
	deregister_self();
}

/********************************************************************************
 * 			Module Parameters 					*
 ********************************************************************************/

module_init(seeker_cpufreq_init);
module_exit(seeker_cpufreq_exit);

module_param_array(allowed_states, int, &allowed_states_length, 0444);
MODULE_PARM_DESC(allowed_states, "Use this to provide an array for allowed states.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Provides abstracted access to the cpufreq driver");


