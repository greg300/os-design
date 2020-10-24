// File:	mypthread.c

// List all group member's name: Gregory Giovannini
// username of iLab: gdg44
// iLab Server: vi.cs.rutgers.edu

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

// Global variables.
int threadsCreated = 0;  		// Number of total created threads (not necessarily size of allThreads).
int isFirst = 0;  				// For an atomic check as to whether pthread_create() is being called for the first time.
ucontext_t schedulerContext;  	// The context of the scheduler, to be swapped into on yield.
ucontext_t mainContext;  		// The context of the main program (given by the first context to call pthread_create()).
threadList * threadRunqueue;  	// List of all threads scheduled to run.
threadList * allThreads;  		// List of all created threads that have not yet been joined.
int allThreadsCount = 0;  		// Size of allThreads.
tcb * runningThread;			// Thread Control Block for the currently-running thread.				
struct itimerval timer;			// Interval Timer used to signal the scheduler with SIGPROF.
struct sigaction timerAction;	// Defines the action to be taken upon timer interrupt.

/* [HELPER] Create a new Linked List node. */
node * newNode(tcb * threadControlBlock)
{
	// Allocate space for a TCB node.
	node * temp = (node *) malloc(sizeof(node));
	// Set the TCB of the node to the given TCB.
	temp -> threadControlBlock = threadControlBlock;
	// Initialize the next node to NULL.
	temp -> next = NULL;
	// Return the created node.
	return temp;
}

/* [UNUSED][HELPER] Create a new, empty runqueue. */
runqueue * runqueueCreate()
{
	// Allocate space for a Queue.
	runqueue * queue = (runqueue *) malloc(sizeof(runqueue));
	// Initialize the front and rear to NULL.
	queue -> front = NULL;
	queue -> rear = NULL;
	// Return a reference to the Queue.
	return queue;
}

/* [HELPER] Create a new, empty threadList. */
threadList * threadListCreate()
{
	// Allocate space for a Linked List.
	threadList * threads = (threadList *) malloc(sizeof(threadList));
	// Initialize the front to NULL.
	threads -> front = NULL;
	// Return a reference to the Linked List.
	return threads;
}

/* [UNUSED][HELPER] Add a thread to the runqueue. */
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

/* [HELPER] Add a thread to the beginning of the threadList. */
void threadListAdd(threadList * threads, tcb * threadControlBlock)
{
	// Create a Linked List node.
	node * temp = newNode(threadControlBlock);

	// Set the new node's next to be the previous front, and set the new front to be the new node.
	temp -> next = threads -> front;
	threads -> front = temp;
}

/* [UNUSED][HELPER] Remove a thread from the runqueue. */
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

	// Free the allocated node.
	free(temp);
}

/* [HELPER] Remove a thread from the threadList, searching by TCB; return 1 if found, 0 if not. */
int threadListRemove(threadList * threads, tcb * threadControlBlock)
{
	// Start at the front of the list, and maintain track of the previous node.
	node * current = threads -> front;
	node * prev = NULL;

	// If list is empty, return 0 (not found).
	if (current == NULL)
	{
		// Not found.
		return 0;
	}
	// If front is the node to be deleted, change new front and free old front, and return 1 (found).
	if (current -> threadControlBlock == threadControlBlock)
	{
		// Front becomes front's next.
		threads -> front = current -> next;
		// Free the allocated node.
		free(current);
		// Found.
		return 1;
	}

	// Traverse the list to find the node to delete.
	while (current != NULL && current -> threadControlBlock != threadControlBlock)
	{
		prev = current;
		current = current -> next;
	}

	// If the node was not present in the list, return 0 (not found).
	if (current == NULL)
	{
		// Not found.
		return 0;
	}

	// Otherwise, unlink the node from the list.
	prev -> next = current -> next;
	// Free the allocated node.
	free(current);
	// Found.
	return 1;
}

