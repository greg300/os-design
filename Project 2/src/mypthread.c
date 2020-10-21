// File:	mypthread.c

// List all group member's name: Gregory Giovannini
// username of iLab:
// iLab Server:

#include "mypthread.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
int threadsCreated = 0;
ucontext_t schedulerContext;
ucontext_t mainContext;
threadList * threadRunqueue;
threadList * allThreads;
tcb * runningThread;
tcb * mainThread;
struct itimerval timer;
struct sigaction timerAction;

/* Create a new Linked List node. */
node * newNode(tcb * threadControlBlock)
{
	node * temp = (node *) malloc(sizeof(node));
	temp -> threadControlBlock = threadControlBlock;
	temp -> next = NULL;
	return temp;
}

/* Create a new, empty runqueue. */
runqueue * runqueueCreate()
{
	runqueue * queue = (runqueue *) malloc(sizeof(runqueue));
	queue -> front = NULL;
	queue -> rear = NULL;
	return queue;
}

/* Create a new, empty threadList. */
threadList * threadListCreate()
{
	threadList * threads = (threadList *) malloc(sizeof(threadList));
	threads -> front = NULL;
	return threads;
}

/* Add a thread to the runqueue. */
void runqueueEnqueue(runqueue * queue, tcb * threadControlBlock)
{
	// Create a Linked List node.
	node * temp = newNode(threadControlBlock);

	// If this is the first insertion (runqueue is empty), set new node to both front and rear.
	if (queue -> rear == NULL)
	{
		queue -> front = temp;
		queue -> rear = temp;
		return;
	}

	// Otherwise, append new node to end of queue and update rear.
	queue -> rear -> next = temp;
	queue -> rear = temp;
}

/* Add a thread to the beginning of the threadList. */
void threadListAdd(threadList * threads, tcb * threadControlBlock)
{
	// Create a Linked List node.
	node * temp = newNode(threadControlBlock);

	temp -> next = threads -> front;
	threads -> front = temp;
}

/* Remove a thread from the runqueue. */
void runqueueDequeue(runqueue * queue)
{
	// If queue is empty, return nothing.
	if (queue -> front == NULL)
	{
		return;
	}

	// Store old front, and move current front one node ahead.
	node * temp = queue -> front;
	queue -> front = queue -> front -> next;
	
	// If front becomes NULL (i.e., queue is empty), then fix rear to be NULL as well.
	if (queue -> front == NULL)
	{
		queue -> rear = NULL;
	}

	free(temp);
}

/* Remove a thread from the threadList; return 1 if found, 0 if not. */
int threadListRemove(threadList * threads, tcb * threadControlBlock)
{
	node * current = threads -> front;
	node * prev = NULL;

	// If list is empty, return 0.
	if (current == NULL)
	{
		return 0;
	}
	// If front is the node to be deleted, change head and free old head.
	if (current -> threadControlBlock == threadControlBlock)
	{
		threads -> front = current -> next;
		free(current);
		return 1;
	}

	// Traverse the list to find the node to delete.
	while (current != NULL && current -> threadControlBlock != threadControlBlock)
	{
		prev = current;
		current = current -> next;
	}

	// If the node was not present in the list, return 0.
	if (current == NULL)
	{
		return 0;
	}

	// Otherwise, unlink the node from the list.
	prev -> next = current -> next;
	free(current);

	return 1;
}

/* Search for a thread by threadID; return TCB if found, NULL if not. */
tcb * threadListSearch(threadList * threads, mypthread_t * thread)
{
	node * current = threads -> front;

	// If list is empty, return NULL.
	if (current == NULL)
	{
		return NULL;
	}

	while (current != NULL)
	{
		if (current -> threadControlBlock -> threadID == thread)
		{
			return current -> threadControlBlock;
		}
		current = current -> next;
	}

	return NULL;
}

