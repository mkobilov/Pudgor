#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SHIPWAIT semop(sem_id, &sops[0], 1)
#define DESTROYSHIP semop(sem_id, &sops[1], 1)
#define CREATESHIP semop(sem_id, &sops[2], 1)

#define LOWERLADDER semop(sem_id, &sops[3], 1)
#define RAISELADDER semop(sem_id, &sops[4], 1)

#define PERMISSIONTOLEAVE semop(sem_id, &sops[5], 1)
#define WANTTOLEAVE semop(sem_id, &sops[6],1)

#define GIVETICKET semop(sem_id, &sops[7], 1)
#define CHECKTICKETS semop(sem_id, &sops[8], 1)

#define NOMORETRIPS semop(sem_id, &sops[9], 1)
#define CHECKTRIPS semop(sem_id, &sops[10], 1)
#define STARTTRIPS semop(sem_id, &sops[11], 1)

void S(int a, int sem_id)
{
	struct sembuf sops[2];
	
	sops[0].sem_num = 0;
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;
	
	sops[1].sem_num = 0;
	sops[1].sem_op = -1;
	sops[1].sem_flg = 0;
	
	if(a == 1)
		semop(sem_id, &sops[0], 1);
	if(a == -1)
		semop(sem_id, &sops[1], 1);
	
	return ;
}
void L(int a, int sem_id)
{
	struct sembuf sops[2];
	
	sops[0].sem_num = 1;
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;
	
	sops[1].sem_num = 1;
	sops[1].sem_op = -1;
	sops[1].sem_flg = 0;
	
	if(a == 1)
		semop(sem_id, &sops[0], 1);
	if(a == -1)
		semop(sem_id, &sops[1], 1);
	
	return ;
}
int CheckGrammar(char* line)
{
	char* endptr;
	
	int val = strtol(line, &endptr, 10);
	
	if(line == endptr || *endptr != '\0') {
		printf("Only numbers are acceptable\n");
		return -1;
	}
	if(val <= 0) {
		printf("Not enough passengers or not enough space on ladder or ship\n");
		return -1;
	}
	
	return val;
}
void Ship(int sem_id, struct sembuf sops[12], int t)
{
	int i;
	STARTTRIPS;
	printf("Creating ship\n");

	CREATESHIP;
	
	printf("Ship has arived\n");
	LOWERLADDER;
	printf("Lower1 the ladder\n");
	
	for(i = 0; i < t; i++){
		SHIPWAIT;
		CHECKTICKETS;			//Tickets is the resource that indicates if all the passangers on
		printf("Everything is ok\n");	//the ship want to trip, so the ship will not go on a second trip 
						//with the same passengers
		RAISELADDER;
		printf("Raise the ladder\n");
		
		printf("Great sea trip\n");
		PERMISSIONTOLEAVE;
		
		LOWERLADDER;
		printf("Lower2 the ladder\n");
		
	}
	
	printf("Close the gates\n");
	NOMORETRIPS;
	
	printf("Check the ship\n");
	DESTROYSHIP;
	
	printf("Check the ladder\n");
	RAISELADDER;
	
	printf("Enough for now\n");
}
int CheckTrips(int sem_id, struct sembuf sops[12])
{
	struct timespec	timeout;
	timeout.tv_nsec = 1;		//The amount of time passenger shoud wait
	timeout.tv_sec = 0;
	
	if(semtimedop(sem_id, &sops[12], 1, &timeout) == -1)
		return 1;
	return 0;
}
void Passenger(int sem_id, struct sembuf sops[12])
{
	while(1) {
		printf("%d I'm on the ground\n",getpid());
		//Check if there will be any trips
		if(CHECKTRIPS == 0)
			return ;
		//Gets on the ship
		S(-1, sem_id);
		L(-1, sem_id);
		printf("%d I'm on the ladder\n",getpid());
		printf("%d I'm on the ship\n",getpid());
		
		//if there will be no trips passenger walks on the ground
		if(CHECKTRIPS == 0) {	
			L(1, sem_id);
			S(1, sem_id);
			return ;
		}
		L(1, sem_id);
		
		printf("%d I'm giving ticket to ship\n",getpid());		
		GIVETICKET;
		
		WANTTOLEAVE;
		printf("%d I want to leave the ship\n",getpid());
		//Leaves the ship
		L(-1, sem_id);
		printf("%d I'm on the ladder\n",getpid());
		S(1, sem_id);
		printf("%d I've left the ship\n",getpid());
		L(1, sem_id);
		printf("%d I'm on the ground\n",getpid());
	}
}

