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

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
int threadsCreated = 0;
ucontext_t mainContext;
runqueue * threadRunqueue;

/* Create a new Linked List node. */
queueNode * newQueueNode(tcb * threadControlBlock)
{
	queueNode * temp = (queueNode *) malloc(sizeof(queueNode));
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

/* Add a thread to the runqueue. */
void runqueueEnqueue(runqueue * queue, tcb * threadControlBlock)
{
	// Create a Linked List node.
	queueNode * temp = newQueueNode(threadControlBlock);

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

/* Remove a thread from the runqueue. */
void runqueueDequeue(runqueue * queue)
{
	// If queue is empty, return NULL.
	if (queue -> front == NULL)
	{
		return;
	}

	// Store old front, and move current front one node ahead.
	queueNode * temp = queue -> front;
	queue -> front = queue -> front -> next;
	
	// If front becomes NULL (i.e., queue is empty), then fix rear to be NULL as well.
	if (queue -> front == NULL)
	{
		queue -> rear = NULL;
	}

	free(temp);
}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg)
{
	// If this is the first thread being created, create a runqueue.
	if (threadsCreated == 0)
	{
		threadRunqueue = runqueueCreate();
	}
	// create Thread Control Block
	tcb * controlBlock = (tcb *) malloc(sizeof(tcb));
	controlBlock -> threadID = thread;
	controlBlock -> threadStatus = READY;

	// create and initialize the context of this thread
	ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t));
	newContext -> uc_link = &mainContext;
	
	controlBlock -> threadContext = newContext;

	// allocate space of stack for this thread to run
	stack_t * newContextStack = (stack_t *) malloc(sizeof(stack_t));
	newContext -> uc_stack = *newContextStack;

	controlBlock -> threadStack = newContextStack;

	// after everything is all set, push this thread int
	makecontext(newContext, (void (*)()) function, 1, arg);
	runqueueEnqueue(threadRunqueue, controlBlock);

	// YOUR CODE HERE
	threadsCreated++;
	// If this is the first thread being created, get the context of this thread.
	if (threadsCreated == 1)
	{
		getcontext(&mainContext);
	}
    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

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