/* [HELPER] Search for a thread by threadID; return TCB if found, NULL if not. */
tcb * threadListSearch(threadList * threads, mypthread_t thread)
{
	// Start at the front of the list.
	node * current = threads -> front;

	// If list is empty, return NULL.
	if (current == NULL)
	{
		// Not found.
		return NULL;
	}

	// Traverse the list for the node with the given threadID.
	while (current != NULL)
	{
		if (current -> threadControlBlock -> threadID == thread)
		{
			// Found.
			return current -> threadControlBlock;
		}
		current = current -> next;
	}

	// Not found.
	return NULL;
}

/* [HELPER] Free each node in the list. */
void threadListDestroy(threadList * threads)
{
	// Start at the front of the list, and maintain track of the next node.
	node * current = threads -> front;
	node * next;

	// Traverse the list beginning to end and remove each node.
	while (current != NULL)
	{
		// Save the current node's next.
		next = current -> next;
		// Free the allocated node.
		free(current);
		// Update current to the former next.
		current = next;
	}

	// Set front to be NULL.
	threads -> front = NULL;
}

/* [PTHREAD] Create a new thread. */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void * (* function)(void *), void * arg)
{	
	//printf("Creating thread #%d with ID %u...\n", threadsCreated + 1, threadsCreated + 1);

	// Set the new thread's ID.
	*(thread) = threadsCreated + 1;

	// Create the Thread Control Block for the new thread.
	tcb * newControlBlock = (tcb *) malloc(sizeof(tcb));  // Allocate space for the new thread's TCB.
	newControlBlock -> threadID = threadsCreated + 1;  // Use threadsCreated + 1 for the threadID.
	newControlBlock -> timeQuantumsPassed = 0;  // To start, no time quantums have passed.
	newControlBlock -> threadStatus = READY;  // Thread is ready to be scheduled into the runqueue.

	// Create and initialize the context of the new thread.
	ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t));  // Allocate space for the new context.
	newControlBlock -> threadContext = newContext;  // Add the new context to the new thread's TCB.

	// Allocate space on stack for this thread to run.
	getcontext(&(*newContext));  // Get the current context as a base for the new context.
	stack_t * newContextStack = (stack_t *) malloc(sizeof(stack_t));  // Allocate space for the new stack pointer.
	newContextStack -> ss_sp = malloc(STACK_SIZE);  // Allocate space for the new stack.
	newContextStack -> ss_size = STACK_SIZE;  // Set the stack size.
	newContext -> uc_stack = *newContextStack;  // Add the new stack to the new context.
	newControlBlock -> threadStack = newContextStack;  // Add the new stack to the new thread's TCB.

	// If this is the first thread being created, do some initialization.
	if (__atomic_test_and_set((void *) &(isFirst), 0) == 0)
	{
		threadRunqueue = threadListCreate();  // Create the runqueue.
		allThreads = threadListCreate();  // Create the list of all threads.

		// Create an extra thread for the main program.
		tcb * mainControlBlock = (tcb *) malloc(sizeof(tcb));  // Allocate space for the main thread's TCB.
		mainControlBlock -> threadID = 0;  // The main thread will be given by ID = 0.
		mainControlBlock -> timeQuantumsPassed = 0;  // To start, no time quantums have passed.
		mainControlBlock -> threadStatus = READY;  // Thread is ready to be scheduled into the runqueue.
		mainControlBlock -> threadContext = &mainContext;  // Reference the address of global mainContext for this thread's context.
		mainControlBlock -> threadStack = &(mainControlBlock -> threadContext -> uc_stack);  // Reference the main context's stack.

		// Create a context for the scheduler.
		getcontext(&schedulerContext);  // Get the current context as a base for the scheduler context.
		schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);  // Allocate space for the scheduler's stack.
		schedulerContext.uc_stack.ss_size = STACK_SIZE;  // Set the stack size.
		schedulerContext.uc_link = &mainContext;  // Set the context to resume on termination to be the main context.
		makecontext(&schedulerContext, schedule, 0);  // Make the scheduler context with the schedule() function.

		threadListAdd(allThreads, mainControlBlock);  // Add the main thread to the list of all threads.
		allThreadsCount++;  // Increment the size of allThreads.
		runningThread = mainControlBlock;  // Set the currently running thread to be the main thread.

		timerSetup();  // Initialize the timer and interrupt handler.
	}

	// After everything is all set, push this thread.
	threadListAdd(allThreads, newControlBlock);  // Add the new thread to the list of all threads.
	allThreadsCount++;  // Increment the size of allThreads.

	newContext -> uc_link = &mainContext;  // Now that we can be sure a mainContext exists, set the context to resume on termination to be the main context.
	makecontext(newContext, (void (*)()) function, 1, arg);  // Make the new thread's context with its assigned function.

	//printf("Thread %u created successfully.\n", *(thread));
	threadsCreated++;  // Increment the number of created threads.

	getcontext(&mainContext);  // Retrieve the main context right before returning so that it can be swapped into without side effects.

    return 0;  // Successful creation.
};

