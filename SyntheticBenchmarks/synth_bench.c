/*****************************************************
 * Copyright 2009 Amithash Prasad                    *
 *                                                   *
 * This file is part of Seeker                       *
 *                                                   *
 * Seeker is free software: you can redistribute     *
 * it and/or modify it under the terms of the        *
 * GNU General Public License as published by        *
 * the Free Software Foundation, either version      *
 * 3 of the License, or (at your option) any         *
 * later version.                                    *
 *                                                   *
 * This program is distributed in the hope that      *
 * it will be useful, but WITHOUT ANY WARRANTY;      *
 * without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE. See the GNU General Public License       *
 * for more details.                                 *
 *                                                   *
 * You should have received a copy of the GNU        *
 * General Public License along with this program.   *
 * If not, see <http://www.gnu.org/licenses/>.       *
 *****************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEFAULT_CACHE_SIZE 1024

#ifndef CACHE_SIZE
#warning "CACHE_SIZE was not specified assuming a conservative 1MB"
#define CACHE_SIZE DEFAULT_CACHE_SIZE
#endif

#if CACHE_SIZE <= 0
#undef CACHE_SIZE
#warning "CACHE_SIZE was invalid using a conservative 1MB"
#define CACHE_SIZE DEFAULT_CACHE_SIZE
#endif

#define CACHE_SIZE_INT ((CACHE_SIZE * 1024) / sizeof(int))

/* kernel_cpu_bound - stress the cpu
 * @trials   = total times the inner kernel is executed.
 * @sqrt_lim = The square root of the limit of the array size
 * 	       Makes no sence in this context, as there is no
 * 	       array, but used to make the total instructions
 * 	       in the inner kernel the same as the mem variant.
 * @return   = The value of computation. Usually ignored.
 *
 * Stress the CPU of the system performing computation.
 */
unsigned long long int kernel_cpu_bound(
		unsigned int trials, 
		unsigned int sqrt_lim)
{
	int i,j,k;
	unsigned long long int sum;
	for(i=0;i<trials;i++){
		for(j=0;j<sqrt_lim;j++){
			for(k=0;k<sqrt_lim;k++){
				sum += i + (k*sqrt_lim)+j;
			}
		}
	}
	return sum;
}

/* kernel_mem_bound - stress the memory subsystem
 * @trials = total times the inner kernel is executed.
 * @lim2   = The square root of the limit of the array size
 * @return = The value of computation. Usually ignored.
 *
 * Stress the CPU of the system performing computation.
 */
unsigned long long int kernel_mem_bound(
		int *array,
		unsigned int trials, 
		unsigned int sqrt_lim)
{
	int i,j,k;
	unsigned long long int sum;
	for(i=0;i<trials;i++){
		for(j=0;j<sqrt_lim;j++){
			for(k=0;k<sqrt_lim;k++){
				/* This is done the fortran way,
				 * to confuse the pre-fetcher
				 */
				sum += array[(k*sqrt_lim) + j];
			}
		}
	}
	return sum;
}

#define in_sec(start,end) (((double)end.tv_sec + ((double)end.tv_usec/1000000.0)) - \
			((double)start.tv_sec + ((double)start.tv_usec/1000000.0)))
			
#define CACHES 8
#define DEFAULT_TRIALS 100

int main(int argc, char *argv[])
{
	unsigned long long nll;
	unsigned int *big_array;
	struct timeval start,end;
	unsigned int sqrt_mem_size;
	int trials;
	if(argc < 2){
		printf("Invalid parameters, USAGE: %s [mem|cpu] [trials, default=100]\n",argv[0]);
		return 0;
	}
	if(argc == 3){
		trials = atoi(argv[2]);
		if(trials < 1){
			printf("Invalid number of trials = %d\n",trials);
			return -1;
		}
	} else {
		trials = DEFAULT_TRIALS;
	}
	/* We want to use CACHES times the cache size */
	sqrt_mem_size = (unsigned int)sqrt(sizeof(int) * (CACHE_SIZE_INT * CACHES));
	big_array = (int *)malloc(sizeof(int) * sqrt_mem_size * sqrt_mem_size);
	gettimeofday(&start,NULL);
	if(strcmp(argv[1],"cpu") == 0)
		nll = kernel_cpu_bound(trials, sqrt_mem_size);
	else if(strcmp(argv[1],"mem") == 0)
		nll = kernel_mem_bound(big_array,trials,sqrt_mem_size);
	else
		printf("Invalid parameters, USAGE: %s [mem|cpu]\n",argv[0]);
	gettimeofday(&end,NULL);
	printf("%f\n",in_sec(start,end));
	return 0;
}


