#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef MEM_SIZE 
#warning "MEM_SIZE was not specified assuming a conservative 1GB"
#define MEM_SIZE 1000000
#endif

#define MEM_SIZE_INT (MEM_SIZE * 1024) / sizeof(int)

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
			

int main(int argc, char *argv[])
{
	unsigned long long nll;
	unsigned int *big_array;
	struct timeval start,end;
	int sqrt_mem_size;
	if(argc < 2){
		printf("Invalid parameters, USAGE: %s [mem|cpu]\n",argv[0]);
		return 0;
	}
	/* We want to use 3/4 of the mem avaliable */
	sqrt_mem_size = (int)sqrt((MEM_SIZE_INT * 3) / 4);
	big_array = (int *)malloc(sizeof(int) * sqrt_mem_size * sqrt_mem_size);
	gettimeofday(&start,NULL);
	if(strcmp(argv[1],"cpu") == 0)
		nll = kernel_cpu_bound(sqrt_mem_size, sqrt_mem_size);
	else if(strcmp(argv[1],"mem") == 0)
		nll = kernel_mem_bound(big_array,sqrt_mem_size,sqrt_mem_size);
	else
		printf("Invalid parameters, USAGE: %s [mem|cpu]\n",argv[0]);
	gettimeofday(&end,NULL);
	printf("%d",in_sec(start,end));
	return 0;
}


