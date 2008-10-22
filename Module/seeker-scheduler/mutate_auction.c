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
	int i,j,k,l;
	int winner=0;
	int winner_val = 0;
	int sum;
	int best_proc = 0;
	int best_proc_val = 0;
	int new_cpu_state[NROW] = {-1};

	/* Create a state matrix such that, the cell which
	 * indicates a processors current state, gets the highest
	 * value = NCOL^2, and parabolically decreases on either side.
	 */
	for(i=0;i<NROW;i++){
		l=0;
		for(j=cur_cpu_state[i];j<NCOL;j++){
			state_matrix[i][j] = (NCOL-l)*(NCOL-l);
			l++;
		}
		l=1;
		for(j=cur_cpu_state[i]-1;j>=0;j--){
			state_matrix[i][j] = (NCOL-l)*(NCOL-l);
			l++;
		}
	}

	/* Now for each delta to spend, hold an auction */
	while(delta > 0){
		winner = 0;
		winner_val = 0;
		best_proc = 0;
		best_proc_val = 0;

		/* There is an optimization here, so do not get confused.
		 * Technically, each column in the state matrix is supposed
		 * to be multiplied by the demand. But that is done here,
		 * as the demand decreases once won.
		 */

		/* For each state, */
		for(i=0;i<NCOL;i++){
			sum = 0;
			/* Do not pointlessly sum N 0's to get 0 
			 * It is going to get rejected anyway */
			if(demand[i] == 0)
				continue;
			/* Sum the cost over all rows */
			for(j=0;j<NROW;j++){
				sum += (state_matrix[j][i] * demand[i]);
			}
			/* if this is the max, make a note of the 
			 * potential winner */
			if(sum > winner_val){
				winner = i;
				winner_val = sum;
			}
		}
		/* A winning val of 0 indicated a failed auction.
		 * all contenstents are broke. Go home loosers.*/
		if(winner_val <= 0)
			break;

		/* Now the winning state, reduces its demand */
		demand[winner]--;
	
		/* The winning state chooses the best deal (Higer state value
		 * but it won it! so might as well get the most expensive one!*/
		for(i=0;i<NROW;i++){
			if(state_matrix[i][winner] > best_proc_val){
				best_proc_val = state_matrix[i][winner];
				best_proc = i;
			}
		}
		/* The best processor is best_proc */

		/* Subtract that from the delta */
		delta -= abs(cur_cpu_state[best_proc] - winner);

		/* Assign the new cpus state to be the winner */
		new_cpu_state[best_proc] = winner;

		/* Set the state_matrix entry to 0 as sold! */
		state_matrix[best_proc][winner] = 0;
		
		/* Continue the auction if delta > 0 */
	}	

	for(i=0;i<NROW;i++){
		if(new_cpu_state[i] == -1)
			new_cpu_state[i] = 0;
		
		if(new_cpu_state[i] != cur_cpu_state[i]){
			cur_cpu_state[i] = new_cpu_state[i];
			set_freq(i,new_cpu_state[i]);
		}
	}
}