/* Free each node in the list. */
void threadListDestroy(threadList * threads)
{
	node * current = threads -> front;
	node * next;

	while (current != NULL)
	{
		next = current -> next;
		free(current);
		current = next;
	}

	threads -> front = NULL;
}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg)
{	
	// create Thread Control Block
	tcb * controlBlock = (tcb *) malloc(sizeof(tcb));
	controlBlock -> threadID = thread;
	controlBlock -> timeQuantumsPassed = 0;
	controlBlock -> threadStatus = READY;

	// create and initialize the context of this thread
	ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t));
	// ucontext_t * originalContext = (ucontext_t *) malloc(sizeof(ucontext_t));
	// newContext -> uc_link = originalContext;
	
	controlBlock -> threadContext = newContext;

	// allocate space of stack for this thread to run
	stack_t * newContextStack = (stack_t *) malloc(sizeof(stack_t));
	newContextStack -> ss_sp = malloc(STACK_SIZE);
	newContextStack -> ss_size = STACK_SIZE;
	newContext -> uc_stack = *newContextStack;

	controlBlock -> threadStack = newContextStack;

	// If this is the first thread being created, do some initialization.
	if (__atomic_test_and_set((void *) &(threadsCreated), 0) == 0)
	{
		threadRunqueue = threadListCreate();
		allThreads = threadListCreate();
		timerSetup();

		// Create an extra thread for the main program.
		tcb * mainControlBlock = (tcb *) malloc(sizeof(tcb));
		mainControlBlock -> threadID = thread + 1;
		mainControlBlock -> timeQuantumsPassed = 0;
		mainControlBlock -> threadStatus = READY;
		mainControlBlock -> threadContext = &mainContext;
		mainControlBlock -> threadStack = &(mainControlBlock -> threadContext -> uc_stack);

		runningThread = mainControlBlock;

		// Create a context for the scheduler.
		stack_t * schedulerContextStack = (stack_t *) malloc(sizeof(stack_t));
		schedulerContextStack -> ss_sp = malloc(STACK_SIZE);
		schedulerContextStack -> ss_size = STACK_SIZE;
		schedulerContext.uc_stack = *schedulerContextStack;
		schedulerContext.uc_link = &mainContext;

		getcontext(&mainContext);
		makecontext(&schedulerContext, schedule, 0);
	}
	else
	{
		//printf("Incrementing thread counter\n");
		threadsCreated++;
	}
	printf("Created thread %d\n", threadsCreated);

	// after everything is all set, push this thread int
	threadListAdd(threadRunqueue, controlBlock);
	threadListAdd(allThreads, controlBlock);

	// YOUR CODE HERE
	newContext -> uc_link = &mainContext;
	getcontext(&mainContext);
	makecontext(newContext, (void (*)()) function, 1, arg);

	getcontext(&mainContext);

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield()
{
	// change thread state from Running to Ready
	runningThread -> threadStatus = READY;

	// save context of this thread to its thread control block
	//getcontext(runningThread -> threadContext);

	// wwitch from thread context to scheduler context
	//swapcontext(runningThread -> threadContext, &mainContext);
	swapcontext(runningThread -> threadContext, &schedulerContext);

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// If value_ptr is not NULL, save the return value.
	// if (value_ptr != NULL)
	// {
	// 	runningThread -> returnValuePtr = value_ptr;
	// }
	runningThread -> returnValuePtr = value_ptr;
	
	// If a thread was waiting to join on this runningThread, unblock the waiting thread.
	node * current = allThreads -> front;
	while (current != NULL)
	{
		if (runningThread -> threadWaitingToJoin == current -> threadControlBlock -> threadID)
		{
			current -> threadControlBlock -> threadStatus = READY;
			break;
		}
		current = current -> next;
	}

	// Deallocated any dynamic memory created when starting this thread
	//printf("EXITING!\n");
	runningThread -> threadStatus = EXITED;
	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// wait for a specific thread to terminate

	// if (*(runningThread -> threadID) == thread && runningThread -> threadStatus == EXITED)

	// Look for the thread in the threadList.
	tcb * controlBlock = threadListSearch(allThreads, &thread);

	// If the given thread is not in the threadList, it must have already terminated.
	if (controlBlock == NULL)
	{
		return 0;
	}

	// Block until the given thread terminates.
	if (controlBlock -> threadStatus != EXITED)
	{
		runningThread -> threadStatus = BLOCKED;
		threadListRemove(threadRunqueue, runningThread);
		controlBlock -> threadWaitingToJoin = runningThread -> threadID;
		swapcontext(runningThread -> threadContext, &schedulerContext);
	}

	if (value_ptr != NULL)
	{
		*(value_ptr) = runningThread -> returnValuePtr;
	}

	// de-allocate any dynamic memory created by the joining thread
	threadListRemove(allThreads, controlBlock);
	free(controlBlock -> threadStack -> ss_sp);
	free(controlBlock -> threadStack);
	//free(controlBlock -> threadContext -> uc_link);
	free(controlBlock -> threadContext);
	free(controlBlock);

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex
	mutex -> isLocked = 0;
	// Initialize a list for any threads that may be waiting on this mutex.
	mutex -> waitingThreads = threadListCreate();

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread
		
		// If the mutex is free, acquire it.
		if (__atomic_test_and_set((void *) &(mutex -> isLocked), 0) == 0)
		{
			// mutex -> isLocked = 1
			mutex -> lockingThread = runningThread;
			return 0;
		}
		// Otherwise, block this thread and move to the scheduler.
		else
		{
			runningThread -> threadStatus = BLOCKED;
			threadListRemove(threadRunqueue, runningThread);
			threadListAdd(mutex -> waitingThreads, runningThread); 
			swapcontext(runningThread -> threadContext, &schedulerContext);
		}
		
        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	mutex -> isLocked = 0;
	mutex -> lockingThread = NULL;

	// Add all blocked threads to the runqueue.
	node * current = mutex -> waitingThreads -> front;
	while (current != NULL)
	{
		current -> threadControlBlock -> threadStatus = READY;
		threadListAdd(threadRunqueue, current -> threadControlBlock);
		current = current -> next;
	}

	// Remove all threads from the mutex's block list.
	threadListDestroy(mutex -> waitingThreads);

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	// Remove all threads from the mutex's block list. (Should already be empty.)
	threadListDestroy(mutex -> waitingThreads);
	free(mutex -> waitingThreads);
	return 0;
};

