#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_LEN 64

struct _monitor
{
	//pizza is 4 symbol string, each symbol is an ingridient
	char pizza[4];
	int number_of_pizza;
	
	//all pizzas will be sent in stack, that will be later checked
	char need_to_check[STACK_LEN][4];
	int num_to_check;
	
	//semaphores, so multiple simultanious actions with pizza is impossible
	pthread_mutex_t busy;
	pthread_cond_t pizza_is_ready;
	
	//put ingridient on pizza
	void (*put_ingr)(struct _monitor* monitor, char ingr);
	//check symbols of pizza
	int (*check_pizza)(struct _monitor* monitor);
	//send pizza to check queue
	void (*snd_pizza_to_check)(struct _monitor* monitor);
};


int check_grammar(char* line)
{
	char* endptr;
	
	int val = strtol(line, &endptr, 10);
	
	if(line == endptr || *endptr != '\0') {
		printf("Only numbers are acceptable\n");
		return -1;
	}
	if(val <= 0) {
		printf("Incorrect pizza number\n");
		return -1;
	}
	
	return val;
}
void put_ingr(struct _monitor* monitor, char ingr)
{
	pthread_mutex_lock(&(monitor->busy));
	int i;
	//choosing correct place for ingridient
	for(i = 0 ;monitor->pizza[i] != 0; i++){}		 
											
	monitor->pizza[i] = ingr;
	//if final ingridient was put, send pizza to check
	if(i == 2) {	
		monitor->snd_pizza_to_check(monitor);
		pthread_cond_broadcast(&(monitor->pizza_is_ready));
	}
	else
		pthread_cond_wait(&(monitor->pizza_is_ready), &(monitor->busy));
	
	pthread_mutex_unlock(&(monitor->busy));
}
int check_pizza(struct _monitor* monitor)
{
	pthread_mutex_lock(&(monitor->busy));
	
	int res = 0;
	//Waiting if there are no pizzas to check
	if(monitor->num_to_check == 0)
		pthread_cond_wait(&(monitor->pizza_is_ready), &(monitor->busy));
	//Printing ingridients in stdout(valgrind is ok here)
	printf("%c %c %c\n",monitor->need_to_check[monitor->num_to_check][0],
						monitor->need_to_check[monitor->num_to_check][1],
						monitor->need_to_check[monitor->num_to_check][2]);
	
	//Checking if all ingridients are there(valgrind is angry here!!)
	if((monitor->need_to_check[monitor->num_to_check][0]+
		monitor->need_to_check[monitor->num_to_check][1]+
		monitor->need_to_check[monitor->num_to_check][2]+
		monitor->need_to_check[monitor->num_to_check][3]) == ('c' + 'c' + 'p'))
		res++;
	
	monitor->num_to_check--;
	
	pthread_mutex_unlock(&(monitor->busy));
	return res;
}
void snd_pizza_to_check(struct _monitor* monitor)
{
	monitor->num_to_check++;
	int i = 0;
	for(i = 0; i < 3; i++)
	{
		monitor->need_to_check[monitor->num_to_check][i] = monitor->pizza[i];
		monitor->pizza[i] = 0;
	}
}
void monitor_create(struct _monitor* monitor, int number_of_pizza)
{
	monitor->number_of_pizza = number_of_pizza;
	monitor->num_to_check = 0;
	
	monitor->put_ingr = &put_ingr;
	monitor->check_pizza = &check_pizza;
	monitor->snd_pizza_to_check = &snd_pizza_to_check;
	
	pthread_mutex_init(&(monitor->busy), NULL);
	pthread_cond_init(&(monitor->pizza_is_ready), NULL);
	
	int i = 0;
	for(i = 0 ; i < 3 ; i++)
		monitor->pizza[i] = 0;
}
void* pineappler(void* p)
{
	struct _monitor* monitor = p;
	
	int i = 0;
	for(i = 0; i < monitor->number_of_pizza; i++)
		monitor->put_ingr(monitor, 'p');
		
	return NULL;
}

void* cheeser(void* p)
{
	struct _monitor* monitor = p;
	
	int i = 0;
	for(i = 0; i < monitor->number_of_pizza; i++)
		monitor->put_ingr(monitor, 'c');
	
	return NULL;
}

void* checker(void* p)
{
	struct _monitor* monitor = p;
	
	int i = 0, res = 0;
	for(i = 0; i < monitor->number_of_pizza; i++)
		res += monitor->check_pizza(monitor);
	
	if(res != 0) {
		printf("Good pizza\n");
	}
	else {
		printf("Bad pizza\n");
	}
	return NULL;
}

int main(int argc, char** argv)
{
	if(argc < 2) {
		printf("Number of pizzas is required\n");
		return 0;
	}
	
	struct _monitor monitor;
	
	int number_of_pizza = check_grammar(argv[1]);
	
	monitor_create(&monitor, number_of_pizza);
	
	pthread_t id_pineapple, id_cheese_1, id_cheese_2, id_checker;
	
	pthread_mutex_init(&(monitor.busy), NULL);
	
	pthread_create(&id_pineapple, NULL, &pineappler, &monitor);    
	pthread_create(&id_cheese_1, NULL, &cheeser, &monitor);
	pthread_create(&id_cheese_2, NULL, &cheeser, &monitor);
	pthread_create(&id_checker, NULL, &checker, &monitor);
	
	pthread_join(id_checker, NULL);
	pthread_join(id_pineapple, NULL);
	pthread_join(id_cheese_1, NULL);
	pthread_join(id_cheese_2, NULL);
	
	return 0;
}
