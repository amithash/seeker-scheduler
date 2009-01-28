
#ifndef __ASSIGNCPU_H_
#define __ASSIGNCPU_H_

void put_mask_from_stats(struct task_struct *ts);

/* Keep the threshold at 1M Instructions
 * This removes artifcats from IPC and 
 * removes IPC Computation for small tasks
 */
#define INST_THRESHOLD 10000000

#endif
