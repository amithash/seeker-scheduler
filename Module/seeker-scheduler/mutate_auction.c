#include <stdio.h>
#include <stdlib.h>

/* Total processors */
#if (0)
#	define NROW 4
#else
#	define NROW NR_CPUS
#endif;

/* Total states */
#if (0)
#	define NCOL 4
#else
#	define NCOL MAX_STATES
#endif

short state_matrix[NROW][NCOL];
short demand[NROW];

short cur_cpu_state[NROW];

void new_layout(int delta)
{

}
