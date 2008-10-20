
/* Takes care of logging what the system
 * status is with respect to scheduling
 * and mutable cores.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <seeker.h>

#include "debug.h"


static spinlock_t debug_lock = SPIN_LOCK_UNLOCKED;

static struct debug_block *start_debug = NULL;
static struct debug_block *current_debug = NULL;

static int first_read = 1;

static struct file_operations seeker_debug_fops;
static struct miscdevice seeker_debug_mdev;
static int dev_open = 0;
static struct kmem_cache *debug_cachep = NULL;

struct debug_block *get_debug(void)
{
	struct debug_block *p;
	if(!dev_open)
		return NULL;

	p = (struct debug_block *)kmem_cache_alloc(debug_cachep, GFP_ATOMIC | GFP_DMA);
	p->next = NULL;
	return p;
}

void debug_free(struct debug_block *p)
{
	kmem_cache_free(debug_cachep,p);
}

void debug_link(struct debug_block *p)
{
	if(!current_debug){
		warn("Current or ent is null");
		return;
	}
	spin_lock(&debug_lock);
	current_debug->next = p;
	current_debug = p;
	spin_unlock(&debug_lock);
}

void purge_debug(void)
{
	struct debug_block *c1,*c2;
	spin_lock(&debug_lock);
	c1 = start_debug;
	start_debug = NULL;
	current_debug = NULL;
	while(c1){
		c2 = c1->next;
		debug_free(c1);
		c1 = c2;
	}
	spin_unlock(&debug_lock);
}
	


ssize_t seeker_debug_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset)
{
	struct debug_block *log;
	int i = 0;
	int exit = 0;
	if(unlikely(start_debug == NULL || buf == NULL || file_ptr == NULL)){
		warn("Nothing read");
		return -1;
	}
	if(unlikely(first_read)){
		if(unlikely(!start_debug->next))
			return -1;
		log = start_debug;
		start_debug = start_debug->next;
		debug_free(log);
		first_read = 0;
	}
	while(i+sizeof(debug_t) <= count &&
		!exit &&
		start_debug != current_debug){
		memcpy(buf+i,&(start_debug->entry),sizeof(debug_t));
		log = start_debug;
		if(log->next == NULL)
			exit = 1;
		start_debug = start_debug->next;
		debug_free(log);
		i += sizeof(debug_t);
	}
	return i;
}

int seeker_debug_open(struct inode *in, struct file *f)
{
	dev_open = 1;
	return 0;
}

int seeker_debug_close(struct inode *in, struct file *f)
{
	dev_open = 0;
	return 0;
}

int seeker_init_debug(void)
{
	seeker_debug_fops.open = seeker_debug_open;
	seeker_debug_fops.release = seeker_debug_close;
	seeker_debug_fops.read = seeker_debug_read;

	seeker_debug_mdev.minor = 241;
	seeker_debug_mdev.name  = "seeker_debug";
	seeker_debug_mdev.fops = &seeker_debug_fops;

	if(unlikely(misc_register(&seeker_debug_mdev) < 0)){
		error("seeker_debug device register failed");
		return -1;
	}
	return 0;
}

void debug_init(void)
{
	debug_cachep = kmem_cache_create("seeker_debug_cache",
					 sizeof(struct debug_block),
					 0,
					 SLAB_PANIC | SLAB_CACHE_DMA,
					 NULL);
	if(!debug_cachep){
		error("Could not create debug cache");
		return;
	}
	current_debug = kmem_cache_alloc(debug_cachep, GFP_ATOMIC);
	start_debug = current_debug;
	spin_lock_init(&debug_lock);
	seeker_init_debug();
	first_read = 1;
}

void debug_exit(void)
{
	purge_debug();
	kmem_cache_destroy(debug_cachep);
	misc_deregister(&seeker_debug_mdev);
}