/* [UNUSED][PTHREAD] Give CPU possession to other user-level threads voluntarily. */
int mypthread_yield()
{
	// Change thread state from Running to Ready, and naively increment timeQuantumsPassed for this thread.
	runningThread -> threadStatus = READY;
	runningThread -> timeQuantumsPassed++;

	// Save context of this thread to its thread control block (taken care of by swap).
	// Switch from thread context to scheduler context.
	swapcontext(runningThread -> threadContext, &schedulerContext);

	return 0;
};

/* [PTHREAD] Terminate a thread. */
void mypthread_exit(void * value_ptr)
{
	// If value_ptr is not NULL, save the return value. (If NULL, we'll just save a NULL value.)
	runningThread -> returnValuePtr = value_ptr;
	
	// If a thread was waiting to join on this runningThread, unblock the waiting thread.
	node * current = allThreads -> front;
	while (current != NULL)
	{
		// If current -> threadControlBlock is the waiting thread, unblock it.
		if (runningThread -> threadWaitingToJoin == current -> threadControlBlock -> threadID)
		{
			current -> threadControlBlock -> threadStatus = READY;  // Thread is ready to be scheduled back into the runqueue.
			// if (current -> threadControlBlock -> threadID != 0)
			// 	printf("On exit, thread %u unblocked thread %u which was waiting to join.\n", runningThread -> threadID, current -> threadControlBlock -> threadID);
			// else
			// 	printf("On exit, thread %u unblocked main thread which was waiting to join.\n", runningThread -> threadID);

			break;  // We can stop looking, since we assume only one thread will be waiting on another.
		}
		current = current -> next;
	}

	// if (runningThread -> threadID != 0)
	// 	printf("Thread %u exiting.\n", runningThread -> threadID);
	// else
	// 	printf("Main thread exiting.\n");

	// Deallocate any dynamic memory created when starting this thread.
	runningThread -> threadStatus = EXITED;  // Thread should NOT be scheduled back into the runqueue.
	runningThread -> timeQuantumsPassed++;  // Naively increment timeQuantumsPassed for this thread.
	swapcontext(runningThread -> threadContext, &schedulerContext);  // Yield to the scheduler.
};

