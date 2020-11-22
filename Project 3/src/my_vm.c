#include "my_vm.h"

int isFirst = 0;  				        // For an atomic check as to whether myalloc() is being called for the first time.
int numVirtualPages;                    // The number of virtual pages needed (given by MAX_MEMSIZE / PGSIZE).
int numPhysicalPages;                   // The number of physical pages needed (given by MEMSIZE / PGSIZE).
int numPageBitsOuter;                   // The number of bits to index into the outer Page Directory.
int numPageBitsInner;                   // The number of bits to index into an inner Page Table.
int numPageBitsOffset;                  // The number of bits needed to represent the physical offset.
int numPageDirectoryEntries;            // The number of pde_t's per page directory (given by 2^numPageBitsOuter).
int numPageTableEntries;                // The number of pte_t's per page table (given by 2^numPageBitsInner).
char *virtualBitmap;                    // Array of chars, where each char represents a bit, and each bit is marked as 0 (free) or 1 (in use) for virtual memory.
char *physicalBitmap;                   // Array of chars, where each char represents a bit, and each bit is marked as 0 (free) or 1 (in use) for physical memory.
pde_t *pageDirectory;                   // The Page Directory; each entry is a pointer to a Page Table.
void *physicalMemory;                   // The physical memory being simulated (size given by MEMSIZE).
pthread_mutex_t pageDirectoryLock;      // Lock for the Page Directory.
pthread_mutex_t physicalMemoryLock;     // Lock for Physical Memory.
pthread_mutex_t virtualBitmapLock;      // Lock for the Virtual Bitmap.
pthread_mutex_t physicalBitmapLock;     // Lock for the Physical Bitmap.


void printBits(uint32_t x)
{
    unsigned int size = sizeof(uint32_t);
    unsigned int maxPow = 1 << (size * 8 - 1);
    int i;
    for(i = 0; i < size * 8; ++i)
    {
        printf("%u", x & maxPow ? 1 : 0);
        x = x << 1;
    }
    printf("\n");   
}


uint32_t createBitMask(int start, int end)
{
    uint32_t result = 0;
    int i;
    for (i = start; i <= end; i++)
    {
        result |= 1 << (32 - i);
    }

    return result;
}


uint32_t getBits(void *addr, int start, int end)
{
    uint32_t result;
    uint32_t mask = createBitMask(start, end);

    result = ((uint32_t) addr) & mask;
    result >>= (32 - end);

    return result;
}


uint32_t extractOuterBits(void *addr)
{
    int start = 1;
    int end = numPageBitsOuter;
    return getBits(addr, start, end);
}


uint32_t extractInnerBits(void *addr)
{
    int start = numPageBitsOuter + 1;
    int end = start + numPageBitsInner;
    return getBits(addr, start, end);
}


uint32_t extractOffsetBits(void *addr)
{
    int start = numPageBitsOuter + numPageBitsInner + 2;
    int end = start + numPageBitsOffset;
    return getBits(addr, start, end);
}


pde_t indexPageDirectory(pde_t *pgdir, uint32_t outerIndex)
{
    return pgdir[outerIndex];
}


pte_t indexPageTable(pde_t pageTable, uint32_t innerIndex)
{
    return pageTable[innerIndex];
}


