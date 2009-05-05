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

/* the dynamic programming results */
static int f[NR_CPUS+1][MAX_DELTA+1];

/* the solution index. */
static int sol[NR_CPUS+1][MAX_DELTA+1];

/* the selection. */
static int select[NR_CPUS][NR_STATES];

static int old_layout[NR_CPUS] = {0,1,2,3};
static int new_layout[NR_CPUS];

static int wt[NR_CPUS][NR_STATES];

static int val[NR_STATES] = {
	1,2,4,1,1
};

#define ABS(n) ((n) > 0 ? (n) : (-1 * (n)))

void make_weights(int n,int m){
	int i,j;
	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			wt[i][j] = ABS(j - old_layout[i]);
		}
	}
}

mck(int n, int m, int w)
{
	int i;
	int j;
	int b;
	int k;
	int max;
	int ind;
	int val1, val2;
	for(j = 0; j <=w; j++){
		f[0][j] = 0;
	}
	for(i = 0; i <= n; i++){
		f[i][0] = 0;
	}
	for(i=0;i<=n;i++){
		for(j = 0; j <= w; j++){
			sol[i][j] = 0;
		}
	}
	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			select[i][j] = 0;
		}
	}

	/* init first proc */
	for(b = 1; b <= w; b++){
		max = -1;
		ind = -1;
		for(j=0;j<m;j++){
			if(wt[0][j] <= b && max < val[j]){
				max = val[j];
				ind = j;
			}
			if(f[1][b-1] > max){
				f[1][b] = f[1][b-1];
			} else {
				f[1][b] = max;
			}
			if(max > 0){
				sol[1][b] = ind+1;
			}
		}	
	}

	/* do for all! */
	for(k = 2; k <= n; k++){
		for(b = 1; b <= w; b++){
			max = -1;
			ind = -1;
			for(j = 0; j < m; j++){
				val1 = (b - wt[k-1][j]) > 0 ? f[k-1][b-wt[k-1][j]] : 0;
				val2 = val1 + val[j];
				if(wt[k-1][j] <= b && val1 > 0 && max < val2){
					max = val2;
					ind = j;
				}
			}
			if(f[k][b-1] > max){
				f[k][b] = f[k][b-1];
			} else {
				f[k][b] = max;
				sol[k][b] = ind+1;
			}
		}
	}

	/* backtrack */
	b = w;
	for(k = n; k > 0; k--){
		while(b > 0){
			if(sol[k][b] > 0){
				new_layout[k-1] = sol[k][b] - 1;
				b = b - wt[k-1][sol[k][b]-1];
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
	int delta = 3;
	int w = delta;
	make_weights(n,m);
	mck(n,m,w);
	printf("Old Layout: ");
	for(i=0;i<n;i++){
		printf("%d ",old_layout[i]);
	}
	printf("\n State Demand: ");
	for(j = 0; j < m; j++){
		printf("%d ",val[j] - 1);
	}
	printf("\nNew Layout: ");
	for(i=0;i<n;i++){
		printf("%d ",new_layout[i]);
	}
	printf("\n");
	return 0;
}