/* [PTHREAD] Wait for thread termination. */
int mypthread_join(mypthread_t thread, void ** value_ptr)
{
	// if (runningThread -> threadID != 0)
	// 	printf("Thread %u attempting to join on thread %u.\n", runningThread -> threadID, thread);
	// else
	// 	printf("Main thread attempting to join on thread %u.\n", thread);

	// Wait for a specific thread to terminate.
	// Look for the thread in the threadList.
	tcb * controlBlock = threadListSearch(allThreads, thread);

	// If the given thread is not in the threadList, it must have already terminated.
	if (controlBlock == NULL)
	{
		// Return without doing anything.
		return 0;
	}

	// If the given thread to join on has not exited, block this thread until the given thread terminates.
	if (controlBlock -> threadStatus != EXITED)
	{
		runningThread -> threadStatus = BLOCKED;  // Thread should NOT be scheduled.
		controlBlock -> threadWaitingToJoin = runningThread -> threadID;  // Make the given thread keep track of the thread waiting on it.

		runningThread -> timeQuantumsPassed++;  // Naively increment timeQuantumsPassed for this thread.

		// if (runningThread -> threadID != 0)
		// 	printf("Thread %u blocked attempting to join on thread %u.\n", runningThread -> threadID, thread);
		// else
		// 	printf("Main thread blocked attempting to join on thread %u.\n", thread);

		swapcontext(runningThread -> threadContext, &schedulerContext);  // Yield to the scheduler.
	}

	// if (runningThread -> threadID != 0)
	// 	printf("Thread %u done waiting to join on thread %u.\n", runningThread -> threadID, thread);
	// else
	// 	printf("Main thread done waiting to join on thread %u.\n", thread);

	// If value_ptr is not NULL, pass back the return value of the exiting thread.
	if (value_ptr != NULL)
	{
		*(value_ptr) = controlBlock -> returnValuePtr;
	}

	// Free any dynamic memory created by the joining thread.
	threadListRemove(allThreads, controlBlock);  // Remove the completed thread from the list of all threads.
	free(controlBlock -> threadStack -> ss_sp);  // Free the completed thread's stack.
	free(controlBlock -> threadStack);  // Free the completed thread's stack pointer.
	free(controlBlock -> threadContext);  // Free the completed thread's context.
	free(controlBlock);  // Free the completed thread's TCB.
	allThreadsCount--;  // Decrement the size of allThreads.

	return 0;  // Successful join.
};

/* [PTHREAD] Initialize the mutex lock. */
int mypthread_mutex_init(mypthread_mutex_t * mutex, const pthread_mutexattr_t * mutexattr)
{
	// Make sure the mutex is unlocked upon creation.
	mutex -> isLocked = 0;

	// Initialize data structures for this mutex.
	// Initialize a list for any threads that may be waiting on this mutex.
	mutex -> waitingThreads = threadListCreate();

	return 0;  // Successful init.
};

/* [PTHREAD] Aquire the mutex lock. */
int mypthread_mutex_lock(mypthread_mutex_t * mutex) {
        // Use the built-in test-and-set atomic function to test the mutex.
        // If the mutex is acquired successfully, enter the critical section (do not block).
        // If acquiring mutex fails, block on current thread and context switch to the scheduler thread.
		
		// If the mutex is free, acquire it.
		if (__atomic_test_and_set((void *) &(mutex -> isLocked), 0) == 0)
		{
			mutex -> lockingThread = runningThread;

			// if (runningThread -> threadID != 0)
			// 	printf("Thread %u acquired mutex %p.\n", runningThread -> threadID, mutex);
			// else
			// 	printf("Main thread acquired mutex %p.\n", mutex);

			return 0;  // Successful lock.
		}
		// Otherwise, block this thread and move to the scheduler.
		else
		{
			runningThread -> threadStatus = BLOCKED;  // Thread should NOT be scheduled.
			threadListRemove(threadRunqueue, runningThread);  // Remove this thread from the runqueue.
			threadListAdd(mutex -> waitingThreads, runningThread);  // Add this thread to the list of threads waiting on this mutex.

			// if (runningThread -> threadID != 0)
			// 	printf("Thread %u blocked while acquiring mutex %p.\n", runningThread -> threadID, mutex);
			// else
			// 	printf("Main thread blocked while acquiring mutex %p.\n", mutex);

			runningThread -> timeQuantumsPassed++;  // Naively increment timeQuantumsPassed for this thread.
			swapcontext(runningThread -> threadContext, &schedulerContext);  // Yield to the scheduler.

			// Make sure thread tries to acquire the lock again.
			return mypthread_mutex_lock(mutex);
		}
};