int main(int argc, char* argv[])
{
	if(argv[1] == NULL){
		printf("Number of passengers ship can handle is required\n");
		return 0;
	}
	if(argv[2] == NULL){
		printf("Number of passengers ladder can handle is required\n");
		return 0;
	}
	if(argv[3] == NULL){
		printf("Number of passengers is required\n");
		return 0;
	}
	if(argv[4] == NULL){
		printf("Number of trips is required\n");
		return 0;
	}
	
	int i, cpid;
	int sem_id;
	int s = CheckGrammar(argv[1]);		//Number of passengers ship can handle
	int l = CheckGrammar(argv[2]);		//Number of passengers ladder can handle
	int p = CheckGrammar(argv[3]);		//Number of passengers 
	int t = CheckGrammar(argv[4]);		//Number of trips 
	
	if(s <= 0 || p<=0 || l<=0) {
		return 0;
	}
	
	if(s > p)
		s = p;
	
	struct sembuf sops[12];		//Operations with semaphores

	sops[0].sem_num = 0;
	sops[0].sem_op = 0;
	sops[0].sem_flg = 0;

	sops[1].sem_num = 0;		//Remove places on a ship
	sops[1].sem_op = -s;
	sops[1].sem_flg = 0;
	
	sops[2].sem_num = 0;		//Create s places on a ship
	sops[2].sem_op = s;
	sops[2].sem_flg = 0;
	
	sops[3].sem_num = 1;		//Create l places on a ladder
	sops[3].sem_op = l;
	sops[3].sem_flg = 0;
	
	sops[4].sem_num = 1;		//Remove ladder
	sops[4].sem_op = -l;
	sops[4].sem_flg = 0;
	
	sops[5].sem_num = 2;		//Give permissions to leave
	sops[5].sem_op = s;
	sops[5].sem_flg = 0;
	
	sops[6].sem_num = 2;		//Take a permission to leave
	sops[6].sem_op = -1;
	sops[6].sem_flg = 0;
	
	sops[7].sem_num = 3;		//Give ticket to ship
	sops[7].sem_op = 1;
	sops[7].sem_flg = 0;
	
	sops[8].sem_num = 3;		//Check tickets
	sops[8].sem_op = -s;
	sops[8].sem_flg = 0;
	
	sops[9].sem_num = 4;		//No more trips today
	sops[9].sem_op = -1;
	sops[9].sem_flg = 0;
		 
	sops[10].sem_num = 4;		//Check if there will be 
	sops[10].sem_op = 0;		//any trips today
	sops[10].sem_flg = IPC_NOWAIT;
	
	sops[11].sem_num = 4;					
	sops[11].sem_op = 1;
	sops[11].sem_flg = 0;
	
	if ((sem_id = semget(IPC_PRIVATE, 5, 0777|IPC_CREAT)) == -1) {
		fprintf(stdout, "%s\n", strerror(errno));
		return 0;
	}

	for(i = 0;i < p; i++){		//Creating passengers
		cpid = fork();
		if(cpid == 0) {
			Passenger(sem_id, sops);
			printf("%d I'm leaving now\n",getpid());
			return 0;
		}
	}

	Ship(sem_id, sops, t);
	
	while(wait(NULL) != -1){}
	
	semctl(sem_id, 5, IPC_RMID);
	
	while(wait(0) != -1){}
	
	return 0;
}
