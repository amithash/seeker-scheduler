/******************************************************************************\
 * FILE: log.c
 * DESCRIPTION: 
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 * Copyright 2006 Tipp Mosely                                                  *
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

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "log.h"
#include "state.h"

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

int seeker_log_close(struct inode *in, struct file *f);
int seeker_log_open(struct inode *in, struct file *f);
ssize_t seeker_log_read(struct file *file_ptr, char __user * buf,
			  size_t count, loff_t * offset);


void log_free(struct log_block *p);

/********************************************************************************
 * 			External Variables 					*
 ********************************************************************************/

/* main.c: total online cpus */
extern int total_online_cpus;

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* spin lock for the writers reader, does not lock */
static DEFINE_SPINLOCK(log_lock);

/* Start of the log buffer list */
static struct log_block *start_log = NULL;

/* End of the buffer list */
static struct log_block *current_log = NULL;

/* flag if one indicates the first read operation. Cleared after the first read */
static int first_read = 1;

/* Flag indicating that the /dev interface is created */
static int dev_created = 0;

/* The /dev interface file operations structure */
static struct file_operations seeker_log_fops = {
	.owner = THIS_MODULE,
	.open = seeker_log_open,
	.release = seeker_log_close,
	.read = seeker_log_read,
};

/* misc dev structure for log */
static struct miscdevice seeker_log_mdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "seeker_log",
	.fops = &seeker_log_fops,
};

/* Flag set to 1 when the device is opened and cleared on a close */
static int dev_open = 0;

/* The slab cache for the buffer space for the log buffer list */
static struct kmem_cache *log_cachep = NULL;

/********************************************************************************
 * 				Functions 					*
 ********************************************************************************/

/********************************************************************************
 * get_log - allocate and return a log block
 * @return - address of the log block, NULL if failed or device not opened.
 * @Side Effects - Writer's spin lock is taken.
 *
 * Allocate a block from the cache and return it when the device is opened. 
 ********************************************************************************/
struct log_block *get_log(void)
{
	struct log_block *p = NULL;
	if (unlikely(!current_log))
		return NULL;
	spin_lock(&log_lock);
	if (unlikely(!dev_open || !current_log || !log_cachep))
		goto out;
	/* Just in case this was waiting for the lock
	 * and meanwhile, purge just set current_log
	 * to NULL. */
	p = (struct log_block *)kmem_cache_alloc(log_cachep, GFP_ATOMIC);
	if (!p) {
		debug("Allocation failed");
		goto out;
	}
	p->next = NULL;
	return p;
out:
	spin_unlock(&log_lock);
	return NULL;

}

/********************************************************************************
 * put_log - Finalize the block
 * @p - the block allocated from get_log
 * @Side Effects - Release spin lock and link into the list if p is not NULL. 
 *
 * Link in the block allocated by get_log.
 ********************************************************************************/
void put_log(struct log_block *p)
{
	if (p) {
		current_log->next = p;
		current_log = p;
		spin_unlock(&log_lock);
	}
}

/********************************************************************************
 * log_free - free a block
 * @p - The address of the block to be freed
 * @Side Effects - None
 *
 * Frees the pointer from the cache.
 ********************************************************************************/
void log_free(struct log_block *p)
{
	if (!p || !log_cachep)
		return;
	kmem_cache_free(log_cachep, p);
}

/********************************************************************************
 * purge_log - Purge the log buffer list.
 * @return - None
 * @Side Effects - the log buffer list is completely purged and the start and
 *                 end flag posts are set to NULL so no more writes can occur.
 *
 * Purges the buffer cache. Only done on a close or an exit. 
 ********************************************************************************/
void purge_log(void)
{
	struct log_block *c1, *c2;
	if (start_log == NULL || current_log == NULL)
		return;
	/* Acquire the lock, then set current log to NULL
	 */
	debug("Starting safe section");
	spin_lock(&log_lock);
	c1 = start_log;
	start_log = NULL;
	current_log = NULL;
	spin_unlock(&log_lock);
	debug("Ending safe section starting cleanup");
	while (c1) {
		c2 = c1->next;
		log_free(c1);
		c1 = c2;
	}
	debug("Ended cleanup");
}

