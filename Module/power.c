#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ipmi.h>

#include "seeker.h"
#include "power.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amithash Prasad (amithash.prasad@colorado.edu)");
MODULE_DESCRIPTION("Module to sample the current power consumption using the IPMI Interface");

struct ipmi_user_hndl user_handle;
ipmi_user_t user;
struct ipmi_addr ipmi_address;

void receive_message(struct ipmi_recv_msg *msg, void *msg_data)
{
	debug("type=%d,msgid=%d",msg->recv_type,msg->msgid);
	ipmi_free_recv_msg(msg);
}

inline int init_ipmi_interface(void)
{
	user_handle.ipmi_recv_hndl = &receive_message;
	user_handle.ipmi_watchdog_pretimeout = NULL;
	ipmi_create_user(POWER_IF_NUM, &user_handle, NULL, &user);
	return 0;

}

void get_power(void)
{
//	ipmi_request();
}

static int __init init_power(void)
{
	if(init_ipmi_interface()){
		error("Could not initialize the IPMI Interface");
		return -1;
	}

	return 0;
}

static void __exit exit_power(void)
{
	ipmi_destroy_user(user);
}

module_init(init_power);
module_exit(exit_power);

