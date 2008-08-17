#include <linux/slab.h>
#include "alloc.h"
#include "log.h"


static kmem_cache_t * seeker_cachep;

int init_seeker_cache(void)
{
	seeker_cachep = kmem_cache_create("seeker_cache",
					  sizeof(log_block_t),
					  0,
					  SLAB_PANIC,
					  NULL,
					  NULL);
	if(!seeker_cachep)
		return -1;
	return 0;
}

log_block_t * alloc_seeker(void)
{
	log_block_t *ent = 
	(log_block_t *)kmem_cache_alloc(seeker_cachep, GFP_ATOMIC);
	seq_log_init(ent->lock,UNLOCKED);
	return ent;
}

void free_seeker(log_block_t * entry)
{
	kmem_cache_free(seeker_cachep,entry);
}


