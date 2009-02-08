
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <seeker.h>

#include "alloc.h"
#include "log.h"
#include "io.h"
#include "intr.h"

extern int log_events[MAX_COUNTERS_PER_CPU];
extern unsigned int log_ev_masks[MAX_COUNTERS_PER_CPU];
extern int log_num_events;
extern int sample_freq;
extern int os_flag;
extern int pmu_intr;
extern int dev_open;

/*---------------------------------------------------------------------------*
 * Function: seeker_sample_log_read
 * Descreption: This is the "read" function. So daemons can read the buffer.
 * Input Parameters: Same params as a generic read function.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
ssize_t seeker_sample_log_read(struct file *file_ptr, char __user * buf,
			       size_t count, loff_t * offset)
{
	return log_read(file_ptr, buf, count, offset);
}

/*---------------------------------------------------------------------------*
 * Function: seeker_sample_open
 * Descreption: Seeker's file 'open' handle Creates and initializes the kernel 
 * 		buffer and enables PMU interrupts if required, and starts sampling
 * 		by setting dev_open to 1.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int seeker_sample_open(struct inode *in, struct file *f)
{

	int i, retval = 0;
	struct log_block *pentry;
	log_init();
	pentry = log_create();
	if (!pentry) {
		error("Creating log failed\n");
		return -1;
	}

	pentry->sample.type = SAMPLE_DEF;
	pentry->sample.u.seeker_sample_def.num_counters =
	    log_num_events + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS;
	for (i = 0; i < log_num_events; i++) {
		pentry->sample.u.seeker_sample_def.counters[i] = log_events[i];
		pentry->sample.u.seeker_sample_def.masks[i] = log_ev_masks[i];
	}

	/* add definations for the fixed counters:
	 * 0x01:0x00 = FIXED COUNTER 0; 
	 * 0x02:0x00 = FIXED COUNTER 1; 
	 * 0x03:0x00 = FIXED COUNTER 2; 
	 * 0x04:0x00 = TEMPERATURE
	 */
	pentry->sample.u.seeker_sample_def.counters[i] = 0x01;
	pentry->sample.u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->sample.u.seeker_sample_def.counters[i] = 0x02;
	pentry->sample.u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->sample.u.seeker_sample_def.counters[i] = 0x03;
	pentry->sample.u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->sample.u.seeker_sample_def.counters[i] = 0x04;
	pentry->sample.u.seeker_sample_def.masks[i] = 0x00;
	i++;

	/* Enable and configure interrupts on each cpu */
#ifdef LOCAL_PMU_VECTOR
	if (pmu_intr >= 0) {
		if (ON_EACH_CPU(configure_enable_interrupts, NULL, 1, 1) < 0) {
			error("Could not configure interrupts on all cpu's");
		}
	}
#endif
	retval = generic_open(in, f);
	dev_open = 1;
	return retval;
}

/*---------------------------------------------------------------------------*
 * Function: seeker_sample_close
 * Descreption: seeker's file close handle. Frees all log buffers, disables 
 * 		interrupts if enabled, and stops any sampling by setting
 * 		dev_open to 0.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int seeker_sample_close(struct inode *in, struct file *f)
{
	int retval = 0;
	dev_open = 0;
	/* Disable interrupts on each cpu. */
#ifdef LOCAL_PMU_VECTOR
	if (pmu_intr >= 0) {
		if (unlikely
		    (ON_EACH_CPU
		     ((void *)configure_disable_interrupts, NULL, 1, 1) < 0)) {
			error
			    ("Oops... Could not disable interrupts on all cpu's");
		}
	}
#endif
	log_finalize();
	retval = generic_close(in, f);
	return retval;
}

int generic_open(struct inode *i, struct file *f)
{
	return 0;
}

int generic_close(struct inode *i, struct file *f)
{
	return 0;
}
