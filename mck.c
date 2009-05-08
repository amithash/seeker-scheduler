#include <stdio.h>

/* This is the implementiation of the algorithm using dynamic programming
 * to solve the multiple-choice knapsack problem in relation to the 
 * mutation usecase scenario. */

/* this might be merged into Module/seeker-scheduler/mutator.c 
 * or merged into a simulator based on some pseudo analysis on 
 * which will be better. 
 * My current understanding gives me a 90% condidance that the method
 * employed in the current mutator is better. But we always need
 * a proof right? Experimental or analytical
 */

#define NR_CPUS 4
#define NR_STATES 5
#define MAX_DELTA (NR_CPUS * (NR_STATES - 1))


static int cur_cpu_state[NR_CPUS] = {0,1,2,3};
static int new_cpu_state[NR_CPUS];
static int demand[NR_STATES] = {
	1,2,4,1,1
};

#define ABS(n) ((n) > 0 ? (n) : (-1 * (n)))

/* Implementation of the dynamic programming solution
 * for the multiple knap sack problem whose algorithm
 * is provided in:
 *
 * Multiple Choice Knapsack Functions
 * James C. Bean
 * Department of Industrial and Operations Engineering
 * The University of Michigan
 * Ann, Arbor, MI 48109-2117
 * January 4 1988
 *
 * Number of classes = n
 * Differences:
 * 1. All classes have equal number of elements = m;
 * 2. The value of element x_ij (jth element in class i)
 * has a value val_j => the value of all elements 
 * x_ij ( 1 <= i <= n ) are equal. 
 */
mck(int n, int m, int w)
{
	int i;
	int j;
	int b;
	int k;
	int max;
	int ind;
	int demand1, demand2;
	struct struct_f {
		int f;
		int sol;
	};
	static struct struct_f dyna[NR_CPUS+1][MAX_DELTA+1];
	static int wt[NR_CPUS][NR_STATES];

	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			wt[i][j] = ABS(j - cur_cpu_state[i]);
		}
	}

	for(i=0;i<=n;i++){
		for(j = 0; j <= w; j++){
			dyna[i][j].f = 0;
			dyna[i][j].sol = -1;
		}
	}

	/* init first proc */
	for(b = 1; b <= w; b++){
		max = -1;
		ind = -1;
		for(j=0;j<m;j++){
			if(wt[0][j] <= b && max < demand[j]){
				max = demand[j];
				ind = j;
			}
		}
		if(dyna[1][b-1].f > max){
			dyna[1][b].f = dyna[1][b-1].f;
		} else {
			dyna[1][b].f = max;
		}
		if(max > 0){
			dyna[1][b].sol = ind;
		}
	}

	/* do for all! */
	for(k = 2; k <= n; k++){
		for(b = 1; b <= w; b++){
			max = -1;
			ind = -1;
			for(j = 0; j < m; j++){
				demand1 = (b - wt[k-1][j]) > 0 ? dyna[k-1][b-wt[k-1][j]].f : 0;
				demand2 = demand1 + demand[j];
				if(wt[k-1][j] <= b && demand1 > 0 && max < demand2){
					max = demand2;
					ind = j;
				}
			}
			if(dyna[k][b-1].f > max){
				dyna[k][b].f = dyna[k][b-1].f;
			} else {
				dyna[k][b].f = max;
				dyna[k][b].sol = ind;
			}
		}
	}

	/* backtrack */
	b = w;
	for(k = n; k > 0; k--){
		while(b > 0){
			if(dyna[k][b].sol >= 0){
				new_cpu_state[k-1] = dyna[k][b].sol;
				b = b - wt[k-1][dyna[k][b].sol];
				break;
			}
			b--;
		}
	}
}

int main(void)
{
	int i,j;
	int n = 4;
	int m = 5;
	int delta = 2;
	int w = delta;
	mck(n,m,w);
	printf("Old Layout: ");
	for(i=0;i<n;i++){
		printf("%d ",cur_cpu_state[i]);
	}
	printf("\n State Demand: ");
	for(j = 0; j < m; j++){
		printf("%d ",demand[j] - 1);
	}
	printf("\nNew Layout: ");
	for(i=0;i<n;i++){
		cur_cpu_state[i] = new_cpu_state[i];
		printf("%d ",new_cpu_state[i]);
	}
	printf("\n");
	return 0;
}

