
#include "alloc.h"
#include "log.h"

static log_block_t *head;
static log_block_t *current;
spin_lock_t log_lock;
static first_read = 1;

void log_init(void)
{
	init_seeker_cache();
	head = current = seeker_alloc();
	first_read = 1;
}

/* Create, add elements and link. */
log_block_t *log_create(void)
{
	return alloc_seeker();
}

log_link(log_block_t * ent)
{
	lock(&log_lock);
	current->next = ent;
	current = ent;
	unlock(&log_lock);
}


void delete_log(log_block_t *ent)
{
	free_seeker(ent);
}

/* IMP: Make sure writers are done before calling this. */
purge_log(void)
{
	log_block_t *c;
	while(head){
		c = c->next;
		log_delete(head);
		head = c;
	}
	head = NULL;
	current = NULL;
}

int log_read(struct file* file_ptr, char *buf, size_t count, loff_t *offset){
	log_block_t *log;
	int i = 0;
	char *b = buf;
	int bytes_read = 0;
	int exit = 0;

	if(unlikely(head == NULL || buf == NULL || file_ptr == NULL))
		return -1;

	if(unlikely(first_read)){
		if(unlikely(!head->next))
			return -1;
		log = head;
		head = head->next;
		log_delete(log);
		first_read = 0;
	}

		
	/* Read max till current, do not read current 
	 * Trying to read current will make the reader
	 * a competetor with the writers and creates the 
	 * need for a lock.
	 */
	while(i+sizeof(seeker_sampler_entry_t) <= count && !exit && head != current){
		memcpy(buf+i,head->sample,sizeof(seeker_sampler_entry_t));	
		/* Check if this is the last */
		log = head;
		if(log->next == NULL)
			exit = 1;
		head = head->next;
		log_delete(log);
		i+=sizeof(seeker_sampler_entry_t);
	}
	return i;
}

void log_finalize(void)
{
	
