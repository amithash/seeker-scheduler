

#ifndef __POWER_H_
#define __POWER_H_

#define POWER_IF_NUM 1


inline int init_ipmi_interface(void);
void receive_message(struct ipmi_recv_msg *msg, void *msg_data);


#endif