/********************************************************************************
 * seeker_log_read - log's read file operation. 
 * @file_ptr - The pointer to the open file. 
 * @buf - The pointer to the user space buffer
 * @count - Requested number of bytes to be read.
 * @offset - Offset from the file start - Ignored.
 * @return - Number of bytes copied to buf.
 * @ Side Effect - start_log is changed and read blocks are freed.
 *
 * Read count or less number of bytes from the log buffer list or till 
 * start_log equals current_log. This is what allows us to avoid readers
 * from taking the log lock. For every block copied to the user space buffer,
 * it is freed and start_log is made to point to the next block. 
 ********************************************************************************/
ssize_t seeker_log_read(struct file *file_ptr, char __user * buf,
			  size_t count, loff_t * offset)
{
	struct log_block *log;
	int i = 0;

	if (unlikely
	    (start_log == NULL || buf == NULL || file_ptr == NULL
	     || start_log == current_log)) {
		return 0;
	}
	if (unlikely(first_read)) {
		debug("First read");
		if (unlikely(!start_log->next))
			return 0;
		log = start_log;
		start_log = start_log->next;
		log_free(log);
		first_read = 0;
	}
	debug("Data Reading");
	while (i + sizeof(log_t) <= count &&
	       start_log->next && start_log != current_log) {
		memcpy(buf + i, &(start_log->entry), sizeof(log_t));
		log = start_log;
		start_log = start_log->next;
		log_free(log);
		i += sizeof(log_t);
	}
	debug("Read %d bytes", i);
	return i;
}

/********************************************************************************
 * seeker_log_open - Debug's open file operation.
 * @in - inode of the file - Ignored
 * @f  - the file - Ignored,
 * @return - 0 on success (Always returns 0) 
 * @Side Effect - dev_open is set so effectively starting the log operations.
 ********************************************************************************/
int seeker_log_open(struct inode *in, struct file *f)
{
	debug("Device opened");
	start_state_logger();
	dev_open = 1;
	return 0;
}

/********************************************************************************
 * seeker_log_close - Debug's close file operation
 * @in - inode of the file - Ignored
 * @f  - the file - Ignored
 * @return - 0 on success (Always returns 0)
 ********************************************************************************/
int seeker_log_close(struct inode *in, struct file *f)
{
	debug("Device closed");
	dev_open = 0;
	stop_state_logger();
	return 0;
}

/********************************************************************************
 * log_init - Init function for log.
 * @return - 0 on success an error code otherwise. 
 * @Side Effects - Debug's device is registered, log's cache is created,
 *                 Debug buffer list's first block is created and initialized,
 *                 start_log and current_log is set to this address,
 *                 first_read is set to request the read operation to ignore the 
 *                 first block.
 *
 * Initializes the various elements of the log subsystem
 ********************************************************************************/
int log_init(void)
{
	debug("Initing log lock");
	spin_lock_init(&log_lock);

	debug("Regestering misc device");
	if (unlikely(misc_register(&seeker_log_mdev) < 0)) {
		error("seeker_log device register failed");
		return -1;
	}
	debug("Creating cache");
	log_cachep = kmem_cache_create("seeker_log_cache",
					 sizeof(struct log_block),
					 0, SLAB_PANIC, NULL);
	if (!log_cachep) {
		error
		    ("Could not create log cache, Debug unit will not be avaliable");
		goto err;
	}
	debug("Allocating first entry");
	current_log =
	    (struct log_block *)kmem_cache_alloc(log_cachep, GFP_ATOMIC);
	if (!current_log) {
		error("Initial allocation from the cache failed");
		kmem_cache_destroy(log_cachep);
		goto err;
	}
	debug("Initing first entry");
	start_log = current_log;
	start_log->next = NULL;

	dev_created = 1;
	first_read = 1;
	return 0;
err:
	error("Something went wrong in the log init section. "
      "It will be unavaliable based on the implementation of the caller");
	misc_deregister(&seeker_log_mdev);
	return -1;
}

/********************************************************************************
 * log_exit - the finalizing function for log. 
 * @Side Effect - De-registers the device, purges the buffer, destroys the cache.
 *
 * The function has to be called before the module is unloaded as it finalizes the
 * log subsystem. 
 ********************************************************************************/
void log_exit(void)
{
	debug("Exiting log section");
	if (dev_open)
		dev_open = 0;

	if (dev_created) {
		debug("Deregestering the misc device");
		misc_deregister(&seeker_log_mdev);
		debug("purging the log");
		purge_log();
		debug("Destroying the cache");
		kmem_cache_destroy(log_cachep);
		debug("Debug exit done!!!");
		dev_created = 0;
	}
}
