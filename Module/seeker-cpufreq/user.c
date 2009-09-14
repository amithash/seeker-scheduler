#include <linux/kernel.h>
#include <linux/module.h>

#include <seeker.h>
#include <seeker_cpufreq.h>

/********************************************************************************
 * 				Global Prototyprs 				*
 ********************************************************************************/

struct scpufreq_user_list{
	struct scpufreq_user *user;
	struct scpufreq_user_list *next;
};

/********************************************************************************
 * 				Global Datastructures 				*
 ********************************************************************************/

/* List of registered users */
struct scpufreq_user_list *user_head = NULL;

/* Lock to be held whenever accessing user_head. */
static DEFINE_SPINLOCK(user_lock);

/* The counter to give a unique un-used id 4B ids can be provided.*/
static unsigned int current_highest_id = 0;

/*******************************************************************************
 * create_user - creates a new user in the user list.
 * @u - The user struct.
 * @return - 0 on success, error code on failure
 *
 * Create a new user node and link into the users list. 
 *******************************************************************************/
static int create_user(struct scpufreq_user *u)
{
	struct scpufreq_user_list *i;
	if(!u)
		return ERR_INV_USER;
	if(!u->inform)
		return ERR_INV_CALLBACK;
	spin_lock(&user_lock);
	if(user_head){
		for(i=user_head;i->next;i=i->next);
		i->next = kmalloc(sizeof(struct scpufreq_user_list), GFP_KERNEL);
		i = i->next;
	} else {
		i = user_head = kmalloc(sizeof(struct scpufreq_user_list), GFP_KERNEL);
	}
	if(!i){
		spin_unlock(&user_lock);
		return ERR_USER_MEM_LOW;
	}
	i->next = NULL;
	i->user = u;
	i->user->user_id = current_highest_id++;
	spin_unlock(&user_lock);
	return 0;
}

/*******************************************************************************
 * destroy_user - Removes and deallocated a user from the users list.
 * @u - The user struct
 * @return - 0 on success, error code on failure.
 *
 * Search for a user in the list with id = u->user_id and if found, unlink 
 * from the list and de-allocate the memory.
 *******************************************************************************/
static int destroy_user(struct scpufreq_user *u)
{
	struct scpufreq_user_list *i,*j;
	if(!u)
		return ERR_INV_USER;
	if(!u->inform)
		return ERR_INV_CALLBACK;
	spin_lock(&user_lock);

	for(i=user_head,j=NULL;i && i->user->user_id != u->user_id ;j=i,i=i->next);

	if(!i){
		spin_unlock(&user_lock);
		return ERR_INV_USER;
	}
	if(!j)
		user_head = i->next;
	else
		j->next = i->next;
	spin_unlock(&user_lock);
	kfree(i);
	return 0;
}


/*******************************************************************************
 * inform_freq_change - Infrom all registered users of a freq change.
 * @cpu - The cpu for which freq is changing.
 * @state - The new state cpu will take.
 *
 * Call the inform callback for every registered users. 
 *******************************************************************************/
void inform_freq_change(int cpu, int state)
{
	struct scpufreq_user_list *i;
	int dummy_ret = 0;

	for(i=user_head;i;i=i->next){
		if(i->user->inform)
			dummy_ret |= i->user->inform(cpu,state);
	}
	if(dummy_ret){
		warn("Some of the users returned an error code which was lost");
	}
}

/*******************************************************************************
 * register_scpufreq_user - Register a new user of scpufreq.
 * @u - The user struct for the calling user.
 * 
 * Register user u with scpufreq. All notifications will be sent. 
 *******************************************************************************/
int register_scpufreq_user(struct scpufreq_user *u)
{
	return create_user(u);
}
EXPORT_SYMBOL_GPL(register_scpufreq_user);

/*******************************************************************************
 * deregister_scpufreq_user - Un-register a new user of scpufreq.
 * @u - The user struct for the calling user.
 * 
 * Un-register user u with scpufreq. All notifications will no longer be sent. 
 *******************************************************************************/
int deregister_scpufreq_user(struct scpufreq_user *u)
{
	return destroy_user(u);
}
EXPORT_SYMBOL_GPL(deregister_scpufreq_user);

void init_user_interface(void)
{
	user_head = NULL;
	spin_lock_init(&user_lock);
}

void exit_user_interface(void)
{
	struct scpufreq_user_list *i,*j;
	spin_lock(&user_lock);
	for(i=user_head;i;){
		j = i->next;
		kfree(i);
		i = j;
	}
	user_head = NULL;
	spin_unlock(&user_lock);
}


