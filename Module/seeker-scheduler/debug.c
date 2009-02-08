
/* Takes care of logging what the system
 * status is with respect to scheduling
 * and mutable cores.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "debug.h"

static DEFINE_SPINLOCK(debug_lock);

static struct debug_block *start_debug = NULL;
static struct debug_block *current_debug = NULL;

static int first_read = 1;
static int dev_created = 0;

int seeker_debug_close(struct inode *in, struct file *f);
int seeker_debug_open(struct inode *in, struct file *f);
ssize_t seeker_debug_read(struct file *file_ptr, char __user * buf,
			  size_t count, loff_t * offset);

static struct file_operations seeker_debug_fops = {
	.owner = THIS_MODULE,
	.open = seeker_debug_open,
	.release = seeker_debug_close,
	.read = seeker_debug_read,
};

static struct miscdevice seeker_debug_mdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "seeker_debug",
	.fops = &seeker_debug_fops,
};

static int dev_open = 0;
static struct kmem_cache *debug_cachep = NULL;

/* Returns a pointer and takes a lock if allocation is
 * successful. Do not waste time. Fill it and call
 * put_debug asap! */
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

/* Releases the spinlock */
void put_debug(struct debug_block *p)
{
	if (p) {
		/* Update and then release the lock */
		current_debug->next = p;
		current_debug = p;
		spin_unlock(&debug_lock);
	}
}

void debug_free(struct debug_block *p)
{
	if (!p || !debug_cachep)
		return;
	kmem_cache_free(debug_cachep, p);
}

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

int seeker_debug_open(struct inode *in, struct file *f)
{
	debug("Device opened");
	dev_open = 1;
	return 0;
}

int seeker_debug_close(struct inode *in, struct file *f)
{
	debug("Device closed");
	dev_open = 0;
	return 0;
}

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
