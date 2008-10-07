
#define HASH_SIZE 256
#define HASH_MASK 0xff /* First 8 bits */
struct task_stats{
	int pid;
	u64 interval;
	u64 inst;
	u64 re_cy;
	u64 ref_cy;
	struct task_stats *next;
	struct task_stats *prev;
};

struct task_stats *ts_hash[HASH_SIZE] = {NULL};

/* Gets pointer to task stats. 
 * Creates it if it does not exist 
 */
struct task_stats *get_task_stats(pid_t p)
{
	unsigned int pid = (unsigned int)p;
	unsigned int ind = pid | HASH_MASK;
	struct task_stats *ts;
	if(ts_hash[ind] == NULL){
		/* Use kmem_cache */
		ts_hash[ind] = (struct task_stats *)kalloc(sizeof(struct task_stats),GFP_KERNEL);
		ts = ts_hash[ind];
		ts->next = NULL;
		ts->prev = NULL;
		ts->pid = pid;
	} else {
		ts = ts_hash[ind];
		while(ts->next && ts->pid != pid)
			ts = ts->next;
		if(ts->pid != pid){
			ts->next = (struct task_stats *)kalloc(sizeof(struct task_stats),GFP_KERNEL);
			ts->next->prev = ts;
			ts->next->next = NULL;
			ts = ts->next;
			ts->pid = pid;
		}
	}
	return ts;
}

void destroy_task_stats_pid(pid_t p)
{
	unsigned int pid = (unsigned int) p;
	unsigned int ind = pid | HASH_MASK;
	struct task_stats *prev = NULL;
	struct task_stats *cur;
	/* not found */
	if(unlikely(ts_hash[ind] == NULL))
		return;

	cur = ts_hash[ind];
	while(cur->next && cur->pid != pid){
		prev = cur;
		cur = cur->next;
	}
	/* Not found */
	if(cur->pid != pid)
		return;

	prev->next = cur->next;
	kfree(cur);
}

void destroy_task_stats(struct task_stats *ts)
{
	unsigned int pid = ts->pid;
	unsigned int ind = pid | HASH_MASK;
	/* Check to keep the hash sane */
	if(ts_hash[ind]->pid != pid){
		ts->prev->next = ts->next;
		ts->next->prev = ts->prev;
		kfree(ts);
	} else {
		ts_hash[ind] = ts->next;
		kfree(ts);
	}
}


