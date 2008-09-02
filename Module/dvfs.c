
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


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/msr.h>
#include <linux/smp.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>

#include "seeker.h"
#include "dvfs.h"
#include "dvfs_int.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module provides an interface to change "
		   "the P-States of specific cores");
/* The file operations and the mdev structures */
static struct file_operations dvfs_fops[NR_CPUS];
static struct miscdevice dvfs_mdev[NR_CPUS];
static int mdev_registered[NR_CPUS] = {0};

unsigned int pstate[NR_CPUS] = {0};

/* Array of func pointers for the read and write funcs */
ssize_t (*dvfs_read[50])(struct file *, char __user *, size_t, loff_t *) = {
	MAKE_RCB_10()
	,MAKE_RCB_10(1)
	,MAKE_RCB_10(2)
	,MAKE_RCB_10(3)
	,MAKE_RCB_10(4)
};


ssize_t (*dvfs_write[50])(struct file *, const char __user *, size_t, loff_t *) = {
	MAKE_WCB_10()
	,MAKE_WCB_10(1)
	,MAKE_WCB_10(2)
	,MAKE_WCB_10(3)
	,MAKE_WCB_10(4)
};

int stoi(char *str);

/* Open and close are stubs */
int generic_open(struct inode *i, struct file *f) 
{
	return 0;
}
int generic_close(struct inode *i, struct file *f) 
{
	return 0;
}

/* Defining read and write funcs from dvfs_*0 to dvfs_*49 */
MAKE_10()
MAKE_10(1)
MAKE_10(2)
MAKE_10(3)
MAKE_10(4)

/* dvfs_readX calls this with cpu = X */
ssize_t dvfs_read_cpu(struct file *file_ptr, char __user *buf, 
			size_t count, loff_t *offset, int cpu)
{
	int val;
	pstate_read(cpu);
	val = pstate[cpu];
	if(val < 10){
		buf[0] = val + 48; /*Char it */
		buf[1] = '\0';
	} else {
		buf[1] = val % 10;
		buf[0] = ((val / 10) % 10)+48;
		buf[2] = '\0';
	}

	return 0;
}

/* dvfs_writeX calls this with cpu = X */
ssize_t dvfs_write_cpu(struct file *file_ptr, const char __user *buf,	
			      size_t count, loff_t *offset, int cpu){
	pstate[cpu] = (unsigned int) stoi((char *)buf);
	pstate_write(cpu);
	return 1;
}

/* Read pstate on cpu */
void pstate_read(int cpu)
{
	int this_cpu = smp_processor_id();
	if(this_cpu == cpu)
		__pstate_read(NULL);
	else 
		smp_call_function_single(cpu,__pstate_read,NULL,1,1);
}
EXPORT_SYMBOL(pstate_read);

/* Write pstate on cpu */
void pstate_write(int cpu)
{
	int this_cpu = smp_processor_id();
	if(this_cpu == cpu)
		__pstate_write(NULL);
	else 
		smp_call_function_single(cpu,__pstate_write,NULL,1,1);
}
EXPORT_SYMBOL(pstate_write);

/* called by pstate_read */
void __pstate_read(void *info)
{
	u32 low,high;
	int cpu = get_cpu();
	rdmsr(PERF_STATUS,low,high);
	pstate[cpu] = low | PERF_MASK;
	put_cpu();
}


/* called by pstate_write */
void __pstate_write(void *info)
{
	u32 high = 0;
	u32 low;
	int cpu = get_cpu();
	low = pstate[cpu];
	wrmsr(PERF_CTL,low,high);
	put_cpu();
}

/* Return the current pstate */
unsigned int get_pstate(int cpu)
{
	return pstate[cpu];
}
EXPORT_SYMBOL(get_pstate);

/* set the current pstate */
void put_pstate(int cpu, unsigned int state)
{
	pstate[cpu] = state | PERF_MASK;
}
EXPORT_SYMBOL(put_pstate);

int stoi(char *str)
{
	int val = 0;
	int i=0;
	while(str[i] != '\0'){
		if(str[i] >= 48 && str[i]<= 57){
			val = val * 10;
			val += (str[i] - 48);
		} else {
			warn("Non decimal char ignored: %c\n",str[i]);
		}
		i++;
	}
	return val;
}

/* Initialize the mdev */
static int mdev_node_init(void)
{
	int i;
	int cpus = num_online_cpus();
	char name[20];
	if(cpus >= 50){
		error("%d CPUS NOT SUPPORTED\n",NR_CPUS);
		return -1;
	}

	for(i=0;i<cpus;i++){
		dvfs_fops[i].open = generic_open;
		dvfs_fops[i].release = generic_close;
		dvfs_fops[i].read = dvfs_read[i];
		dvfs_fops[i].write = dvfs_write[i];
		dvfs_mdev[i].minor = i;
		dvfs_mdev[i].fops = &(dvfs_fops[i]);
		dvfs_mdev[i].name = (const char *)kmalloc(sizeof(const char) * 20, GFP_ATOMIC);
		sprintf(name,"pstate%d",i);
		strcpy((char *)dvfs_mdev[i].name,name);
		if(unlikely(misc_register(&dvfs_mdev[i]) < 0)) {
			error("Device %s register failed",dvfs_mdev[i].name);
		} else {
			mdev_registered[i] = 1;
		}
	}
	return 0;
}

/* Un-initialize the mdev */
static int mdev_node_exit(void)
{
	int i;
	for(i=0;i<NR_CPUS;i++){
		if(mdev_registered[i] == 1){
			mdev_registered[i] = 0;
			misc_deregister(&dvfs_mdev[i]);
		}
	}
	return 0;
}

/* Init and exit Funcs */
static int __init dvfs_init(void)
{
	mdev_node_init();
	return 0;
}

static void __exit dvfs_exit(void)
{
	mdev_node_exit();
}


module_init(dvfs_init);
module_exit(dvfs_exit);

