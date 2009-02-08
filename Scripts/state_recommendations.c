#include <stdio.h>
#include <unistd.h>

#include "state_recommendations.h"

#ifdef __LP64__
#define __NR_seeker 295
#else
#define __NR_seeker 333
#endif

int low_freq(void)
{
	return syscall(__NR_seeker, 0, 0);
}

int mid_freq(void)
{
	return syscall(__NR_seeker, 0, 1);
}

int high_freq(void)
{
	return syscall(__NR_seeker, 0, 2);
}

int continue_dynamically(void)
{
	return syscall(__NR_seeker, 1, 0);
}

int ignore_task(void)
{
	return syscall(__NR_seeker, 2, 0);
}
