#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#include <seeker.h>
#include <seeker_cpuidle.h>

MODULE_AUTHOR("Amithash Prasad <amithash@gmail.com>");
MODULE_DESCRIPTION("Tests the scpuidle cpuidle governor");
MODULE_LICENSE("GPL");

static int init_test_scpuidle(void)
{
	int cpus = num_online_cpus();
	enter_seeker_cpuidle();	
	sleep_proc(cpus-1,1);
	return 0;
}

static void exit_test_scpuidle(void)
{
	exit_seeker_cpuidle();
}
module_init(init_test_scpuidle);
module_exit(exit_test_scpuidle);

