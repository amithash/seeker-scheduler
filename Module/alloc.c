#include <linux/slab.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include "seeker.h"
#include "alloc.h"
#include "log.h"


static struct kmem_cache *seeker_cachep;

int init_seeker_cache(void)
{
	seeker_cachep = kmem_cache_create("seeker_cache",
					  sizeof(struct log_block),
					  0,
					  SLAB_PANIC,
					  NULL);
	if(!seeker_cachep)
		return -1;
	return 0;
}

struct log_block * alloc_seeker(void)
{
	struct log_block *ent = 
	(struct log_block *)kmem_cache_alloc(seeker_cachep, GFP_ATOMIC);
	return ent;
}


void free_seeker(struct log_block * entry)
{
	kmem_cache_free(seeker_cachep,entry);
}

void finalize_seeker_cache(void)
{
	kmem_cache_destroy(seeker_cachep);
}


