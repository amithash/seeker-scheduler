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
#include <linux/miscdevice.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "debug.h"

/********************************************************************************
 * 			Function Declarations 					*
 ********************************************************************************/

int seeker_debug_close(struct inode *in, struct file *f);
int seeker_debug_open(struct inode *in, struct file *f);
ssize_t seeker_debug_read(struct file *file_ptr, char __user * buf,
			  size_t count, loff_t * offset);

/********************************************************************************
 * 			Global Datastructures 					*
 ********************************************************************************/

/* spin lock for the writers reader, does not lock */
static DEFINE_SPINLOCK(debug_lock);

/* Start of the debug buffer list */
static struct debug_block *start_debug = NULL;

/* End of the buffer list */
static struct debug_block *current_debug = NULL;

/* flag if one indicates the first read operation. Cleared after the first read */
static int first_read = 1;

/* Flag indicating that the /dev interface is created */
static int dev_created = 0;

/* The /dev interface file operations structure */
static struct file_operations seeker_debug_fops = {
	.owner = THIS_MODULE,
	.open = seeker_debug_open,
	.release = seeker_debug_close,
	.read = seeker_debug_read,
};

/* misc dev structure for debug */
static struct miscdevice seeker_debug_mdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "seeker_debug",
	.fops = &seeker_debug_fops,
};

/* Flag set to 1 when the device is opened and cleared on a close */
static int dev_open = 0;

/* The slab cache for the buffer space for the debug buffer list */
static struct kmem_cache *debug_cachep = NULL;

/********************************************************************************
 * 				Functions 					*
 ********************************************************************************/


/********************************************************************************
 * get_debug - allocate and return a debug block
 * @return - address of the debug block, NULL if failed or device not opened.
 * @Side Effects - Writer's spin lock is taken.
 *
 * Allocate a block from the cache and return it when the device is opened. 
 ********************************************************************************/
struct debug_block *get_debug(void)
{
	struct debug_block *p = NULL;
	if (unlikely(!current_debug))
		return NULL;
	spin_lock(&debug_lock);
	if (unlikely(!dev_open || !current_debug || !debug_cachep))
		goto out;
	/* Just in case this was waiting for the lock
	 * and meanwhile, purge just set current_debug
	 * to NULL. */
	p = (struct debug_block *)kmem_cache_alloc(debug_cachep, GFP_ATOMIC);
	if (!p) {
		debug("Allocation failed");
		goto out;
	}
	p->next = NULL;
	return p;
out:
	spin_unlock(&debug_lock);
	return NULL;

}

/********************************************************************************
 * put_debug - Finalize the block
 * @p - the block allocated from get_debug
 * @Side Effects - Release spin lock and link into the list if p is not NULL. 
 *
 * Link in the block allocated by get_debug.
 ********************************************************************************/
void put_debug(struct debug_block *p)
{
	if (p) {
		current_debug->next = p;
		current_debug = p;
		spin_unlock(&debug_lock);
	}
}

/********************************************************************************
 * debug_free - free a block
 * @p - The address of the block to be freed
 * @Side Effects - None
 *
 * Frees the pointer from the cache.
 ********************************************************************************/
void debug_free(struct debug_block *p)
{
	if (!p || !debug_cachep)
		return;
	kmem_cache_free(debug_cachep, p);
}

/********************************************************************************
 * purge_debug - Purge the debug buffer list.
 * @return - None
 * @Side Effects - the debug buffer list is completely purged and the start and
 *                 end flag posts are set to NULL so no more writes can occur.
 *
 * Purges the buffer cache. Only done on a close or an exit. 
 ********************************************************************************/
void purge_debug(void)
{
	struct debug_block *c1, *c2;
	if (start_debug == NULL || current_debug == NULL)
		return;
	/* Acquire the lock, then set current debug to NULL
	 */
	debug("Starting safe section");
	spin_lock(&debug_lock);
	c1 = start_debug;
	start_debug = NULL;
	current_debug = NULL;
	spin_unlock(&debug_lock);
	debug("Ending safe section starting cleanup");
	while (c1) {
		c2 = c1->next;
		debug_free(c1);
		c1 = c2;
	}
	debug("Ended cleanup");
}

