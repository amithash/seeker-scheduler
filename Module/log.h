
void log_init(void);
struct log_block *log_create(void);
void log_link(struct log_block * ent);
void delete_log(struct log_block *ent);
void purge_log(void);
int log_read(struct file* file_ptr, char *buf, size_t count, loff_t *offset);
void log_finalize(void);

