
#define _XOPEN_SOURCE

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

pthread_mutex_t mutex;
pthread_t * thread;
int * counter;

ucontext_t schedulerContext;

static void func1()
{
	printf("In func1.\n");
}

void vector_multiply(void* arg) {
	pthread_exit(NULL);
}

void sigHandler(int sigNum, siginfo_t * siginfo, void * context)
{
	printf("Signal\n");
	//swapcontext(runningThread -> threadContext, schedulerContext);
}

int main(int argc, char **argv) {

	// Create a context for the scheduler.
	//ucontext_t * scp = (ucontext_t *) malloc(sizeof(ucontext_t));
	//ucontext_t schedulerContext = *scp;
	ucontext_t main;

	stack_t * schedulerContextStack = (stack_t *) malloc(sizeof(stack_t));
	schedulerContextStack -> ss_sp = malloc(STACK_SIZE);
	schedulerContextStack -> ss_size = STACK_SIZE;
	schedulerContext.uc_stack = *schedulerContextStack;
	schedulerContext.uc_link = &main;
	//schedulerContext.uc_link = mcp;
	
	getcontext(&main);
	makecontext(&schedulerContext, func1, 0);
	swapcontext(&main, &schedulerContext);

	printf("Success.\n");

	// threadList * thrds = threadListCreate();
	// tcb * t1 = (tcb *) malloc(sizeof(tcb));
	// tcb * t2 = (tcb *) malloc(sizeof(tcb));
	// tcb * t3 = (tcb *) malloc(sizeof(tcb));
	// tcb * t4 = (tcb *) malloc(sizeof(tcb));
	// threadListAdd(thrds, t1);
	// threadListAdd(thrds, t2);
	// threadListAdd(thrds, t3);
	// threadListAdd(thrds, t4);
	// threadListDestroy(thrds);

	// struct sigaction act;
	// memset(&act, '\0', sizeof(act));

	// act.sa_sigaction = &sigHandler;
	// act.sa_flags = SA_SIGINFO;

	// sigaction(SIGALRM, &act, NULL);  // ITIMER_VIRTUAL: decrements only when the process is executing, and delivers SIGVTALRM upon expiration.
	
	// struct itimerval time;
	// time.it_value.tv_sec = 1;
	// time.it_value.tv_usec = 0;
	// time.it_interval = time.it_value;

	// setitimer(ITIMER_REAL, &time, NULL);

	// sleep(3);
	// sleep(3);
	// sleep(3);

	//setitimer(ITIMER_REAL, &time, NULL);

	//sleep(10);

	/* Implement HERE */
	// int i = 0;
	// if (__atomic_test_and_set((void *) &(i), 0) == 0)
	// {
	// 	printf("Free\n");
	// }
	// printf("%d\n", i);

	// initialize pthread_t
	// int i;
	// thread = (pthread_t *) malloc(sizeof(pthread_t));

	// counter = (int *) malloc(sizeof(int));
	// for (i = 0; i < 1; ++i)
	// 	counter[i] = i;

	// //pthread_mutex_init(&mutex, NULL);

	// for (i = 0; i < 1; ++i)
	// 	pthread_create(&thread[i], NULL, &vector_multiply, &counter[i]);

	// for (i = 0; i < 1; ++i)
	// 	pthread_join(thread[i], NULL);

	return 0;
}