/*
Function responsible for allocating and setting your physical memory.
*/
void SetPhysicalMem()
{
    /* Allocate physical memory using mmap or malloc; this is the total size of
    your memory you are simulating. */

    // HINT: Also calculate the number of physical and virtual pages and allocate
    // virtual and physical bitmaps and initialize them.
    int i;

    // Initialize mutexes.
    pthread_mutex_init(&physicalMemoryLock, NULL);
    pthread_mutex_init(&physicalBitmapLock, NULL);
    pthread_mutex_init(&virtualBitmapLock, NULL);
    pthread_mutex_init(&pageDirectoryLock, NULL);

    // Lock all mutexes until their part of the initialization process is complete.
    pthread_mutex_lock(&physicalMemoryLock);
    pthread_mutex_lock(&physicalBitmapLock);
    pthread_mutex_lock(&virtualBitmapLock);
    pthread_mutex_lock(&pageDirectoryLock);

    // 1. malloc MEMSIZE memory.
    physicalMemory = malloc(MEMSIZE);
    pthread_mutex_unlock(&physicalMemoryLock);

    // 2. Calculate number of virtual and physical pages.
    // # of physical pages = MEMSIZE / size of single page (PGSIZE).
    numPhysicalPages = MEMSIZE / PGSIZE;
    // # of virtual pages = MAX_MEMSIZE / size of a single page (PGSIZE).
    numVirtualPages = MAX_MEMSIZE / PGSIZE;

    // 3. Initialize physical and virtual bitmaps.
    // Phsyical bitmap would be numPhysicalPages size.
    physicalBitmap = (char *) malloc(numPhysicalPages * sizeof(char));
    for (i = 0; i < numPhysicalPages; i++)
        physicalBitmap[i] = 0;
    pthread_mutex_unlock(&physicalBitmapLock);

    // Virtual bitmap wold be numVirtualPages size.
    virtualBitmap = (char *) malloc(numVirtualPages * sizeof(char));
    for (i = 0; i < numVirtualPages; i++)
        virtualBitmap[i] = 0;
    pthread_mutex_unlock(&virtualBitmapLock);

    // 4. Initialize page directory (array of pde_t pointers).
    // Number of bits needed for offset is given by log base 2 of PGSIZE.
    numPageBitsOffset = (int) log2(PGSIZE);
    // If the offset is even, VPN bits are even and can be evenly split.
    if (numPageBitsOffset % 2 == 0)
    {
        numPageBitsOuter = (32 - numPageBitsOffset) / 2;
        numPageBitsInner = (32 - numPageBitsOffset) / 2;
    }
    // If the offset is odd, VPN bits are odd and cannot be evenly split.
    else
    {
        numPageBitsOuter = (32 - numPageBitsOffset) / 2 + 1;
        numPageBitsInner = (32 - numPageBitsOffset) / 2;
    }
    
    // The number of pde_t's per page directory is given by 2^numPageBitsOuter.
    numPageDirectoryEntries = exp2(numPageBitsOuter);
    // The number of pte_t's per page table is given by 2^numPageBitsInner.
    numPageTableEntries = exp2(numPageBitsInner);

    pageDirectory = (pde_t *) malloc(numPageDirectoryEntries * sizeof(pde_t));
    for (i = 0; i < numPageDirectoryEntries; i++)
        pageDirectory[i] = NULL;
    pthread_mutex_unlock(&pageDirectoryLock);
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int add_TLB(void *va, void *pa)
{
    /* Part 2 HINT: Add a virtual to physical page translation to the TLB. */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t * check_TLB(void *va)
{
    /* Part 2: TLB lookup code here. */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate()
{
    double miss_rate = 0;

    /* Part 2: Code here to calculate and print the TLB miss rate. */




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address.
*/
pte_t *Translate(pde_t *pgdir, void *va)
{
    /* HINT: Get the Page directory index (1st level) Then get the
    2nd-level-page table index using the virtual address. Using the page
    directory index and page table index get the physical address. */

    // Extract all necessary bits from the address va.
    uint32_t outerIndex = extractOuterBits(va);
    uint32_t innerIndex = extractInnerBits(va);
    uint32_t offset = extractOffsetBits(va);

    // Index into the Page Directory (1st level) to find the corresponding Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pde_t pageTable = indexPageDirectory(pgdir, outerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Directory does not yet exist, translation fails.
    if (pageTable == NULL)
    {
        return NULL;
    }

    // Index into the found Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pte_t page = indexPageTable(pageTable, innerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Table does not yet exist, translation fails.
    if (page == NULL)
    {
        return NULL;
    }

    // physical address = address of physical page + offset.
    pte_t *physicalAddress = (pte_t *) page + offset;

    return physicalAddress;
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added.
*/
int PageMap(pde_t *pgdir, void *va, void *pa)
{
    /* HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping. */

    // Extract all necessary bits from the address va.
    uint32_t outerIndex = extractOuterBits(va);
    uint32_t innerIndex = extractInnerBits(va);
    uint32_t offset = extractOffsetBits(va);

    // Index into the Page Directory (1st level) to find the corresponding Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pde_t pageTable = indexPageDirectory(pgdir, outerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Directory does not yet exist, create an entry.
    if (pageTable == NULL)
    {
        // Allocate memory for a new Page Table, and save it to the Page Directory.
        pthread_mutex_lock(&pageDirectoryLock);
        pgdir[outerIndex] = (pde_t) malloc(numPageTableEntries * sizeof(pte_t));
        pageTable = pgdir[outerIndex];
        pthread_mutex_unlock(&pageDirectoryLock);
    }

    // Index into the found or created Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pte_t page = indexPageTable(pageTable, innerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Table does not yet exist, create an entry.
    if (page == NULL)
    {
        // Set the mapping at the indexed virtual address to be the corresponding physical address.
        pthread_mutex_lock(&pageDirectoryLock);
        pageTable[innerIndex] = pa;
        page = pageTable[innerIndex];
        pthread_mutex_unlock(&pageDirectoryLock);
        
        // Update the virtual bitmap.
        // pthread_mutex_lock(&virtualBitmapLock);
        // virtualBitmap[outerIndex * numPageTableEntries + innerIndex] = 1;
        // pthread_mutex_unlock(&virtualBitmapLock);
    }

    return 1;
}


/*
Function that gets the next available virtual page.
*/
void *get_next_avail_virt(int numPages)
{
    /* Use virtual address bitmap to find the next free page. */
    unsigned long long i, j;
    unsigned long long startIndex = 0;
    int foundPages = 0;
    pthread_mutex_lock(&virtualBitmapLock);
    for (i = 1; i < numVirtualPages; i++)  // Ignore first entry, to reserve 0x0 for NULL.
    {
        // If the page at index i is free, count it, save the index, and look for contiguous free pages, if needed.
        if (virtualBitmap[i] == 0)
        {
            foundPages++;
            startIndex = i;
            for (j = 0; j < numVirtualPages && foundPages < numPages; j++)
            {
                if (virtualBitmap[j] == 0)
                {
                    foundPages++;
                }
                else
                {
                    break;
                }
                
            }
        }
        // If the needed number of free contiguous pages have been found, stop looking.
        if (foundPages == numPages)
        {
            break;
        }
        // If the needed number of free contigious pages have not yet been found, reset the found count to 0 and keep looking from j.
        else
        {
            foundPages = 0;
            i = j;
        }
    }
    
    // If the needed number of free contiguous pages has not been found at all, return failure.
    if (foundPages < numPages)
    {
        pthread_mutex_unlock(&virtualBitmapLock);
        return NULL;
    }

    // Mark chosen pages as used in the Virtual Bitmap.
    for (i = 0; i < foundPages; i++)
    {
        virtualBitmap[startIndex + i] = 1;
    }

    pthread_mutex_unlock(&virtualBitmapLock);

    // Return the virtual address of the first page.
    // virtual address = index of bit * PGSIZE.
    return startIndex * PGSIZE;

    // Reserve 0x0000 for NULL
    // 0x1000 – start

    // 1 * 4kb = 0x1000
    // 6 * 4kb = 0x6000
}


/*
Function that gets the next available physical page.
*/
void *get_next_avail_phys()
{
    /* Use physical address bitmap to find the next free page. */
    unsigned long long i;
    pthread_mutex_lock(&physicalBitmapLock);
    for (i = 0; i < numPhysicalPages; i++)
    {
        // If this physical page is available, return its address.
        if (physicalBitmap[i] == 0)
        {
            physicalBitmap[i] = 1;
            // physical address = index of bit * pagesize + offset of start of physical memory.
            pthread_mutex_unlock(&physicalBitmapLock);
            return i * PGSIZE + (unsigned long long) physicalMemory;
        }
    }
    
    // Here, no physical address was found to be available.
    pthread_mutex_unlock(&physicalBitmapLock);
    return NULL;
}


/*
Function responsible for allocating pages
and used by the benchmark.
*/
void *myalloc(unsigned int num_bytes)
{
    /* HINT: If the physical memory is not yet initialized, then allocate and initialize. */

    /* HINT: If the page directory is not initialized, then initialize the
    page directory. Next, using get_next_avail(), check if there are free pages. If
    free pages are available, set the bitmaps and map a new page. Note, you will
    have to mark which physical pages are used. */

    // Upon first call, initialize library.
    // If this is the first call to myalloc, do some initialization.
	if (__atomic_test_and_set((void *) &(isFirst), 0) == 0)
    {
        SetPhysicalMem();
    }

    // Calculate the number of pages needed.
    int numPages = (int) ceil((double) num_bytes / PGSIZE);

    // Check for free physical pages.
    
    // Check for free virtual pages.

    // Map the virtual addresses to physical addresses in the Page Directory.

    // Update the bitmaps.

    // If no free virtual pages, "free" marked physical pages in Physical Bitmap.

    return NULL;
}

/*
Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size)
{
    /* Free the page table entries starting from this virtual address (va).
    Also mark the pages free in the bitmap.
    Only free if the memory from "va" to va+size is valid. */

    // Assume that myfree will be used correctly.
}


/*
The function copies data pointed by "val" to physical
memory pages using virtual address (va).
*/
void PutVal(void *va, void *val, int size)
{
    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function. */

    // Use memcpy

}


/*
Given a virtual address, this function copies the contents of the page to val
*/
void GetVal(void *va, void *val, int size)
{
    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */
}


/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer)
{
    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/
}
