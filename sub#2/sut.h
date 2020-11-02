#ifndef __SUT_H__
#define __SUT_H__
#include <stdbool.h>
#include <ucontext.h>

#define MAX_NUM_THREADS 15
#define TASK_STACK_SIZE 1024 * 64

typedef void (*sut_task_f)();

typedef struct __taskdesc
{
	int taskid;
	char *taskstack;
	void *taskfunc;
	ucontext_t taskcontext;
} taskdesc;

typedef struct __requestdesc
{
	int requestid;
	char *dest;
	int port;
	char *buf;

	bool sut_open;
	bool sut_write;
	bool sut_read;
	bool sut_close;

	ucontext_t requestcontext;
} requestdesc;

void sut_init();
bool sut_create(sut_task_f fn);
void sut_yield();
void sut_exit();
void sut_open(char *dest, int port);
void sut_write(char *buf, int size);
void sut_close();
char *sut_read();
void sut_shutdown();

#endif
