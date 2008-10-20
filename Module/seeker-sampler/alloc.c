
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
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <seeker.h>

#include "alloc.h"
#include "log.h"


static struct kmem_cache *seeker_cachep = NULL;

int init_seeker_cache(void)
{
	seeker_cachep = kmem_cache_create("seeker_cache",
					  sizeof(struct log_block),
					  0,
					  SLAB_PANIC | SLAB_CACHE_DMA,
					  NULL);
	if(!seeker_cachep)
		return -1;
	return 0;
}

struct log_block * alloc_seeker(void)
{
	struct log_block *ent = 
	(struct log_block *)kmem_cache_alloc(seeker_cachep, GFP_ATOMIC | GFP_DMA);
	if(!ent){
		warn("Allocation failed");
		return NULL;
	}
	return ent;
}


void free_seeker(struct log_block * entry)
{
	kmem_cache_free(seeker_cachep,entry);
	entry = NULL;
}

void finalize_seeker_cache(void)
{
	kmem_cache_destroy(seeker_cachep);
}


