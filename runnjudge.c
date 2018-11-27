#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

typedef struct msg
{
	long type;
}msg,*pmsg;

int CheckGrammar(char* line)
{
	char* endptr;

	int val = strtol(line, &endptr, 10);

	if(line == endptr || *endptr != '\0') {
		printf("Only numbers are acceptable\n");
		return -1;
	}
	if(val <= 0) {
		printf("Not enough players\n");
		return -1;
	}
	if(val >= 1000) {
		printf("Too many players players\n");
		return -1;
	}

	return val;
}

void judge(int n, int id)
{
	int i;
	msg start;
	msg ready;

	start.type = 1;

	struct timeval time1;
	struct timeval time2;

	for(i = 0; i < n;i++) {				//recive ready status messages from runners
		msgrcv(id, &ready, 0, n + 2, 0);
	}

	printf("start\n");					//start the race
	msgsnd(id, &start, 0, 0);
	gettimeofday(&time1,NULL);				//start the timer
	msgrcv(id, &start, 0, n+1, 0);				//stop the race after recieving msg from the last runner

	gettimeofday(&time2,NULL);				//stop the timer

	printf("time : %ld\n", (-time1.tv_usec + time2.tv_usec) + 1000000*(-time1.tv_sec + time2.tv_sec));		//print the ammount of microseconds it took for all runners to run
}	

void runner(int i, int id, int n)
{
	msg ready_status;
	msg start_running;
	msg stopped_running;

	stopped_running.type = i+1;
	ready_status.type = n+2;

	printf("%d come\n",i);
	msgsnd(id, &ready_status, 0, 0);
	msgrcv(id, &start_running, 0, i, 0);				//recive msg from another 
	printf("%d start\n",i);						//runner to start running
	printf("%d end\n",i);
	msgsnd(id, &stopped_running, 0, 0);				//send msg to another runner or judge,
									// so it could start running or stop the race
}


int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("Number of players is required\n");
		return 0;
	}

	int n = CheckGrammar(argv[1]);					//Check if argv[1] has letters in it 
	if(n <= 0) {
		return 0;
	}

	int i , id;
	int cpid;

	if ((id = msgget(IPC_PRIVATE, 0777|IPC_CREAT)) == -1) {		//Create msg queue for 
		perror("msgget");					//start/stop running messages
	}

	for(i = 0;i < n; i++){						//Creating runners
		cpid = fork();
		if(cpid == 0){						//Runner part
			runner(i + 1, id, n);
			return 0;
		}
	}

	judge(n, id);							//Judge part
	
	while(wait(NULL) != -1){}					//Wait for all runners to print their info in stdout
	
	if ((msgctl(id, IPC_RMID, NULL)) == -1) {			//Destroy msg queues
		perror("msgctl");
	}

	return 0;
}
