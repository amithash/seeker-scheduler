#define MAX_INSTRUCTIONS_BEFORE_SCHEDULE 10000000

void update_stats(struct task_struct *t,u64 inst,u64 cy_re, u64 cy_ref);
void init_stats(struct task_struct *t);
void increment_interval(void);

