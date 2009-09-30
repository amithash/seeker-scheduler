/******************************************************************************\
 * FILE: mck.c
 * DESCRIPTION: This is the user space implementiation of the algorithm using 
 * dynamic programming to solve the multiple-choice knapsack problem in relation
 * to the mutation usecase scenario.
 * Note that this is useless, but a proof of cocept of the memorization based
 * dynamic programming solution for the multiple-choice knapsack problem 
 * and kept to show the user space implementation.
 * The kernel implemnetation with all its bells and wistles is in
 * Module/seeker-scheduler/mutate_dyn.c
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include <stdio.h>

#define NR_CPUS 4
#define NR_STATES 5
#define MAX_DELTA (NR_CPUS * (NR_STATES - 1))


static int cur_cpu_state[NR_CPUS] = {1,1,0,0};
static int new_cpu_state[NR_CPUS];
static int demand[NR_STATES] = {
	1,1,5,1,1
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
	for(b = 0; b <= w; b++){
		max = -1;
		ind = -1;
		for(j=0;j<m;j++){
			if(wt[0][j] <= b && max < demand[j]){
				max = demand[j];
				ind = j;
			}
		}
		if(b > 0 && dyna[1][b-1].f > max){
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
		for(b = 0; b <= w; b++){
			max = -1;
			ind = -1;
			for(j = 0; j < m; j++){
				demand1 = (b - wt[k-1][j]) >= 0 ? dyna[k-1][b-wt[k-1][j]].f : 0;
				demand2 = demand1 + demand[j];
				if(wt[k-1][j] <= b && demand1 > 0 && max < demand2){
					max = demand2;
					ind = j;
				}
			}
			if(dyna[k][b-1].f > max){
				dyna[k][b].f = dyna[k][b-1].f;
			} else if(max > 0){
				dyna[k][b].f = max;
				dyna[k][b].sol = ind;
			}
		}
	}

	/* backtrack */
	b = w;
	for(k = n; k > 0; k--){
		while(b >= 0){
			if(dyna[k][b].sol >= 0){
				new_cpu_state[k-1] = dyna[k][b].sol;
				b = b - wt[k-1][dyna[k][b].sol];
				break;
			}
			b--;
		}
	}

	for(k=0;k<=n;k++){
		for(b=0;b<=w;b++){
			printf("%d:%d\t",dyna[k][b].f, dyna[k][b].sol);
		}
		printf("\n");
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

