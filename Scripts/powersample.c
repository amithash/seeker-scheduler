#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

int main(int argc,char *argv[])
{
	char touch[200];
	char cat[200];
	pid_t pid;
	if(argc != 2){
		printf("Usage: %s FILE\n",argv[0]);
		exit(1);
	}
	sprintf(touch,"/usr/bin/echo \" \" > %s",argv[1]);
	sprintf(cat,"/usr/bin/ipmitool sensor get /SYS/VPS | /bin/grep -i \"sensor reading\" >> %s",argv[1]);
	system(touch);
	pid = fork();
	if(pid != 0){
		exit(EXIT_SUCCESS);
	} else {
		while(1){
			system(cat);
			usleep(1000000);
		}
	}
	return 0;
}

