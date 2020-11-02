#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "sut.h"
#include "queue.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

taskdesc taskarr[20];

taskdesc *tdescptr;
taskdesc *c_t;
taskdesc *curtask;

int numthreads, curthread;
int curtaskint;
int numtasks = 0;

// initialize queues
struct queue task_ready_queue, wait_queue, in_io_queue, out_io_queue;
static pthread_t c_handle, i_handle;
static ucontext_t c_context, i_context;

// locks
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t io_queue_lock = PTHREAD_MUTEX_INITIALIZER;

int sockfd;

void *c_exec(void *arg)
{

	//	printf("C-EXEC here we go!\n");

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

		// avoid data race
		pthread_mutex_lock(&lock);
		struct queue_entry *test = queue_peek_front(&task_ready_queue);
		pthread_mutex_unlock(&lock);

		if (test != NULL) // if not null, then assign
		{
			//			printf("Line 41!\n");
			struct queue_entry *pointer = queue_pop_head(&task_ready_queue);
			curtask = (taskdesc *)pointer->data;
			swapcontext(&c_context, &(curtask->taskcontext));
		}
		else // if null, break
		{
			break;
		}
		usleep(100);
	}

	//	printf("C-EXEC goodbye!\n");
}

void *i_exec(void *arg)
{

	//	printf("I-EXEC here we go!\n");

	getcontext(&i_context);

	// busy waiting
	while (true)
	{
		if (numthreads > 0)
		{
			break;
		}
	}

	while (true)
	{
//		printf("Line 93!\n");

		// avoid data race
		pthread_mutex_lock(&io_queue_lock);
		struct queue_entry *req = queue_peek_front(&in_io_queue);
		pthread_mutex_unlock(&io_queue_lock);

		requestdesc *message = (requestdesc *)req->data;

		// avoid data race
		pthread_mutex_lock(&lock);
		struct queue_entry *pointer = queue_pop_head(&wait_queue);
		pthread_mutex_unlock(&lock);

		taskdesc *task = (taskdesc *)pointer->data;

		if (!message->sut_close && !message->sut_read && message->sut_open && !message->sut_write)
		{
			// open

			printf("open!\n");

			sockfd = socket(AF_INET, SOCK_STREAM, 0);

			if (sockfd < 0)
			{
				printf("socket creation error!");
				pthread_exit(0);
			}
			struct sockaddr_in server_address = { 0 };
			inet_pton(AF_INET, message->dest, &(server_address.sin_addr.s_addr));
			server_address.sin_port = htons(message->port);
			if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
			{
				printf("Failed to connect to socket, exiting.\n");
				pthread_exit(0);
			}
		}
		if (!message->sut_close && !message->sut_read && !message->sut_open && message->sut_write)
		{
			// write
			send(sockfd, message->buf, sizeof(message->buf), 0);
		}
		if (!message->sut_close && message->sut_read && !message->sut_open && !message->sut_write)
		{
			// read
			recv(sockfd, message->buf, sizeof(message->buf), 0);
		}
		if (message->sut_close && !message->sut_read && !message->sut_open && !message->sut_write)
		{
			// close
			close(sockfd);
		}
		usleep(100);
	}
}

void sut_init()
{

	//	printf("INIT being called!\n");

	numthreads = 0;
	curthread = 0;
	curtaskint = 0;

	// initialize queues

	task_ready_queue = queue_create();
	queue_init(&task_ready_queue);
	wait_queue = queue_create();
	queue_init(&wait_queue);

	in_io_queue = queue_create();
	queue_init(&in_io_queue);
	out_io_queue = queue_create();
	queue_init(&out_io_queue);

	if (pthread_create(&c_handle, NULL, c_exec, NULL) == 0)
	{
		printf("kernel threads created succesfully - c_exec!\n");
	}
	else
	{
		printf("kernel threads creation failed - c_exec!\n");
		return;
	}
	if (pthread_create(&i_handle, NULL, i_exec, NULL) == 0)
	{
		printf("kernel threads created succesfully - i_exec!\n");
	}
	else
	{
		printf("kernel threads creation failed - i_exec!\n");
		return;
	}

	//	printf("INIT done!\n");
}

bool sut_create(sut_task_f fn)
{

	//	printf("CREATE being called!\n");

	//	pthread_mutex_lock(&lock);
	if (numthreads >= MAX_NUM_THREADS)
	{
		printf("Maximum task created.\n");
		//      pthread_mutex_unlock(&lock);
		return false;
	}
	//	pthread_mutex_unlock(&lock);

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

	// avoid data race
	pthread_mutex_lock(&lock);
	queue_insert_tail(&task_ready_queue, node);
	pthread_mutex_unlock(&lock);

	numthreads++;
	numtasks++;

	//	printf("creation of task number %d is successful,\n %d tasks in total\n", tdescptr->taskid, numtasks);
	return true;
}

