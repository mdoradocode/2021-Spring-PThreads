/*..
 * par_sumsq.c
 *
 * CS 446.646 Project 5 (Pthreads)
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

typedef struct threadL{
	bool isFree;
	struct threadL* next;
	int id;
}threadL;
volatile struct threadL threadArray[10001];

//The struct of each task that is read from the test file
typedef struct task{
	char action;
	long number;
	struct task* next;
}task;

//This was used to get mutiple arguements into the pthreads function
typedef struct arguements{
	long num;
	int threadNum;
	struct threadL* threadLHead;
}arguements;

// function prototypes
//Main work to be done function
void *calculate_square(void* args);

//Creates a singly linked list of tasks to be taken from
void create_task_queue(struct task *task, FILE *fin);

//initializes some of the pthreads stuff
void initialize();

//Returns the next free thread of a list of 3, if none are available, it returns a zero
int next_free_thread(struct threadL* threadL);
void mark_thread(struct threadL* threadL, int id);
bool are_we_done(struct threadL*);

void create_thread_list(struct threadL *threadL, int threadNum, int counter);

//Fucntion Definitions
void create_thread_list(struct threadL *threadL, int threadNum, int counter){
	if(counter == threadNum){
		threadL->next = NULL;
	}
	else{
		counter++;
		struct threadL* next =  malloc(sizeof(threadL));
		threadL->next= next;
		next->isFree = true;
		next->id = counter;
		create_thread_list(next, threadNum, counter);
	}
}
int next_free_thread(struct threadL* threadL){
	if(threadL->isFree == true){
		threadL->isFree = false;
		return threadL->id;
	}
	else if(threadL->next == NULL){
		return 0;
	}
	return next_free_thread(threadL->next);
}

void mark_thread(struct threadL* threadL, int id){
	if(threadL->id == id){
		threadL->isFree = true;
		return;
	}
	else{
		mark_thread(threadL->next, id);
	}

}
bool are_we_done(struct threadL* threadL){
	if(threadL->isFree == false){
		return false;
	}
	else if(threadL->next == NULL){
		return true;
	}
	return are_we_done(threadL->next);
}
void initialize(){
	pthread_mutex_init(&cond_mutex,NULL);//Intialize the mutex with basic variables
	pthread_cond_init(&cond_cond,NULL);//Initialize the condition with basic variabless
}

//Create the Task Queue
void create_task_queue(struct task *task, FILE *fin){
	char action;//variable for the file to read into
	long num;//variable for the file to read into
	if(fscanf(fin, "%c %ld\n", &action, &num) != 2){//Base case, if the return of fscanf is != 2 then its EOF
		task->next = NULL;//Have the last node next point to null
	}
	else{//Recursive case, allocate a new task and assign the variables from the if statement
		struct task* next = malloc(sizeof(task));
		task->next = next;
		next->action = action;
		next->number = num;
		create_task_queue(next, fin);//Recursive call
	}
}

//update global aggregate variables given a number
void* calculate_square(void* args)
{
	//pthread_mutex_lock(&cond_mutex);
	struct arguements *arg = (struct arguements*)args;//Create a pointer to the pointer of args

	//printf("here in calc square begin\n");
	pthread_mutex_lock(&cond_mutex);
	long number = (*arg).num;//lock the current value of args to number
	int threadNum = (*arg).threadNum;//lock the current value ofargs to threadNum
	struct threadL* threadLHead = (*arg).threadLHead;
	pthread_cond_signal(&cond_cond);//Allow the master to keep assigning new args values
	pthread_mutex_unlock(&cond_mutex);
	long the_square = number*number;//Calculate the square
	sleep(number);

//Everything after this point will be the critical section
	//let's add this to our (global) sum
	pthread_mutex_lock(&cond_mutex);//Lock global variables
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
	mark_thread(threadLHead, threadNum);//mark the thread as available for reassignment
	pthread_mutex_unlock(&cond_mutex);//unlock mutex *may remove*
	return 0;
}


int main(int argc, char* argv[])
{	volatile struct arguements args;//Arguments for the calc square function
	initialize();
  	// check and parse command line options
  	if (argc != 3) {
    		printf("Usage: sumsq <infile>\n");
    		exit(EXIT_FAILURE);
  	}
	int threadCount = atoi(argv[2]);
	volatile struct threadL* threadHead = (threadL*) malloc(sizeof(threadL));
	threadHead->isFree = true;
	threadHead->id = 0;
	create_thread_list(threadHead, threadCount, 0);
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
  		create_task_queue(head, fin);//Build rest of queue
	}
	while(1){
		//Next 3 lines "pop" the next instruction from the queue
		pthread_mutex_lock(&cond_mutex);
		args.num = (*head).number;
		action = (*head).action;
		head = (*head).next;
		args.threadLHead = threadHead;
		pthread_mutex_unlock(&cond_mutex);
		switch(action){//Choose which action to preform
			case 'w':
				sleep(num);
				break;
			case 'p':
				pthread_mutex_lock(&cond_mutex);
				int freeThread = next_free_thread(threadHead->next);//Get the next free thread
				args.threadNum = freeThread;//Assign that free thread
				pthread_mutex_unlock(&cond_mutex);
				while(freeThread == 0){//if the thread is 0, there are no threads available so it must wait
					pthread_mutex_lock(&cond_mutex);
					if((freeThread = next_free_thread(threadHead->next)) != 0){//This may be extra
						args.threadNum = freeThread;//Assign the new thread to args
						pthread_mutex_unlock(&cond_mutex);
						break;
					}
					pthread_mutex_unlock(&cond_mutex);
				}
				pthread_create(&threadArray[freeThread],NULL,&calculate_square,(void*) &args);
				//Wait for calc square to assign values before getting new ones
				pthread_mutex_lock(&cond_mutex);
				pthread_cond_wait(&cond_cond, &cond_mutex);
				pthread_mutex_unlock(&cond_mutex);
				break;
		}
		//After all instructions are popped and assigned
		if(head == NULL){
			//Wait for all threads to finish
			while(1){
				pthread_mutex_lock(&cond_mutex);
				if(are_we_done(threadHead) == true){break;}
				pthread_mutex_unlock(&cond_mutex);
                        }
			break;
		}
	}
  	fclose(fin);
  	// print results
  	printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  	// clean up and return
  	return (EXIT_SUCCESS);
}
