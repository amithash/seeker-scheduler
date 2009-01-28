
#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <seeker.h>

struct debug_block{
	debug_t entry;
	struct debug_block *next;
};

int debug_init(void);
void debug_exit(void);

/* Get a block. User has to check for 
 * !NULL before using the return value 
 * If the return value is not NULL, a 
 * spin lock is held */
struct debug_block *get_debug(void);

/* Release the spin lock held by get debug.
 * If p is NULL, then do nothing */
void put_debug(struct debug_block *p);

void debug_free(struct debug_block *p);

#endif

