#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_SYS_CALLS 100000

int main(int argc, char *argv[])
{
	// Define the timeval structs to store the start and end times, as well as the difference between them.
	struct timeval start, end, diff;

	// Start timing.
    gettimeofday(&start, NULL);

	// Make NUM_SYS_CALLS calls to getpid().
	int i;
	for (i = 0; i < NUM_SYS_CALLS; i++)
	{
		getpid();
	}
	
	// Stop timing.
	gettimeofday(&end, NULL);

	// Calculate the difference.
    timersub(&end, &start, &diff);

	// Determine the results.
	double total = diff.tv_sec * 1000000.0 + diff.tv_usec;
	double average = total / NUM_SYS_CALLS;

	// Output the results.
	printf("Syscalls Performed: %d\n", NUM_SYS_CALLS);
	printf("Total Elapsed Time: %8.5f microseconds\n", total);
	printf("Average Time Per Syscall: %8.5f microseconds\n", average);

	return 0;
}
