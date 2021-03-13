/*..
 * par_sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 */

#include <pthread.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// aggregate variables
volatile long sum = 0;
volatile long odd = 0;
volatile long min = INT_MAX;
volatile long max = INT_MIN;
volatile bool done = false;
pthread_mutex_t cond_mutex;
pthread_cond_t cond_cond;

typedef struct thread{
	bool isFree;
	int threadID;
	pthread_t *threadName;
}thread;
volatile struct thread thread_array[4];

typedef struct task{
	char action;
	long number;
	struct task* next;
}task;

// function prototypes
void calculate_square(long number);
void create_task_queue(struct task *task, FILE *fin);
void initialize();
void add_process(int num, int threadID);//To be implemeted
int next_free_thread();

//Fucntion Definitions
void add_process(int num, int threadID){
	
}

int next_free_thread(){
	if(thread_array[1].isFree == true){
		thread_array[1].isFree = false;
		return 1;
	}
	else if(thread_array[2].isFree == true){
		thread_array[2].isFree = false;
		return 2;
	}
	else if(thread_array[3].isFree == true){
		thread_array[3].isFree = false;
		return 3;
	}
	else{
		return 0;
	}
}

void initialize(){
	pthread_mutex_init(&cond_mutex,NULL);
	pthread_cond_init(&cond_cond,NULL);
}
void create_task_queue(struct task *task, FILE *fin){
	char action;
	long num;
	if((fscanf(fin, "%c %ld\n", &action, &num)) != 2){
		task->next = NULL;
	}
	else{
		struct task* next = malloc(sizeof(task));
		task->next = next;
		next->action = action;
		next->number = num;
		printf("%c\n", next->action);
		printf("%ld\n", next->number);
		create_task_queue(next, fin);
	}
}

//update global aggregate variables given a number
void calculate_square(long number)
{
	//calculate the square
	long the_square = number * number;
	sleep(number);

//Everything after this point will be the critical section
	//let's add this to our (global) sum
	sum += the_square;
	//now we also tabulate some (meaningless) statistics
	if (number % 2 == 1) {
    		odd++;
  	}
  	if (number < min) {
    		min = number;
  	}
  	if (number > max) {
    		max = number;
  	}
}


int main(int argc, char* argv[])
{	pthread_t masterThread, thread1, thread2, thread3;
	initialize();
  	// check and parse command line options
  	if (argc != 3) {
    		printf("Usage: sumsq <infile>\n");
    		exit(EXIT_FAILURE);
  	}
  	//Read in text file
  	char *fn = argv[1];
  	//Read in the number of threads
  	int threadcount;
  	threadcount = atoi(argv[2]);
  	printf("Thread count: %d \n",threadcount);
  	// load numbers and add them to the queue
  	FILE* fin = fopen(fn, "r");
	//Create the intial head of the queue
  	char action;
  	long num;
  	struct task* head = (task*) malloc(sizeof(task));
  	if(fscanf(fin, "%c %ld\n", &action, &num)==2){
  		(*head).action = action;
  		(*head).number = num;
  		(*head).next = NULL;
  		create_task_queue(head, fin);
	}
	//This initializes the array of free threads, couldnt do it in a function
	for(int i=0;i<4;i++){
		thread_array[i].isFree = true;
		thread_array[i].threadID = i;
		if(i == 0){
			thread_array[i].threadName = &masterThread;
		}
		else if(i == 1){
			thread_array[i].threadName = &thread1;
		}
		else if(i == 2){
			thread_array[i].threadName = &thread2;
		}
		else if (i == 3){
			thread_array[i].threadName = &thread3;
		}
	}
	while((*head).next != NULL){
		//Next 3 lines "pop" the next instruction from the queue
		action = (*head).action;
		num = (*head).number;
		head = (*head).next;
		if(action == 'w'){
			sleep(num);
		}
		else if(action == 'p'){
			//int freeThread = next_free_thread();
			//if(freeThread == 0){
				//pthread_mutex_lock(&cond_mutex);
				//while(next_free_thread() == 0){
					//pthread_cond_wait(&cond_cond, &cond_mutex);
				//}

			//}
			//assign_thread();
		}
		else{
			printf("ERROR: Unrecognized action: '%c'\n", action);
			exit(EXIT_FAILURE);
		}
	}

  	fclose(fin);

  	// print results
  	printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  	// clean up and return
  	return (EXIT_SUCCESS);
}

