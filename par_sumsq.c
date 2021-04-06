
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

//This contains information about each thread and what its current state is
typedef struct thread{
	bool isFree;
	int threadID;
	pthread_t *threadName;
}thread;
volatile struct thread thread_array[4];

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
}arguements;

// function prototypes
//Main work to be done function
void *calculate_square(void* args);

//Creates a singly linked list of tasks to be taken from
void create_task_queue(struct task *task, FILE *fin);

//initializes some of the pthreads stuff
void initialize();

//Returns the next free thread of a list of 3, if none are available, it returns a zero
int next_free_thread();

//Fucntion Definitions
//Return the next free thread
int next_free_thread(){
	if(thread_array[1].isFree == true){//Give the thread number and mark it as in use
		thread_array[1].isFree = false;
		return 1;
	}
	else if(thread_array[2].isFree == true){//Return the thread number and mark it as in use
		thread_array[2].isFree = false;
		return 2;
	}
	else if(thread_array[3].isFree == true){//Return the thread number and mark it as in use
		thread_array[3].isFree = false;
		return 3;
	}
	else{//Return 0 (the master thread) and do not mark it, as the 0 thread will never be
	     //passed into pthread function
		return 0;
	}
}

//initialize Pthreads variables
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
	long number = (*arg).num;//lock the current value of args to number
	int threadNum = (*arg).threadNum;//lock the current value ofargs to threadNum
	pthread_cond_signal(&cond_cond);//Allow the master to keep assigning new args values
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
	thread_array[threadNum].isFree = true;//mark the thread as available for reassignment
	pthread_mutex_unlock(&cond_mutex);//Unlock the mutex
	pthread_mutex_lock(&cond_mutex);//Relock mutex to signal *may remove*
	pthread_cond_signal(&cond_cond);//allow any locked and waiting tasks to be assigned
	pthread_mutex_unlock(&cond_mutex);//unlock mutex *may remove*
	return 0;
}


int main(int argc, char* argv[])
{	pthread_t masterThread, thread1, thread2, thread3;//All 4 threads
	volatile struct arguements args;//Arguments for the calc square function
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
  		create_task_queue(head, fin);//Build rest of queue
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
		pthread_mutex_lock(&cond_mutex);
		args.num = (*head).number;
		action = (*head).action;
		head = (*head).next;
		pthread_mutex_unlock(&cond_mutex);
		switch(action){//Choose which action to preform
			case 'w':
				sleep(num);
				break;
			case 'p':
				pthread_mutex_lock(&cond_mutex);
				int freeThread = next_free_thread();//Get the next free thread
				args.threadNum = freeThread;//Assign that free thread
				pthread_mutex_unlock(&cond_mutex);
				while(freeThread == 0){//if the thread is 0, there are no threads available so it must wait
					pthread_cond_wait(&cond_cond, &cond_mutex);//Wait for a thread to signal availablity
					if((freeThread = next_free_thread()) != 0){//This may be extra
						args.threadNum = freeThread;//Assign the new thread to args
						break;
					}
				}
				pthread_mutex_unlock(&cond_mutex);
				pthread_create(thread_array[freeThread].threadName,NULL,&calculate_square,(void*) &args);
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
				if(thread_array[0].isFree == true &&
					thread_array[1].isFree == true &&
					thread_array[2].isFree == true &&
					thread_array[3].isFree == true){break;}
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

