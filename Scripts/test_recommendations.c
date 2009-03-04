#include <stdio.h>
#include "../lib/state_recommendations.h"

int main(void)
{
	int i, j, k;
	unsigned long long sum = 0;
	/* Hi cpu bound kernel */
	high_freq();
	for (i = 0; i < 10000000; i++) {
		sum += i;
	}
	continue_dynamically();
	return 0;
}
