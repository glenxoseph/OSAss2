#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "sut.h"
#include "queue.h"

taskdesc taskarr[20];

taskdesc *tdescptr;
taskdesc *c_t;
taskdesc *curtask;

int numthreads, curthread;
int curtaskint;
int numtasks = 0;
struct queue task_ready_queue, wait_queue;
static pthread_t c_handle, i_handle;
static ucontext_t c_context, i_context;
static pthread_mutex_t task_ready_queue_lock = PTHREAD_MUTEX_INITIALIZER;

void *c_exec(void *arg) {

getcontext(&c_context);

	// busy waiting
	while (true)
	{
		//		printf("Line 28!\n");
		if (numthreads > 0)
		{
			break;
		}
	}

	while (true)
	{
		//		printf("Line 35!\n");
		pthread_mutex_lock(&task_ready_queue_lock);
		struct queue_entry *test = queue_peek_front(&task_ready_queue);
		pthread_mutex_unlock(&task_ready_queue_lock);

		if (test != NULL)
		{
			//			printf("Line 41!\n");
			struct queue_entry *pointer = queue_pop_head(&task_ready_queue);
			curtask = (taskdesc *)pointer->data;
			swapcontext(&c_context, &(curtask->taskcontext));
		}
		else
		{
			break;
		}
		usleep(100);
	}

//	printf("C-EXEC goodbye!\n");
	
}

void *i_exec(void *arg) {

//	printf("I-EXEC here we go!\n");
	
}

void sut_init() {

//	printf("INIT being called!\n");

    numthreads = 0;
	curthread = 0;
	curtaskint = 0;

    task_ready_queue = queue_create();
	queue_init(&task_ready_queue);
	wait_queue = queue_create();
	queue_init(&wait_queue);

	if (pthread_create(&c_handle, NULL, c_exec, NULL) == 0){
		printf("kernel threads created succesfully - c_exec!\n");
	}
	else {
		printf("kernel threads creation failed - c_exec!\n");
		return;
	}
    if (pthread_create(&i_handle, NULL, i_exec, NULL) == 0){
		printf("kernel threads created succesfully - i_exec!\n");
	}
	else {
		printf("kernel threads creation failed - i_exec!\n");
		return;
	}

//	printf("INIT done!\n");
}

bool sut_create(sut_task_f fn) {

//	printf("CREATE being called!\n");

//	pthread_mutex_lock(&task_ready_queue_lock);
	if (numthreads >= MAX_NUM_THREADS){
        printf("Maximum task created.\n");
//      pthread_mutex_unlock(&task_ready_queue_lock);
        return false;
    }
//	pthread_mutex_unlock(&task_ready_queue_lock);

	// code from YAU

	tdescptr = &(taskarr[numtasks]);

	getcontext(&(tdescptr->taskcontext));

	tdescptr->taskid = numtasks;
	tdescptr->taskstack = (char *)malloc(TASK_STACK_SIZE);
	tdescptr->taskcontext.uc_stack.ss_sp = tdescptr->taskstack;
	tdescptr->taskcontext.uc_stack.ss_size = TASK_STACK_SIZE;
	tdescptr->taskcontext.uc_link = 0;
	tdescptr->taskcontext.uc_stack.ss_flags = 0;
	tdescptr->taskfunc = *fn;

	makecontext(&(tdescptr->taskcontext), *fn, 1, tdescptr);
	struct queue_entry *node = queue_new_node(tdescptr);

	pthread_mutex_lock(&task_ready_queue_lock);
	queue_insert_tail(&task_ready_queue, node);
	pthread_mutex_unlock(&task_ready_queue_lock);

	numthreads++;
	numtasks++;

	printf("creation of task number %d is successful,\n %d tasks in total\n", tdescptr->taskid, numtasks);
    return true;
}

void sut_yield()
{

	//	printf("YIELD being called!\n");

	struct queue_entry *node = queue_new_node(curtask);

	pthread_mutex_lock(&task_ready_queue_lock);
	queue_insert_tail(&task_ready_queue, node);
	pthread_mutex_unlock(&task_ready_queue_lock);

	swapcontext(&(curtask->taskcontext), &(c_context));

	//	printf("YIELD done!\n");
}

void sut_exit() {

//	printf("EXIT being called!\n");

	setcontext(&c_context);

//	printf("EXIT done!\n");

}

void sut_open(char *dest, int port) {

}

void sut_write(char *buf, int size) {

}

char *sut_read() {

}

void sut_close() {

}

void sut_shutdown() {

//	printf("SHUTDOWN being called!\n");

	if (pthread_join(c_handle, NULL) == 0){
		printf("kernel threads shutdown succesfully - c_exec!\n");
	}
	else {
		printf("kernel threads shutdown failed - c_exec!\n");
		return;
	}
    if (pthread_join(i_handle, NULL) == 0){
		printf("kernel threads shutdown succesfully - i_exec!\n");
	}
	else {
		printf("kernel threads shutdown failed - i_exec!\n");
		return;
	}

//	printf("SHUTDOWN done!\n");

}