#ifndef __CALLBACK_H_
#define __CALLBACK_H_

typedef void (* switch_to_callback_t)(struct task_struct *,struct task_struct *);

void seeker_set_callback(switch_to_callback_t callback);
void seeker_clear_callback(void);

#endif

