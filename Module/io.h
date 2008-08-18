
int generic_open(struct inode *i, struct file *f) {return 0;}
int generic_close(struct inode *i, struct file *f) {return 0;}
int seeker_sample_open(struct inode *in, struct file * f);
int seeker_sample_close(struct inode *in, struct file *f);
ssize_t seeker_sample_log_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset);