/* [PTHREAD] Release the mutex lock. */
int mypthread_mutex_unlock(mypthread_mutex_t * mutex)
{
	// Release mutex and make it available again.
	// Set threads in mutex's block list back to READY state so that they can compete for this mutex later.

	mutex -> isLocked = 0;  // Unlock the mutex.
	mutex -> lockingThread = NULL;  // Make sure no thread is marked as holding onto the mutex.

	// if (runningThread -> threadID != 0)
	// 	printf("Thread %u unlocked mutex %p.\n", runningThread -> threadID, mutex);
	// else
	// 	printf("Main thread unlocked mutex %p.\n", mutex);

	// Fix all blocked threads back to READY state.
	node * current = mutex -> waitingThreads -> front;
	while (current != NULL)
	{
		//printf("Thread %u unblocked thread %u.\n", runningThread -> threadID, current -> threadControlBlock -> threadID);

		current -> threadControlBlock -> threadStatus = READY;  // Thread is ready to be scheduled back into the runqueue.
		current = current -> next;
	}

	// Remove all threads from the mutex's block list.
	threadListDestroy(mutex -> waitingThreads);

	return 0;  // Successful unlock.
};


/* [PTHREAD] Destroy the mutex. */
int mypthread_mutex_destroy(mypthread_mutex_t * mutex)
{
	// Free dynamic memory created in mypthread_mutex_init.
	// Remove all threads from the mutex's block list. (Should already be empty.)
	threadListDestroy(mutex -> waitingThreads);
	free(mutex -> waitingThreads);  // Free the mutex's list of waiting threads.
	return 0;
};

/* [SCHEDULER] Invoked on timer interrupt; swaps into the scheduler context. */
void timeSigHandler(int sigNum)
{
	// Swap out of the running thread's context and into the scheduler context.
	if (runningThread != NULL)
	{
		//printf("Quantum passed, swapping into scheduler...\n");

		runningThread -> timeQuantumsPassed++;  // Increment time quantums elapsed for running thread.
		swapcontext(runningThread -> threadContext, &schedulerContext);  // Yield to the scheduler.
	}
}

/* [SCHEDULER] Invoked on first call to pthread_create(); sets up the timer and defines the timer interrupt action. */
void timerSetup()
{
	// Create the signal handler for the timer interrupt signal, SIGPROF.
	timerAction.sa_handler = timeSigHandler;
	sigemptyset(&timerAction.sa_mask);
	timerAction.sa_flags = 0;

	sigaction(SIGPROF, &timerAction, NULL);  // Define the action upon interrupt.

	// Set the timer's value to count down from (QUANTUM given in milliseconds).
	timer.it_interval.tv_sec = QUANTUM / 1000;
	timer.it_interval.tv_usec = (QUANTUM * 1000) % 1000000;

	// Start the timer by setting the it_value equal to the interval.
	timer.it_value = timer.it_interval;

	// Set up ITIMER_PROF timer, which will send SIGPROF.
	setitimer(ITIMER_PROF, &timer, NULL);
}

