
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
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include <seeker.h>

#include "alloc.h"
#include "log.h"

struct log_block *seeker_log_head;
struct log_block *seeker_log_current;
static spinlock_t log_lock = SPIN_LOCK_UNLOCKED;
static int first_read = 1;

void log_init(void)
{
	init_seeker_cache();
	seeker_log_current = alloc_seeker();
	seeker_log_head = seeker_log_current;
	spin_lock_init(&log_lock);
	first_read = 1;
}

/* Create, add elements and link. */
struct log_block *log_create(void)
{
	struct log_block *p;
	p = alloc_seeker();
	if(!p){
		warn("Allocation failed");
		return NULL;
	}
	p->next = NULL;
	return p;
}

void log_link(struct log_block * ent)
{
	if(!seeker_log_current || !ent){
		warn("Current of ent is NULL");
		return;
	}
	spin_lock(&log_lock);
	seeker_log_current->next = ent;
	seeker_log_current = ent;
	spin_unlock(&log_lock);
}


void delete_log(struct log_block *ent)
{
	if(!ent){
		error("Trying to delete a NULL Block.");
		return;
	}
	free_seeker(ent);
}

/* IMP: Make sure writers are done before calling this. */
void purge_log(void)
{
	struct log_block *c1,*c2;
	/* First of all take the lock so no
	 * one will try to access while the
	 * purge is going on
	 */
	spin_lock(&log_lock);
	c1 = seeker_log_head;
	seeker_log_head = NULL;
	seeker_log_current = NULL;
	while(c1){
		c2 = c1->next;
		delete_log(c1);
		c1 = c2;
	}
	spin_unlock(&log_lock);
}

int log_read(struct file* file_ptr, 
	     char *buf, 
	     size_t count, 
	     loff_t *offset)
{
	struct log_block *log;
	int i = 0;
	int exit = 0;

	if(unlikely(seeker_log_head == NULL || buf == NULL || file_ptr == NULL)){
		warn("Trying to read or write from/to a NULL buf");
		return -1;
	}

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
	while(i+sizeof(seeker_sampler_entry_t) <= count 
	      && !exit 
	      && seeker_log_head != seeker_log_current){
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
	purge_log();
	finalize_seeker_cache();
}


