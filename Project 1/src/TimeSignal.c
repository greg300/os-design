#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_SIGNALS 100000

// Keep track of the number of exceptions raised.
int exceptions = 0;
// Define the timeval structs to store the start and end times, as well as the difference between them.
struct timeval start, end, diff;

void handle_sigfpe(int signum)
{
	exceptions++;
	// Check to see if we should stop sending the signal.
	if (exceptions == NUM_SIGNALS)
	{
		// Stop timing.
		gettimeofday(&end, NULL);

		// Calculate the difference.
		timersub(&end, &start, &diff);

		// Determine the results.
		double total = diff.tv_sec * 1000000.0 + diff.tv_usec;
		double average = total / NUM_SIGNALS;

		// Output the results.
		printf("Exceptions Occurred: %d\n", NUM_SIGNALS);
		printf("Total Elapsed Time: %8.5f microseconds\n", total);
		printf("Average Time Per Exception: %8.5f microseconds\n", average);

		exit(0);
	}
}

int main(int argc, char *argv[])
{
	// Set up the signal handler.
	signal(SIGFPE, handle_sigfpe);
	
	// Start timing.
    gettimeofday(&start, NULL);

	// Divide by zero.
	int x = 7, y = 0, z;
	z = x / y;  // Uh oh!
	
	return 0;
}
