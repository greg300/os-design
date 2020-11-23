#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "../my_vm.h"

#define SIZE 5
//#define ALLOC 4*1024*1024
#define ALLOC 400
#define THREADS 10

pthread_t *thread;

/* A CPU-bound task to do parallel array addition */
void parallel_func(void* arg) {
	
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

	pthread_exit(NULL);
}

int main() {

    thread = (pthread_t*)malloc(THREADS*sizeof(pthread_t));
    int i;
    for (i = 0; i < THREADS; ++i)
		pthread_create(&thread[i], NULL, (void * (*)(void *)) parallel_func, NULL);

	for (i = 0; i < THREADS; ++i)
		pthread_join(thread[i], NULL);

    print_TLB_missrate();
    free(thread);

    return 0;
}
