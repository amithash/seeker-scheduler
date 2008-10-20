
#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <seeker.h>

struct debug_block{
	debug_t entry;
	struct debug_block *next;
};

void debug_init(void);
void debug_exit(void);

struct debug_block *get_debug(void);
void debug_link(struct debug_block *p);
void debug_free(struct debug_block *p);

#endif

