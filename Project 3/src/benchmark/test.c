#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "../my_vm.h"

#define SIZE 50
#define ALLOC 10000

int main() {

	// Define the timeval structs to store the start and end times, as well as the difference between them.
	struct timeval start, end, diff;

	// Start timing.
    gettimeofday(&start, NULL);
    
    printf("Allocating three arrays of %d bytes\n", ALLOC);
    void *a = myalloc(ALLOC);
    int old_a = (int)a;
    void *b = myalloc(ALLOC);
    void *c = myalloc(ALLOC);
    int x = 1;
    int y, z;
    int i =0, j=0;
    int address_a = 0, address_b = 0;
    int address_c = 0;

    printf("Addresses of the allocations: %x, %x, %x\n", (int)a, (int)b, (int)c);

    printf("Storing integers to generate a SIZExSIZE matrix\n");
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            PutVal((void *)address_a, &x, sizeof(int));
            PutVal((void *)address_b, &x, sizeof(int));
        }
    }

    printf("Fetching matrix elements stored in the arrays\n");

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            GetVal((void *)address_a, &y, sizeof(int));
            GetVal( (void *)address_b, &z, sizeof(int));
            printf("%d ", y);
            if (y == 0)
            {
                printf("%x\n", (int) address_a);
            }
        }
        printf("\n");
    }

    printf("Performing matrix multiplication with itself!\n");
    MatMult(a, b, SIZE, c);


    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_c = (unsigned int)c + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            GetVal((void *)address_c, &y, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }
    printf("Freeing the allocations!\n");
    myfree(a, ALLOC);
    myfree(b, ALLOC);
    myfree(c, ALLOC);

    printf("Checking if allocations were freed!\n");
    a = myalloc(ALLOC);
    if ((int)a == old_a)
        printf("free function works\n");
    else
        printf("free function does not work\n");

    // Stop timing.
	gettimeofday(&end, NULL);

	// Calculate the difference.
    timersub(&end, &start, &diff);

	// Determine the results.
	double total = diff.tv_sec * 1000000.0 + diff.tv_usec;

	// Output the results.
	printf("Total Elapsed Time: %8.5f microseconds\n", total);

    print_TLB_missrate();

    return 0;
}
