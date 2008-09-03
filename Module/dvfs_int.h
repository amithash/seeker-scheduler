
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
#define xstr(s) str(s)
#define str(s) #s

#if defined(ARCH_C2D)
#	define PERF_STATUS 0x00000198
#	define PERF_CTL 0x00000199
#	define PERF_MASK 0x0000FFFF
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define PERF_STATUS 0xC0010042
#	define PERF_CTL 0xC0010041
#	define PERF_MASK 0x0000FFFF
#else
#error "Architecture Not Supported."
#endif


#define eval(i) i

#define MAKE_READ(i) 							\
ssize_t dvfs_read##i(struct file *file_ptr, char __user *buf, 		\
			      size_t count, loff_t *offset){		\
	return dvfs_read_cpu(file_ptr, buf, count,offset, eval(i));	\
}									

#define MAKE_WRITE(i) 							\
ssize_t dvfs_write##i(struct file *file_ptr, const char __user *buf,	\
			      size_t count, loff_t *offset){		\
	return dvfs_write_cpu(file_ptr, buf, count, offset,eval(i));	\
}

#define DECLARE_READ(i) ssize_t dvfs_read##i(struct file *file_ptr, char __user *buf,size_t count, loff_t *offset);

#define DECLARE_WRITE(i) ssize_t dvfs_write##i(struct file *file_ptr, const char __user *buf,size_t count, loff_t *offset);


#define DECLARE_READ_10(i) DECLARE_READ(i##0) DECLARE_READ(i##1) DECLARE_READ(i##2) \
			   DECLARE_READ(i##3) DECLARE_READ(i##4) DECLARE_READ(i##5) \
			   DECLARE_READ(i##6) DECLARE_READ(i##7) DECLARE_READ(i##8) \
			   DECLARE_READ(i##9) 

#define DECLARE_WRITE_10(i) DECLARE_WRITE(i##0) DECLARE_WRITE(i##1) DECLARE_WRITE(i##2) \
			    DECLARE_WRITE(i##3) DECLARE_WRITE(i##4) DECLARE_WRITE(i##5) \
			    DECLARE_WRITE(i##6) DECLARE_WRITE(i##7) DECLARE_WRITE(i##8) \
			    DECLARE_WRITE(i##9) 

#define DECLARE_10(i) DECLARE_READ_10(i) \
		      DECLARE_WRITE_10(i)

#define MAKE_READ_10(i) MAKE_READ(i##0) MAKE_READ(i##1) MAKE_READ(i##2) \
			MAKE_READ(i##3) MAKE_READ(i##4) MAKE_READ(i##5) \
			MAKE_READ(i##6) MAKE_READ(i##7) MAKE_READ(i##8) \
			MAKE_READ(i##9) 

#define MAKE_WRITE_10(i) MAKE_WRITE(i##0) MAKE_WRITE(i##1) MAKE_WRITE(i##2) \
			MAKE_WRITE(i##3) MAKE_WRITE(i##4) MAKE_WRITE(i##5)  \
			MAKE_WRITE(i##6) MAKE_WRITE(i##7) MAKE_WRITE(i##8)  \
			MAKE_WRITE(i##9) 


#define MAKE_10(i) MAKE_READ_10(i) MAKE_WRITE_10(i)
#define MAKE(i) MAKE_READ(i) MAKE_WRITE(i)

#define MAKE_RCB(i) &dvfs_read##i
#define MAKE_WCB(i) &dvfs_write##i

#define MAKE_RCB_10(i)  MAKE_RCB(i##0), MAKE_RCB(i##1), MAKE_RCB(i##2),	\
			MAKE_RCB(i##3), MAKE_RCB(i##4), MAKE_RCB(i##5), \
			MAKE_RCB(i##6), MAKE_RCB(i##7), MAKE_RCB(i##8), \
			MAKE_RCB(i##9)

#define MAKE_WCB_10(i)  MAKE_WCB(i##0), MAKE_WCB(i##1), MAKE_WCB(i##2),	\
			MAKE_WCB(i##3), MAKE_WCB(i##4), MAKE_WCB(i##5), \
			MAKE_WCB(i##6), MAKE_WCB(i##7), MAKE_WCB(i##8), \
			MAKE_WCB(i##9)

ssize_t dvfs_read_cpu(struct file *file_ptr, char __user *buf, 		\
			      size_t count, loff_t *offset, int cpu);	
ssize_t dvfs_write_cpu(struct file *file_ptr, const char __user *buf,		\
			      size_t count, loff_t *offset, int cpu);	

void __pstate_write(void *info);
void __pstate_read(void *info);

int generic_open(struct inode *i, struct file *f);
int generic_close(struct inode *i, struct file *f);

DECLARE_10()
DECLARE_10(1)
DECLARE_10(2)
DECLARE_10(3)
DECLARE_10(4)
