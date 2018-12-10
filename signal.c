#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#define MAX_LENGTH 20

int c;
int i;

void _print_symbol()
{
	int k, n, p = 0;
	//Translate c from binary to decimal
	for(k = 0; k < 8; k++) {
		n =  (c % (int)pow(10, k));
		if(k > 0)
			n = n  / (int)pow(10, k - 1);
		p = p + n * ((int) pow(2, k - 1));
			
	}
	c = p;
	printf("%c", p);
}
void rec_handler(int signal)
{
	//printf("rcv, signal recieved \n");
	if(signal == SIGUSR1) {
		c = c + (int)pow(10, i);
	}
}
void snd_handler(int signal)
{
	//printf("snd, signal recieved \n");
}
void receiver()
{	
	//Set mask 
	sigset_t set;
	//Save old mask
	sigset_t old_set;
	//Empty new mask, in case anything is there
	sigemptyset(&set);
	//Add SIGUSR1, SIGUSR2 in new mask
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);
	//Block SIGUSR1 until pause
	sigprocmask(SIG_BLOCK, &set, &old_set);	
	
	//Set reaction to signals of the sender
	struct sigaction act;
	act.sa_handler = &rec_handler;
	act.sa_flags = 0;
	act.sa_mask = old_set;
	if(sigaction(SIGUSR1, &act, NULL) == -1)
		perror("ERR rcv sigaction");
	if(sigaction(SIGUSR2, &act, NULL) == -1)
		perror("ERR rcv sigaction");
		
	while(1) {
		//Set global to 0
		c = 0;
		//Receive_symbol
		for(i = 0; i < 8; i++) {
			//Wait for a bit of information
			sigsuspend(&old_set);
			//Send signal to sender, that receiver is ready for another bit
			kill(getppid(), SIGUSR2);
		}
		
		//Print symbol
		_print_symbol();
		fflush(NULL);
		//Exit if it was the last symbol
		if(c == '\n') {
			return;
		}
	}
}
void sender(int rec_id)
{
	//Set mask 
	sigset_t set;
	//Save old mask
	sigset_t old_set;
	//Empty new mask, in case anything is there
	sigemptyset(&set);
	//Add SIGUSR2 in new mask
	sigaddset(&set, SIGUSR2);
	//Block SIGUSR2 until pause
	sigprocmask(SIG_BLOCK, &set, &old_set);
	
	//Read string from stdout and put it in str
	printf("Enter words\n");
	char* str = calloc(MAX_LENGTH, 1);
	fgets(str, MAX_LENGTH, stdin);
	
	//The number of the symbol in the string
	int j = 0;
	
	//Set reaction to signals of the receiver
	struct sigaction act;
	act.sa_handler = &snd_handler;
	act.sa_flags = 0;
	//Block SIGUSR2 during handler
	act.sa_mask = set;
	
	if(sigaction(SIGUSR2, &act, NULL) == -1)
		perror("ERR rcv sigaction");
	int k;
	while(1) {
		//Set global variable to str[i]
		c = str[i];
		//Send symbol bit by bit 
		for(j = 0; j < 8; j++) {
			k = c >> j;
 
			if (k & 1)
			  kill(rec_id, SIGUSR1);
			else
			  kill(rec_id, SIGUSR2);
			sigsuspend(&old_set);
		}
		if(str[i] == '\n'){
			free(str);
			return;
		}
		i++;
	}
}
int main()
{
	int cpid;
	cpid = fork();
	
	if(cpid == 0) {
		receiver();
		return 0;
	}
	sender(cpid);
	wait(NULL);
	
	return 0;
}
