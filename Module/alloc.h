

struct log_block {
	seeker_sampler_entry_t sample;
	struct log_block *next;
};

int init_seeker_cache(void);
struct log_block * alloc_seeker(void);
void free_seeker(struct log_block * entry);
void finalize_seeker_cache(void);