/********************************************************************************
 * seeker_debug_read - debug's read file operation. 
 * @file_ptr - The pointer to the open file. 
 * @buf - The pointer to the user space buffer
 * @count - Requested number of bytes to be read.
 * @offset - Offset from the file start - Ignored.
 * @return - Number of bytes copied to buf.
 * @ Side Effect - start_debug is changed and read blocks are freed.
 *
 * Read count or less number of bytes from the debug buffer list or till 
 * start_debug equals current_debug. This is what allows us to avoid readers
 * from taking the debug lock. For every block copied to the user space buffer,
 * it is freed and start_debug is made to point to the next block. 
 ********************************************************************************/
ssize_t seeker_debug_read(struct file *file_ptr, char __user * buf,
			  size_t count, loff_t * offset)
{
	struct debug_block *log;
	int i = 0;
	if (unlikely
	    (start_debug == NULL || buf == NULL || file_ptr == NULL
	     || start_debug == current_debug)) {
		return 0;
	}
	if (unlikely(first_read)) {
		debug("First read");
		if (unlikely(!start_debug->next))
			return 0;
		log = start_debug;
		start_debug = start_debug->next;
		debug_free(log);
		first_read = 0;
	}
	debug("Data Reading");
	while (i + sizeof(debug_t) <= count &&
	       start_debug->next && start_debug != current_debug) {
		memcpy(buf + i, &(start_debug->entry), sizeof(debug_t));
		log = start_debug;
		start_debug = start_debug->next;
		debug_free(log);
		i += sizeof(debug_t);
	}
	debug("Read %d bytes", i);
	return i;
}

/********************************************************************************
 * seeker_debug_open - Debug's open file operation.
 * @in - inode of the file - Ignored
 * @f  - the file - Ignored,
 * @return - 0 on success (Always returns 0) 
 * @Side Effect - dev_open is set so effectively starting the debug operations.
 ********************************************************************************/
int seeker_debug_open(struct inode *in, struct file *f)
{
	debug("Device opened");
	dev_open = 1;
	return 0;
}

/********************************************************************************
 * seeker_debug_close - Debug's close file operation
 * @in - inode of the file - Ignored
 * @f  - the file - Ignored
 * @return - 0 on success (Always returns 0)
 ********************************************************************************/
int seeker_debug_close(struct inode *in, struct file *f)
{
	debug("Device closed");
	dev_open = 0;
	return 0;
}

/********************************************************************************
 * debug_init - Init function for debug.
 * @return - 0 on success an error code otherwise. 
 * @Side Effects - Debug's device is registered, debug's cache is created,
 *                 Debug buffer list's first block is created and initialized,
 *                 start_debug and current_debug is set to this address,
 *                 first_read is set to request the read operation to ignore the 
 *                 first block.
 *
 * Initializes the various elements of the debug subsystem
 ********************************************************************************/
int debug_init(void)
{
	debug("Initing debug lock");
	spin_lock_init(&debug_lock);

	debug("Regestering misc device");
	if (unlikely(misc_register(&seeker_debug_mdev) < 0)) {
		error("seeker_debug device register failed");
		return -1;
	}
	debug("Creating cache");
	debug_cachep = kmem_cache_create("seeker_debug_cache",
					 sizeof(struct debug_block),
					 0, SLAB_PANIC, NULL);
	if (!debug_cachep) {
		error
		    ("Could not create debug cache, Debug unit will not be avaliable");
		goto err;
	}
	debug("Allocating first entry");
	current_debug =
	    (struct debug_block *)kmem_cache_alloc(debug_cachep, GFP_ATOMIC);
	if (!current_debug) {
		error("Initial allocation from the cache failed");
		kmem_cache_destroy(debug_cachep);
		goto err;
	}
	debug("Initing first entry");
	start_debug = current_debug;
	start_debug->next = NULL;

	dev_created = 1;
	first_read = 1;
	return 0;
err:
	error
	    ("Something went wrong in the debug init section. It will be unavaliable based on the implementation of the caller");
	misc_deregister(&seeker_debug_mdev);
	return -1;
}

/********************************************************************************
 * debug_exit - the finalizing function for debug. 
 * @Side Effect - De-registers the device, purges the buffer, destroys the cache.
 *
 * The function has to be called before the module is unloaded as it finalizes the
 * debug subsystem. 
 ********************************************************************************/
void debug_exit(void)
{
	debug("Exiting debug section");
	if (dev_open)
		dev_open = 0;

	if (dev_created) {
		debug("Deregestering the misc device");
		misc_deregister(&seeker_debug_mdev);
		debug("purging the log");
		purge_debug();
		debug("Destroying the cache");
		kmem_cache_destroy(debug_cachep);
		debug("Debug exit done!!!");
		dev_created = 0;
	}
}

