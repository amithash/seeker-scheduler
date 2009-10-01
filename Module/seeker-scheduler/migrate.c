/******************************************************************************\
 * FILE: migrate.c
 * DESCRIPTION: implements methodologies to migrate a task to a required cpuset
 * within the confines of the capabilities of a kernel module.
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "sched_debug.h"

/* migration pool size */
#define MIG_POOL_SIZE (NR_CPUS * 8)

/********************************************************************************
 * 			Local Prototype 					*
 ********************************************************************************/

struct mask_work {
	struct delayed_work work;
	struct task_struct *task;
	cpumask_t mask;
	int free;
};

/********************************************************************************
 * 				global_variables				*
 ********************************************************************************/

/* migration pool spin lock */
static DEFINE_SPINLOCK(mig_pool_lock);

/* The migration pool */
struct mask_work mig_pool[MIG_POOL_SIZE];

/********************************************************************************
 * change_cpus - Perform migration if required.
 * @w - The work struct responsible for this call.
 *
 * Perfom a migration if required for mw->task to mask mw->mask
 ********************************************************************************/
void change_cpus(struct work_struct *w)
{
	int retval;
	struct delayed_work *wrk = container_of(w, struct delayed_work, work);
	struct mask_work *mw = container_of(wrk, struct mask_work, work);
	struct task_struct *ts = mw->task;
	if (mw->free == 1)
		return;
	if (cpus_equal(mw->mask, ts->cpus_allowed)) {
		debug("No change required");
		goto change_cpus_out;
	}

	retval = set_cpus_allowed_ptr(ts, &(mw->mask));
change_cpus_out:
	mw->free = 1;
}

/********************************************************************************
 * init_mig_pool - initialize the migration pool
 *
 * Initialize all elements in the migration pool.
 ********************************************************************************/
void init_mig_pool(void)
{
	int i;
	for (i = 0; i < MIG_POOL_SIZE; i++) {
		INIT_DELAYED_WORK(&(mig_pool[i].work), change_cpus);
		mig_pool[i].free = 1;
	}
	spin_lock_init(&mig_pool_lock);
}

/********************************************************************************
 * exit_mig_pool - Cleanup and gracefully exit mig pools
 *
 * Cancel all delayed work and mark all pools as busy. 
 ********************************************************************************/
void exit_mig_pool(void)
{
	int i;
	spin_lock(&mig_pool_lock);
	for (i = 0; i < MIG_POOL_SIZE; i++) {
		if (mig_pool[i].free == 1) {
			mig_pool[i].free = 0;
		} else {
			cancel_delayed_work(&mig_pool[i].work);
			mig_pool[i].free = 0;
		}
	}
	spin_unlock(&mig_pool_lock);
}

/********************************************************************************
 * put_work - Start a migration work item.
 * @ts - The task for which a change might be required.
 * @mask - the mask for which ts should execute on.
 *
 * Start some delayed work to change the current executed cpus for ts
 * to mask.
 ********************************************************************************/
void put_work(struct task_struct *ts, cpumask_t mask)
{
	int i;
	spin_lock(&mig_pool_lock);
	for (i = 0; i < MIG_POOL_SIZE; i++) {
		if (mig_pool[i].free == 1) {
			mig_pool[i].free = 0;
			break;
		}
	}
	spin_unlock(&mig_pool_lock);
	if (i == MIG_POOL_SIZE) {
		sched_debug("Migrtion pool is empty");
		return;
	}

	PREPARE_DELAYED_WORK(&(mig_pool[i].work), change_cpus);
	mig_pool[i].mask = mask;
	mig_pool[i].task = ts;
	schedule_delayed_work(&(mig_pool[i].work), 1);
}

/********************************************************************************
 * cancel_task_work - Cancel all pending work for ts if any.
 *
 * Cancels all pending work if any for task ts. Should be called from
 * release_thread.
 ********************************************************************************/
void cancel_task_work(struct task_struct *ts)
{
	int i;
	spin_lock(&mig_pool_lock);
	for (i = 0; i < MIG_POOL_SIZE; i++) {
		if (mig_pool[i].free == 0 && ts->pid == mig_pool[i].task->pid) {
			cancel_delayed_work(&mig_pool[i].work);
			mig_pool[i].free = 1;
		}
	}
	spin_unlock(&mig_pool_lock);
}