// void incrementTimeQuantums()
// {
// 	node * current = allThreads -> front;

// 	while (current != NULL)
// 	{
// 		if (current -> threadControlBlock -> threadStatus)
// 		current -> threadControlBlock -> timeQuantumsPassed++;
// 		current = current -> next;
// 	}
// }

// void timeSigHandler(int sigNum, siginfo_t * siginfo, void * context)
// {
// 	// Increment time quantums elapsed for running thread.
// 	runningThread -> timeQuantumsPassed++;

// 	printf("Quantum passed, swapping into scheduler...\n");
// 	swapcontext(runningThread -> threadContext, &schedulerContext);
// }

void timeSigHandler(int sigNum)
{
	// Increment time quantums elapsed for running thread.
	if (runningThread != NULL)
	{
		runningThread -> timeQuantumsPassed++;

		printf("Quantum passed, swapping into scheduler...\n");
		swapcontext(runningThread -> threadContext, &schedulerContext);
	}
}

void timerSetup()
{
	// memset(&timerAction, '\0', sizeof(timerAction));
	// timerAction.sa_sigaction = &timeSigHandler;
	// timerAction.sa_flags = SA_SIGINFO;
	timerAction.sa_handler = timeSigHandler;
	sigemptyset(&timerAction.sa_mask);
	timerAction.sa_flags = 0;

	sigaction(SIGPROF, &timerAction, NULL); 

	timer.it_interval.tv_sec = QUANTUM / 1000;
	timer.it_interval.tv_usec = (QUANTUM * 1000) % 1000000;

	timer.it_value = timer.it_interval;

	setitimer(ITIMER_PROF, &timer, NULL);
	
	//act.sa_handler = timeSigHandler;
	//sigemptyset(&act.sa_mask);
}

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Freeze the timer.
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	// 		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
	sched_stcf();
#else
	// Choose MLFQ
	sched_mlfq();
#endif
	// Un-freeze the timer.
	timer.it_value = timer.it_interval;

	// Context switch into the next thread to run.
	if (runningThread != NULL)
	{
		setcontext(runningThread -> threadContext);
	}
	else
	{
		return;
		//printf("No running thread.\n");
		//setcontext(&mainContext);
		//exit(0);
	}
	
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	node * current;
	// Add any threads that are "ready" to the runqueue.
	current = allThreads -> front;
	while (current != NULL)
	{
		if (current -> threadControlBlock -> threadStatus == READY)
		{
			threadListAdd(threadRunqueue, current -> threadControlBlock);
		}
	}

	// If a thread was previously running, add it back to the runqueue.
	if (runningThread != NULL)
	{
		runningThread -> threadStatus = READY;
		threadListAdd(threadRunqueue, runningThread);
	}

	current = threadRunqueue -> front;

	// If runqueue is empty, do nothing and stay in current thread.
	if (current == NULL)
	{
		return;
	}

	// Find the thread with least number of elapsed time quantums.
	int minTimeQuantums = current -> threadControlBlock -> timeQuantumsPassed;
	tcb * minTimeQuantumTCB = current -> threadControlBlock;
	while (current != NULL)
	{
		if (current -> threadControlBlock -> timeQuantumsPassed < minTimeQuantums)
		{
			if (current -> threadControlBlock -> threadStatus == SCHEDULED)
			{
				minTimeQuantums = current -> threadControlBlock -> timeQuantumsPassed;
				minTimeQuantumTCB = current -> threadControlBlock;
			}
		}
		current = current -> next;
	}

	runningThread = minTimeQuantumTCB;
	runningThread -> threadStatus = SCHEDULED;
	threadListRemove(threadRunqueue, runningThread);

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE