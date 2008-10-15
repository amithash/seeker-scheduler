
/* Takes care of logging what the system
 * status is with respect to scheduling
 * and mutable cores.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <seeker.h>

#include "debug.h"

static struct file_operations seeker_debug_fops;
static struct miscdevice seeker_debug_mdev;
static int dev_open = 0;

ssize_t seeker_debug_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset)
{
	return debug_read(file_ptr, buf, count, offset);
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

	seeker_debug_mdev.minor = 240;
	seeker_debug_mdev.name  = "seeker_debug";
	seeker_debug_mdev.fops = &seeker_debug_fops;

	if(unlikely(misc_register(&seeker_debug_mdev) < 0)){
		error("seeker_debug device register failed");
		return -1;
	}
	return 0;
}

ssize_t debug_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset){


}

void debug_init(void)
{
	kmem_cache_create();

}

void debug_exit(void)
{
	kmem_cache_destroy();
}

