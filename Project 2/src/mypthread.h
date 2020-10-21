// File:	mypthread_t.h

// List all group member's name: Gregory Giovannini
// username of iLab:
// iLab Server:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE
#define _XOPEN_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

#define READY 0
#define SCHEDULED 1
#define BLOCKED 2
#define EXITED 3

#define STACK_SIZE 8192
#define QUANTUM 5  // in ms

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

typedef uint mypthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	mypthread_t * threadID;  // thread Id
	int threadStatus;  // thread status
	ucontext_t * threadContext;  // thread context
	stack_t * threadStack;  // thread stack
	int timeQuantumsPassed;  // Number of time quantums passed since thread start.
	mypthread_t * threadWaitingToJoin;
	// thread priority
	void * returnValuePtr;  // return value
	// And more ...

	// YOUR CODE HERE
} tcb;

// Linked List node, representing a TCB of a thread in the runqueue or in the list of created threads.
typedef struct node {
	tcb * threadControlBlock;
	struct node * next;
} node;

// The runqueue for created threads.
typedef struct runqueue {
	node * front;
	node * rear;
} runqueue;

// A Linked List of created threads.
typedef struct threadList {
	node * front;
} threadList;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */
	int isLocked;
	tcb * lockingThread;
	threadList * waitingThreads;
	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

/* Function Declarations: */

/* Create a new Linked List node. */
node * newNode(tcb * threadControlBlock);

/* Create a new, empty runqueue. */
runqueue * runqueueCreate();

/* Create a new, empty threadList. */
threadList * threadListCreate();

/* Add a thread to the runqueue. */
void runqueueEnqueue(runqueue * queue, tcb * threadControlBlock);

/* Add a thread to the beginning of the threadList. */
void threadListAdd(threadList * threads, tcb * threadControlBlock);

/* Remove a thread from the runqueue. */
void runqueueDequeue(runqueue * queue);

/* Remove a thread from the threadList; return 1 if found, 0 if not. */
int threadListRemove(threadList * threads, tcb * threadControlBlock);

/* Search for a thread by threadID; return TCB if found, NULL if not. */
tcb * threadListSearch(threadList * threads, mypthread_t * thread);

/* Free each node in the list. */
void threadListDestroy(threadList * threads);

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

//void timeSigHandler(int sigNum, siginfo_t * siginfo, void * context);
void timeSigHandler(int sigNum);

void timerSetup();

/* scheduler */
static void schedule();

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf();

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq();

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
