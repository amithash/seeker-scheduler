#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include "seeker.h"
#include "alloc.h"
#include "log.h"

struct log_block *seeker_log_head;
struct log_block *seeker_log_current;
static spinlock_t log_lock;
static int first_read = 1;

void log_init(void)
{
	init_seeker_cache();
	seeker_log_head = seeker_log_current = alloc_seeker();
	spin_lock_init(&log_lock);
	first_read = 1;
}

/* Create, add elements and link. */
struct log_block *log_create(void)
{
	return alloc_seeker();
}

void log_link(struct log_block * ent)
{
	spin_lock(&log_lock);
	seeker_log_current->next = ent;
	seeker_log_current = ent;
	spin_unlock(&log_lock);
}


void delete_log(struct log_block *ent)
{
	free_seeker(ent);
}

/* IMP: Make sure writers are done before calling this. */
void purge_log(void)
{
	struct log_block *c1,*c2;
	c1 = seeker_log_head;
	seeker_log_head = NULL;
	seeker_log_current = NULL;
	while(c1){
		c2 = c1->next;
		delete_log(c1);
		c1 = c2;
	}
}

int log_read(struct file* file_ptr, 
	     char *buf, 
	     size_t count, 
	     loff_t *offset)
{
	struct log_block *log;
	int i = 0;
	int exit = 0;

	if(unlikely(seeker_log_head == NULL || buf == NULL || file_ptr == NULL))
		return -1;

	if(unlikely(first_read)){
		if(unlikely(!seeker_log_head->next))
			return -1;
		log = seeker_log_head;
		seeker_log_head = seeker_log_head->next;
		delete_log(log);
		first_read = 0;
	}

		
	/* Read max till seeker_log_current, do not read seeker_log_current 
	 * Trying to read seeker_log_current will make the reader
	 * a competetor with the writers and creates the 
	 * need for a lock.
	 */
	while(i+sizeof(seeker_sampler_entry_t) <= count && !exit && seeker_log_head != seeker_log_current){
		memcpy(buf+i,&(seeker_log_head->sample),sizeof(seeker_sampler_entry_t));	
		/* Check if this is the last */
		log = seeker_log_head;
		if(log->next == NULL)
			exit = 1;
		seeker_log_head = seeker_log_head->next;
		delete_log(log);
		i+=sizeof(seeker_sampler_entry_t);
	}
	return i;
}

void log_finalize(void)
{
	/* First make sure there are not going to be any writers...
	 * This is done by setting seeker_log_current and seeker_log_head to NULL.
	 */
	purge_log();
	finalize_seeker_cache();
}


