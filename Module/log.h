#include "seeker.h"
#include "alloc.h"

rw_lock log_lock;

typedef struct _log_block_t {
	seeker_sampler_entry_t sample;
	log_block_t *next;
} log_block_t;