/* [SCHEDULER] Main scheduler; picks scheduling policy to run. */
static void schedule()
{
	// Every time when timer interrupt happens, the thread library should be context switched
	// from thread context to this schedule function.

	// Freeze the timer. (Zero out the value).
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_PROF, &timer, NULL);

	// If there are no remaining threads except main in allThreads, clean up the library.
	if (allThreadsCount == 1 && allThreads -> front -> threadControlBlock -> threadID == 0)
	{
		threadListDestroy(allThreads);  // Clean up allThreads.
		threadListDestroy(threadRunqueue);  // Clean up runqueue.
		free(allThreads);  // Free allocated space for allThreads.
		free(threadRunqueue);  // Free allocated space for runqueue.

		free(schedulerContext.uc_stack.ss_sp);  // Free allocated space for the scheduler context's stack.

		ucontext_t goToContext = *(runningThread -> threadContext);  // Save the main thread's context.
		
		allThreadsCount = 0;  // Reset allThreadsCount.
		threadsCreated = 0;  // Reset threadsCreated.
		isFirst = 0;  // Reset isFirst.

		setcontext(&goToContext);  // Go back to the main context for the last time.
	}

	// Invoke different actual scheduling algorithms according to policy (STCF or MLFQ).
	// Only STCF implemented.
	#ifndef MLFQ
		// Choose STCF
		sched_stcf();
	#else
		// Choose MLFQ
		sched_mlfq();
	#endif

	// Un-freeze the timer. (Set value to interval.)
	timer.it_value = timer.it_interval;
	setitimer(ITIMER_PROF, &timer, NULL);

	// Context switch into the next thread to run.
	if (runningThread != NULL)
	{
		setcontext(runningThread -> threadContext);
	}
	else
	{
		//printf("No running thread.\n");
		return;
	}
}

/* [SCHEDULER] Preemptive SJF (STCF) scheduling algorithm. */
static void sched_stcf()
{
	// Choose the thread with the lowest number of elapsed time quantums.
	
	node * current;
	
	// Add any threads that are "ready" to the runqueue.
	current = allThreads -> front;
	while (current != NULL)
	{
		// If this thread is READY to be scheduled, add it to the runqueue.
		if (current -> threadControlBlock -> threadStatus == READY)
		{
			threadListAdd(threadRunqueue, current -> threadControlBlock);  // Add the thread to the runqueue.
			current -> threadControlBlock -> threadStatus = SCHEDULED;  // This thread is in the runqueue.
		}
		current = current -> next;
	}

	// If a thread was previously running and was not blocking or exiting, add it back to the runqueue.
	if (runningThread != NULL)
	{
		if (runningThread -> threadStatus != BLOCKED && runningThread -> threadStatus != EXITED)
		{
			threadListAdd(threadRunqueue, runningThread);  // Add the thread to the runqueue.
			runningThread -> threadStatus = SCHEDULED;  // his thread is in the runqueue.
		}
	}

	current = threadRunqueue -> front;  // Reset current to the front of the runqueue.

	// If runqueue is empty, do nothing and stay in current thread.
	if (current == NULL)
	{
		return;
	}

	// Find the thread with the least number of elapsed time quantums.
	int minTimeQuantums = current -> threadControlBlock -> timeQuantumsPassed;  // Start with assuming the first thread has the min.
	tcb * minTimeQuantumTCB = current -> threadControlBlock;  // Keep track of TCB with min.
	while (current != NULL)
	{
		// If current thread has a new min for quantums passed:
		if (current -> threadControlBlock -> timeQuantumsPassed < minTimeQuantums)
		{
			// If current thread is in the runqueue, keep track of it.
			if (current -> threadControlBlock -> threadStatus == SCHEDULED)
			{
				minTimeQuantums = current -> threadControlBlock -> timeQuantumsPassed;
				minTimeQuantumTCB = current -> threadControlBlock;
			}
		}
		current = current -> next;
	}

	runningThread = minTimeQuantumTCB;  // Set the running thread to be the thread with the minimum time quantums passed.
	threadListRemove(threadRunqueue, runningThread);  // Remove this thread from the runqueue.

	// if (runningThread -> threadID != 0)
	// 	printf("Scheduler picked thread %u.\n", runningThread -> threadID);
	// else
	// 	printf("Scheduler picked main thread.\n");
}

/* [SCHEDULER] Preemptive MLFQ scheduling algorithm. */
static void sched_mlfq() 
{
	// Not implemented.
}