void sut_yield()
{

	//	printf("YIELD being called!\n");

	struct queue_entry *node = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	queue_insert_tail(&task_ready_queue, node);
	pthread_mutex_unlock(&lock);

	swapcontext(&(curtask->taskcontext), &(c_context));

	//	printf("YIELD done!\n");
}

void sut_exit()
{

	//	printf("EXIT being called!\n");

	setcontext(&c_context);

	//	printf("EXIT done!\n");
}

void sut_open(char *dest, int port)
{

	requestdesc *request = (requestdesc *)malloc(sizeof(requestdesc));

	// set to open
	request->sut_close = false;
	request->sut_open = true;
	request->sut_read = false;
	request->sut_write = false;

	// set dest and port
	request->dest = dest;
	request->port = port;

	struct queue_entry *pointer1 = queue_new_node(request);
	struct queue_entry *pointer2 = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	pthread_mutex_lock(&io_queue_lock);
	queue_insert_tail(&in_io_queue, pointer1);
	queue_insert_tail(&wait_queue, pointer2);
	pthread_mutex_unlock(&io_queue_lock);
	pthread_mutex_unlock(&lock);

	swapcontext(&(curtask->taskcontext), &(c_context));
}

void sut_write(char *buf, int size)
{

	requestdesc *request = (requestdesc *)malloc(sizeof(requestdesc));

	// set to write
	request->sut_close = false;
	request->sut_open = false;
	request->sut_read = false;
	request->sut_write = true;

	// set buf
	request->buf = buf;

	struct queue_entry *pointer1 = queue_new_node(request);
	struct queue_entry *pointer2 = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	pthread_mutex_lock(&io_queue_lock);
	queue_insert_tail(&in_io_queue, pointer1);
	queue_insert_tail(&wait_queue, pointer2);
	pthread_mutex_unlock(&io_queue_lock);
	pthread_mutex_unlock(&lock);
}

char *sut_read()
{

	requestdesc *request = (requestdesc *)malloc(sizeof(requestdesc));

	// set to read
	request->sut_close = false;
	request->sut_open = false;
	request->sut_read = true;
	request->sut_write = false;

	struct queue_entry *pointer1 = queue_new_node(request);
	struct queue_entry *pointer2 = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	pthread_mutex_lock(&io_queue_lock);
	queue_insert_tail(&in_io_queue, pointer1);
	queue_insert_tail(&wait_queue, pointer2);
	pthread_mutex_unlock(&io_queue_lock);
	pthread_mutex_unlock(&lock);

	swapcontext(&(curtask->taskcontext), &(c_context));

	pthread_mutex_lock(&io_queue_lock);
	struct queue_entry *pointer3 = queue_pop_head(&out_io_queue);
	pthread_mutex_unlock(&io_queue_lock);

	requestdesc *rrr = (requestdesc *)pointer3->data;

	// place it into task ready queue.

	struct queue_entry *pointer4 = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	queue_insert_tail(&task_ready_queue, pointer4);
	pthread_mutex_unlock(&lock);

	swapcontext(&(curtask->taskcontext), &(i_context));

	char message[1000] = {0};
	strcpy(message, rrr->buf);

	return message;
}

void sut_close()
{

	requestdesc *request = (requestdesc *)malloc(sizeof(requestdesc));

	// set to close
	request->sut_close = true;
	request->sut_open = false;
	request->sut_read = false;
	request->sut_write = false;

	struct queue_entry *pointer1 = queue_new_node(request);
	struct queue_entry *pointer2 = queue_new_node(curtask);

	// avoid data race
	pthread_mutex_lock(&lock);
	pthread_mutex_lock(&io_queue_lock);
	queue_insert_tail(&in_io_queue, pointer1);
	queue_insert_tail(&wait_queue, pointer2);
	pthread_mutex_unlock(&io_queue_lock);
	pthread_mutex_unlock(&lock);
}

void sut_shutdown()
{

	//	printf("SHUTDOWN being called!\n");

	if (pthread_join(c_handle, NULL) == 0)
	{
		printf("kernel threads shutdown succesfully - c_exec!\n");
	}
	else
	{
		printf("kernel threads shutdown failed - c_exec!\n");
		return;
	}
	if (pthread_join(i_handle, NULL) == 0)
	{
		printf("kernel threads shutdown succesfully - i_exec!\n");
	}
	else
	{
		printf("kernel threads shutdown failed - i_exec!\n");
		return;
	}

	//	printf("SHUTDOWN done!\n");
}