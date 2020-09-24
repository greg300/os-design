#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_FORKS 5000

int main(int argc, char *argv[])
{
	// Define the timeval structs to store the start and end times, as well as the difference between them.
	struct timeval start, end, diff;

	// Start timing.
    gettimeofday(&start, NULL);

	// Make NUM_SYS_CALLS calls to fork() and wait().
	int i;
	for (i = 0; i < NUM_FORKS; i++)
	{
		pid_t result = fork();
		// If child, just exit.
		if (result == 0)
		{
			exit(0);
		}
		// If parent, wait on child.
		else
		{
			waitpid(result, NULL, 0);
		}
	}
	
	// Stop timing.
	gettimeofday(&end, NULL);

	// Calculate the difference.
    timersub(&end, &start, &diff);

	// Determine the results.
	double total = diff.tv_sec * 1000000.0 + diff.tv_usec;
	double average = total / NUM_FORKS;

	// Output the results.
	printf("Forks Performed: %d\n", NUM_FORKS);
	printf("Total Elapsed Time: %8.5f microseconds\n", total);
	printf("Average Time Per Fork: %8.5f microseconds\n", average);

	return 0;
}
