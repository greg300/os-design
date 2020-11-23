#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

// Assume the address space is 32 bits, so the max memory size is 4GB.
// Page size is 4KB.

// Add any important includes here which you may need.

#define PGSIZE 4096

// Maximum size of your memory.
#define MAX_MEMSIZE 4ULL*1024*1024*1024  // 4GB.

// Size of physical memory to malloc.
#define MEMSIZE 1024*1024*1024

// Represents a page table entry.
typedef void* pte_t;

// Represents a page directory entry.
typedef pte_t* pde_t;

// Number of entries in the TLB.
#define TLB_SIZE 120

// Structure representing an entry in the TLB.
typedef struct _tlbEntry {
    void *va;
    void *pa;
} tlbEntry;

// Linked List node, representing a TLB entry.
typedef struct node {
	tlbEntry *entry;	        // The TLB entry at this node.
	struct node *next;			// Reference to the next node in the Linked List.
} node;

// Structure to represent the TLB, implemented as a Queue for FIFO eviction.
typedef struct _tlb {
    // Assume your TLB is a direct mapped TLB of TLB_SIZE (entries).
    // You must also define wth TLB_SIZE in this file.
    // Assume each bucket to be 4 bytes.
    node * front;	// Reference to the first node in the TLB Queue.
	node * rear;    // Reference to the last node in the TLB Queue.
} tlb;

void printBits(uint32_t x);
uint32_t createBitMask(int start, int end);
uint32_t getBits(void *addr, int start, int end);
uint32_t extractOuterBits(void *addr);
uint32_t extractInnerBits(void *addr);
uint32_t extractOffsetBits(void *addr);
pde_t indexPageDirectory(pde_t *pgdir, uint32_t outerIndex);
pte_t indexPageTable(pde_t pageTable, uint32_t innerIndex);
node *newNode(void *va, void *pa);
void tlbCreate();
void tlbAdd(void *va, void *pa);
void tlbEvict();

void SetPhysicalMem();
int add_TLB(void *va, void *pa);
pte_t *check_TLB(void *va);
void print_TLB_missrate();
pte_t *Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void *pa);
void *get_next_avail_virt(int numPages);
int get_next_avail_phys(int numPages, void **physicalPages, int *physicalPageIndices);
void *myalloc(unsigned int num_bytes);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);

#endif
