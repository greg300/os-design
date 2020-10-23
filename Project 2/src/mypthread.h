// File:	mypthread_t.h

// List all group member's name: Gregory Giovannini
// username of iLab: gdg44
// iLab Server: vi.cs.rutgers.edu

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE
#define _XOPEN_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro. */
#define USE_MYTHREAD 1

#define READY 0
#define SCHEDULED 1
#define BLOCKED 2
#define EXITED 3

//#define STACK_SIZE 8192
#define STACK_SIZE 16384  // The size in bytes of a context's stack.
#define QUANTUM 5  // The time in ms after which a timer interrupt will trigger the scheduler.

/* Lib header files: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

/* A unique ID for a thread. */
typedef uint mypthread_t;

/* A data structure to hold all the details associated with a thread. */
typedef struct threadControlBlock
{
	/* add important states in a thread control block */
	mypthread_t threadID;  				// Thread ID.
	int threadStatus;  					// Thread status.
	ucontext_t * threadContext;  		// Thread context.
	stack_t * threadStack;  			// Thread stack.
	int timeQuantumsPassed;  			// Number of time quantums passed since thread start.
	mypthread_t threadWaitingToJoin;  	// Thread ID of the thread waiting to join on this thread.
	void * returnValuePtr;  			// Return value of this thread, if applicable.
} tcb;

/* Linked List node, representing a TCB of a thread in the runqueue or in the list of created threads. */
typedef struct node
{
	tcb * threadControlBlock;	// The TCB of the thread represented by this node.
	struct node * next;			// Reference to the next node in the Linked List.
} node;

/* [UNUSED] The Queue for scheduling created threads. */
typedef struct runqueue
{
	node * front;	// Reference to the first node in the Queue.
	node * rear;    // Reference to the last node in the Queue.
} runqueue;

// A Linked List of created threads.
typedef struct threadList
{
	node * front;  // Reference to the first node in the Linked List.
} threadList;

/* Mutex struct. */
typedef struct mypthread_mutex_t
{
	int isLocked;					// Maintains whether or not the thread is locked(1) or unlocked(0);
	tcb * lockingThread;			// Represents the thread that currently has control of the mutex.
	threadList * waitingThreads;	// Represents the list of all threads waiting on this mutex to be unlocked.
} mypthread_mutex_t;

/* Function Declarations: */
/* [HELPER] Create a new Linked List node. */
node * newNode(tcb * threadControlBlock);

/* [UNUSED][HELPER] Create a new, empty runqueue. */
runqueue * runqueueCreate();

/* [HELPER] Create a new, empty threadList. */
threadList * threadListCreate();

/* [UNUSED][HELPER] Add a thread to the runqueue. */
void runqueueEnqueue(runqueue * queue, tcb * threadControlBlock);

/* [HELPER] Add a thread to the beginning of the threadList. */
void threadListAdd(threadList * threads, tcb * threadControlBlock);

/* [UNUSED][HELPER] Remove a thread from the runqueue. */
void runqueueDequeue(runqueue * queue);

/* [HELPER] Remove a thread from the threadList, searching by TCB; return 1 if found, 0 if not. */
int threadListRemove(threadList * threads, tcb * threadControlBlock);

/* [HELPER] Search for a thread by threadID; return TCB if found, NULL if not. */
tcb * threadListSearch(threadList * threads, mypthread_t thread);

/* [HELPER] Free each node in the list. */
void threadListDestroy(threadList * threads);

/* [PTHREAD] Create a new thread. */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* [UNUSED][PTHREAD] Give CPU possession to other user-level threads voluntarily. */
int mypthread_yield();

/* [PTHREAD] Terminate a thread. */
void mypthread_exit(void *value_ptr);

/* [PTHREAD] Wait for thread termination. */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* [PTHREAD] Initialize the mutex lock. */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* [PTHREAD] Aquire the mutex lock. */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* [PTHREAD] Release the mutex lock. */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* [PTHREAD] Destroy the mutex. */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

/* [SCHEDULER] Invoked on timer interrupt; swaps into the scheduler context. */
void timeSigHandler(int sigNum);

/* [SCHEDULER] Invoked on first call to pthread_create(); sets up the timer and defines the timer interrupt action. */
void timerSetup();

/* [SCHEDULER] Main scheduler; picks scheduling policy to run. */
static void schedule();

/* [SCHEDULER] Preemptive SJF (STCF) scheduling algorithm. */
static void sched_stcf();

/* [SCHEDULER] Preemptive MLFQ scheduling algorithm. */
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
