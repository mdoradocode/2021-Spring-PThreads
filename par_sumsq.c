
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

typedef struct arguements{
	long num;
	int threadNum;
}arguements;

// function prototypes
void *calculate_square(void* args);
void create_task_queue(struct task *task, FILE *fin);
void initialize();
int next_free_thread();

//Fucntion Definitions
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
	if(fscanf(fin, "%c %ld\n", &action, &num) != 2){
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
void* calculate_square(void* args)
{
	struct arguements *arg =(struct arguements*) args;
	printf("here in calc square begin\n");
	//long number =  (long*) num;
	//calculate the square
	long number = arg->num;
	printf("here in calc square before the_square\n");
	long the_square = number*number;
	//long the_square =(*(long*)number) * (*(long*)number);
	printf("here in calc square before sleep(num)\n");
	sleep(number);

//Everything after this point will be the critical section
	//let's add this to our (global) sum
	printf("here in calc square before mutext lock\n");
	printf("%ld %d\n", arg->num,arg->threadNum);
	pthread_mutex_lock(&cond_mutex);
	sum += the_square;
	//now we also tabulate some (meaningless) statistics
	if (number % 2 == 1){
    		odd++;
  	}
  	if (number < min){
    		min = number;
  	}
  	if (number > max){
    		max = number;
  	}
	thread_array[arg->threadNum].isFree = true;
	pthread_mutex_unlock(&cond_mutex);
	printf("here in calc square after mutex unlock\n");
	return 0;
}


int main(int argc, char* argv[])
{	pthread_t masterThread, thread1, thread2, thread3;
	struct arguements args;
	initialize();
  	// check and parse command line options
  	if (argc != 3) {
    		printf("Usage: sumsq <infile>\n");
    		exit(EXIT_FAILURE);
  	}
  	//Read in text file
  	char *fn = argv[1];
  	//Read in the number of thread
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
	while(1){
		//Next 3 lines "pop" the next instruction from the queue
		action = (*head).action;
		head = (*head).next;
		int freeThread = next_free_thread();
		args.threadNum = freeThread;
		args.num = (*head).number;
		printf("%ld %d\n",args.num, args.threadNum);
		switch(action){
			case 'w':
				printf("here in case w\n");
				sleep(num);
				break;
			case 'p':
				pthread_mutex_lock(&cond_mutex);
				printf("here in case p before while loop\n");
				while(freeThread == 0){
					printf("here in case p while loop before wait\n");
					pthread_cond_wait(&cond_cond, &cond_mutex);
					printf("here in case p while loop after wait\n");
				}
				pthread_mutex_unlock(&cond_mutex);
				printf("here in case p after wait and lock\n");
				pthread_create(thread_array[freeThread].threadName,NULL,&calculate_square,(void*) &args);
		//		pthread_mutex_lock(&cond_mutex);
		//		pthread_cond_signal(&cond_cond);
		//		pthread_mutex_unlock(&cond_mutex);
				printf("here in case p just before break\n");
				break;
			//printf("ERROR: Unrecognized action: '%c'\n", action);
			//exit(EXIT_FAILURE);
		}
		if((*head).next == NULL){
			break;
		}
	}

  	fclose(fin);
	while(thread_array[0].isFree == false &&
		thread_array[1].isFree == false &&
		thread_array[2].isFree == false &&
		thread_array[3].isFree == false){}
  	// print results
  	printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  	// clean up and return
  	return (EXIT_SUCCESS);
}

