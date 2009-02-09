#include <stdio.h>
#include <string.h>

unsigned long long int kernel_cpu_bound(unsigned int lim1, 
		unsigned int lim2, 
		unsigned int lim3)
{
	int i,j,k;
	unsigned long long int sum;
	for(i=0;i<lim1;i++){
		for(j=0;j<lim2;j++){
			for(k=0;k<lim3;k++){
				sum += (k*lim1*lim2)+(j*lim2)+i;
				sum--;
			}
		}
	}
	return sum;
}

unsigned long long int kernel_mem_bound(int *array,
		unsigned int lim1, 
		unsigned int lim2, 
		unsigned int lim3)
{
	int i,j,k;
	unsigned long long int sum;
	for(i=0;i<lim1;i++){
		for(j=0;j<lim2;j++){
			for(k=0;k<lim3;k++){
				sum += array[(k*lim1*lim2) + (j*lim2) + i];
			}
		}
	}
	return sum;
}

#define BIG_NUM 1000

int big_array[BIG_NUM * BIG_NUM * BIG_NUM];

int main(int argc, char *argv[])
{
	unsigned long long nll;
	if(argc < 2){
		printf("Invalid parameters, USAGE: %s [mem|cpu]\n",argv[0]);
		return -1;
	}
	if(strcmp(argv[1],"cpu") == 0)
		nll = kernel_cpu_bound(BIG_NUM,BIG_NUM,BIG_NUM);
	else if(strcmp(argv[1],"mem") == 0)
		nll = kernel_mem_bound(big_array,BIG_NUM,BIG_NUM,BIG_NUM);
	else
		printf("Invalid parameters, USAGE: %s [mem|cpu]\n",argv[0]);
	return 0;
	
	return 0;
}

