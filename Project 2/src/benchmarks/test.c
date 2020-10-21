#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
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

void vector_multiply(void* arg) {
	pthread_exit(NULL);
}

void sigHandler(int sigNum, siginfo_t * siginfo, void * context)
{
	printf("Signal\n");
	//swapcontext(runningThread -> threadContext, schedulerContext);
}

int main(int argc, char **argv) {

	struct sigaction act;
	memset(&act, '\0', sizeof(act));

	act.sa_sigaction = &sigHandler;
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGALRM, &act, NULL);  // ITIMER_VIRTUAL: decrements only when the process is executing, and delivers SIGVTALRM upon expiration.
	
	struct itimerval time;
	time.it_value.tv_sec = 1;
	time.it_value.tv_usec = 0;
	time.it_interval = time.it_value;

	setitimer(ITIMER_REAL, &time, NULL);

	sleep(3);
	sleep(3);
	sleep(3);

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
