#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

int main(int argc,char *argv[])
{
	char touch[200];
	char cat[200];
	char interval[200];
	pid_t pid;
	if(argc != 2){
		printf("Usage: %s FILE\n",argv[0]);
		exit(1);
	}
	sprintf(touch,"/bin/echo \" \" > %s",argv[1]);
	sprintf(cat,"/usr/bin/ipmitool sensor get /SYS/VPS | /bin/grep -i \"sensor reading\" >> %s",argv[1]);
	sprintf(interval,"/bin/date +\"%%s\" >> %s",argv[1]);

	system(touch);
	pid = fork();
	if(pid != 0){
		exit(EXIT_SUCCESS);
	} else {
		while(1){
			system(interval);
			system(cat);
			usleep(100000);
		}
	}
	return 0;
}

