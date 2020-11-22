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

    //printBits(result);
    return result;
}


uint32_t getBits(void *addr, int start, int end)
{
    uint32_t result;
    uint32_t mask = createBitMask(start, end);

    result = ((uint32_t) addr) & mask;
    result >>= (32 - end);

    //printBits(result);
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
    int end = start + numPageBitsInner - 1;
    return getBits(addr, start, end);
}


uint32_t extractOffsetBits(void *addr)
{
    int start = numPageBitsOuter + numPageBitsInner + 1;
    int end = start + numPageBitsOffset - 1;
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
    //("Setting Physical Memory.\n");
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
    //("\tAddress of physical memory: %x\n", (int) physicalMemory);

    // 2. Calculate number of virtual and physical pages.
    // # of physical pages = MEMSIZE / size of single page (PGSIZE).
    numPhysicalPages = MEMSIZE / PGSIZE;
    // # of virtual pages = MAX_MEMSIZE / size of a single page (PGSIZE).
    numVirtualPages = MAX_MEMSIZE / PGSIZE;
    //printf("\tNumber of physical pages: %d\n", (int) numPhysicalPages);
    //printf("\tNumber of virtual pages: %d\n", (int) numVirtualPages);

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
    //printf("\tNumber of outer VPN bits: %d\n", (int) numPageBitsOuter);
    //printf("\tNumber of inner VPN bits: %d\n", (int) numPageBitsInner);
    //printf("\tNumber of offset bits: %d\n", (int) numPageBitsOffset);
    
    // The number of pde_t's per page directory is given by 2^numPageBitsOuter.
    numPageDirectoryEntries = exp2(numPageBitsOuter);
    // The number of pte_t's per page table is given by 2^numPageBitsInner.
    numPageTableEntries = exp2(numPageBitsInner);
    //printf("\tNumber of Page Directory entries: %d\n", (int) numPageDirectoryEntries);
    //printf("\tNumber of Page Table entries: %d\n", (int) numPageTableEntries);

    pageDirectory = (pde_t *) malloc(numPageDirectoryEntries * sizeof(pde_t));
    for (i = 0; i < numPageDirectoryEntries; i++)
        pageDirectory[i] = NULL;
    pthread_mutex_unlock(&pageDirectoryLock);

    //printf("Initialization complete.\n");
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

    return NULL;
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
    //printf("Translating virtual address: %x\n", (int) va);

    //printBits((u_int32_t) va);
    //printf("-----------------------------\n");
    // Extract all necessary bits from the address va.
    uint32_t outerIndex = extractOuterBits(va);
    uint32_t innerIndex = extractInnerBits(va);
    uint32_t offset = extractOffsetBits(va);

    //printf("\tOuter index: %d\n", (int) outerIndex);
    //printf("\tInner index: %d\n", (int) innerIndex);
    //printf("\tOffset: %d\n", (int) offset);

    // Index into the Page Directory (1st level) to find the corresponding Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pde_t pageTable = indexPageDirectory(pgdir, outerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Directory does not yet exist, translation fails.
    if (pageTable == NULL)
    {
        //printf("\tReturning NULL: no entry in Page Directory with index %d.\n", outerIndex);
        return NULL;
    }

    // Index into the found Page Table.
    pthread_mutex_lock(&pageDirectoryLock);
    pte_t page = indexPageTable(pageTable, innerIndex);
    pthread_mutex_unlock(&pageDirectoryLock);

    // If the given entry in the Page Table does not yet exist, translation fails.
    if (page == NULL)
    {
        //printf("\tReturning NULL: no entry in Page Table with index %d.\n", innerIndex);
        return NULL;
    }

    // physical address = address of physical page + offset.
    pte_t *physicalAddress = (pte_t *) page + offset;
    //printf("Translated virtual address %x to physical address %x.\n", (int) va, (int) physicalAddress);

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
    //printf("Mapping virtual address %x to physical address %x.\n", (int) va, (int) pa);

    //printBits((u_int32_t) va);
    //printf("-----------------------------\n");
    // Extract all necessary bits from the address va.
    uint32_t outerIndex = extractOuterBits(va);
    uint32_t innerIndex = extractInnerBits(va);
    uint32_t offset = extractOffsetBits(va);

    //printf("\tOuter index: %d\n", (int) outerIndex);
    //printf("\tInner index: %d\n", (int) innerIndex);
    //printf("\tOffset: %d\n", (int) offset);

    // Index into the Page Directory (1st level) to find the corresponding Page Table.
    pde_t pageTable = indexPageDirectory(pgdir, outerIndex);

    // If the given entry in the Page Directory does not yet exist, create an entry.
    if (pageTable == NULL)
    {
        // Allocate memory for a new Page Table, and save it to the Page Directory.
        //printf("\tAllocating new Page Directory entry: no entry in Page Directory with index %d.\n", outerIndex);
        pgdir[outerIndex] = (pde_t) malloc(numPageTableEntries * sizeof(pte_t));
        pageTable = pgdir[outerIndex];
    }

    // Index into the found or created Page Table.
    pte_t page = indexPageTable(pageTable, innerIndex);

    // Set the mapping at the indexed virtual address to be the corresponding physical address.
    pageTable[innerIndex] = pa;
    page = pageTable[innerIndex];

    //printf("Mapped physical address %x to PDE %d, PTE %d.\n", (int) pa, (int) outerIndex, (int) innerIndex);

    // If the given entry in the Page Table does not yet exist, create an entry.
    // if (page == NULL)
    // {
    //     // Set the mapping at the indexed virtual address to be the corresponding physical address.
    //     pageTable[innerIndex] = pa;
    //     page = pageTable[innerIndex];
    // }

    return 1;
}


/*
Function that gets the next available virtual pages.
*/
void *get_next_avail_virt(int numPages)
{
    /* Use virtual address bitmap to find the next free page. */
    //printf("Getting next %d available virtual pages.\n", numPages);

    unsigned long i, j;
    unsigned long startIndex = 0;
    int foundPages = 0;
    
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
            //printf("\tFound %d contiguous virtual pages.\n", numPages);
            break;
        }
        // If the needed number of free contigious pages have not yet been found, reset the found count to 0 and keep looking.
        else
        {
            foundPages = 0;
        }
    }
    
    // If the needed number of free contiguous pages has not been found at all, return failure.
    if (foundPages < numPages)
    {
        //printf("\tReturning NULL: did not find %d contiguous virtual pages.\n", numPages);
        return NULL;
    }

    // Mark chosen pages as used in the Virtual Bitmap.
    for (i = 0; i < foundPages; i++)
    {
        virtualBitmap[startIndex + i] = 1;
    }

    // Return the virtual address of the first page.
    // virtual address = index of bit * PGSIZE.
    //printf("Returning virtual address %x.\n", (int) (startIndex * PGSIZE));
    return (void *) (startIndex * PGSIZE);

    // Reserve 0x0000 for NULL
    // 0x1000 – start

    // 1 * 4kb = 0x1000
    // 6 * 4kb = 0x6000
}


/*
Function that gets the next available physical pages.
*/
int get_next_avail_phys(int numPages, void **physicalPages, int *physicalPageIndices)
{
    /* Use physical address bitmap to find the next free page. */
    //printf("Getting next %d available physical pages.\n", numPages);

    unsigned long i;
    int foundPages = 0;
    for (i = 0; i < numPhysicalPages && foundPages < numPages; i++)
    {
        // If this physical page is available, save its address and index.
        if (physicalBitmap[i] == 0)
        {
            //printf("\tPage #%d found at index %lu.\n", foundPages + 1, i);
            // physical address = index of bit * PGSIZE + offset of start of physical memory.
            physicalPages[foundPages] = (void *) (i * PGSIZE + (unsigned long) physicalMemory);
            physicalPageIndices[foundPages] = i;
            //printf("\t\tAddress of physical page: %x.\n", (int) physicalPages[foundPages]);
            foundPages++;
        }
    }

    //printf("Found %d physical pages.\n", numPages);
    return foundPages;
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

    //printf("Myallocing %u bytes.\n", num_bytes);

    // Upon first call, initialize library.
    // If this is the first call to myalloc, do some initialization.
	if (__atomic_test_and_set((void *) &(isFirst), 0) == 0)
    {
        SetPhysicalMem();
    }

    int i;

    // Calculate the number of pages needed.
    int numPages = (int) ceil((double) num_bytes / PGSIZE);
    //printf("\tNumber of pages: %d.\n", numPages);
    
    // Check for free physical pages.
    void **physicalPages = malloc(numPages * sizeof(void *));
    int *physicalPageIndices = malloc(numPages * sizeof(int));

    pthread_mutex_lock(&physicalBitmapLock);
    int foundPhysicalPages = get_next_avail_phys(numPages, physicalPages, physicalPageIndices);

    // If not enough physical pages are available, return NULL for failure.
    if (foundPhysicalPages < numPages)
    {
        //printf("\tReturning NULL: not enough physical pages. %d needed, %d found.\n", foundPhysicalPages, numPages);
        pthread_mutex_unlock(&physicalBitmapLock);
        free(physicalPages);
        free(physicalPageIndices);
        return NULL;
    }
    
    // Check for free virtual pages.
    pthread_mutex_lock(&virtualBitmapLock);
    void *virtualPages = get_next_avail_virt(numPages);

    // If not enough virtual pages are available, return NULL for failure.
    if (virtualPages == NULL)
    {
        //printf("\tReturning NULL: not enough contiguous virtual pages.\n");
        pthread_mutex_unlock(&physicalBitmapLock);
        pthread_mutex_unlock(&virtualBitmapLock);
        free(physicalPages);
        free(physicalPageIndices);
        return NULL;
    }

    // Map the virtual addresses to physical addresses in the Page Directory.
    //printf("\tMapping virtual addresses to physical addresses for myalloc call.\n");
    pthread_mutex_lock(&pageDirectoryLock);
    for (i = 0; i < numPages; i++)
    {
        PageMap(pageDirectory, virtualPages + i, physicalPages[i]);
    }
    pthread_mutex_unlock(&pageDirectoryLock);

    // Update the Physical Bitmap.
    //printf("\tUpdating the physical bitmap.\n");
    for (i = 0; i < numPages; i++)
    {
        physicalBitmap[physicalPageIndices[i]] = 1;
        //printf("\t\tUpdated physical bitmap at index %d.\n", physicalPageIndices[i]);
    }
    pthread_mutex_unlock(&physicalBitmapLock);

    // Update the Virtual Bitmap.
    //printf("\tUpdating the virtual bitmap.\n");
    for (i = 0; i < numPages; i++)
    {
        virtualBitmap[(unsigned int) virtualPages / PGSIZE] = 1;
        //printf("\t\tUpdated virtual bitmap at index %u.\n", (unsigned int) virtualPages / PGSIZE);
    }
    pthread_mutex_unlock(&virtualBitmapLock);

    free(physicalPages);
    free(physicalPageIndices);

    //printf("Myalloc successful. Returning virtual address %x.\n", (int) virtualPages);

    return virtualPages;
}

/*
Responsible for releasing one or more memory pages using virtual address (va).
*/
void myfree(void *va, int size)
{
    /* Free the page table entries starting from this virtual address (va).
    Also mark the pages free in the bitmap.
    Only free if the memory from "va" to va+size is valid. */

    // Assume that myfree will be used correctly.

    //printf("Myfreeing %u bytes at address %x.\n", size, (int) va);

    // Calculate the number of pages needed to free.
    int numPages = (int) ceil((double) size / PGSIZE);
    //printf("\tNumber of pages: %d.\n", numPages);

    int i;

    // Update the Physical Bitmap.
    pthread_mutex_lock(&physicalBitmapLock);
    for (i = 0; i < numPages; i++)
    {
        void *physicalPage = Translate(pageDirectory, va + i);
        physicalBitmap[((unsigned long) physicalPage - (unsigned long) physicalMemory) / PGSIZE] = 0;
        //printf("\t\tUpdated physical bitmap at index %lu.\n", ((unsigned long) physicalPage - (unsigned long) physicalMemory) / PGSIZE);
        // physical address = index of bit * pagesize + offset of start of physical memory.
    }
    pthread_mutex_unlock(&physicalBitmapLock);

    // Update the Virtual Bitmap.
    pthread_mutex_lock(&virtualBitmapLock);
    for (i = 0; i < numPages; i++)
    {
        virtualBitmap[(unsigned int) va / PGSIZE] = 0;
        //printf("\t\tUpdated virtual bitmap at index %u.\n", (unsigned int) va / PGSIZE);
    }
    pthread_mutex_unlock(&virtualBitmapLock);

    // Update the Page Directory.
    pthread_mutex_lock(&pageDirectoryLock);
    for (i = 0; i < numPages; i++)
    {
        //printf("\t\tUpdating Page Directory at virtual address %x.\n", (int) (va + i));
        PageMap(pageDirectory, va + i, NULL);
    }
    pthread_mutex_unlock(&pageDirectoryLock);

    //printf("Myfree successful.\n");
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
    
    //printf("Putting value at %x into physical memory pointed to by virtual address %x.\n", (int) val, (int) va);

    // Calculate the number of pages needed to hold the data.
    int numPages = (int) ceil((double) size / PGSIZE);
    //printf("\tNumber of pages: %d.\n", numPages);

    int i;
    int bytesRemaining = size, bytesToWrite;

    // For each virtual page, find the corresponding physical page and write the data.
    for (i = 0; i < numPages; i++)
    {
        // Get the Physical Page.
        void *physicalPage = Translate(pageDirectory, va + i);
        //printf("\tPhysical page address %d: %x.\n", i, (int) physicalPage);
        
        // If there is no entry in the Page Directory for this virtual address, it has not been allocated; return.
        if (physicalPage == NULL)
        {
            //printf("\tReturning: no entry in Page Directory.\n");
            return;
        }

        // Write the data.
        //printf("\tAttemping to write data from source address %x to destination %x.\n", (int) (val + i * PGSIZE), (int) physicalPage);
        bytesToWrite = bytesRemaining > PGSIZE ? PGSIZE : size % PGSIZE;
        pthread_mutex_lock(&physicalMemoryLock);
        memcpy(physicalPage, val + i * PGSIZE, bytesToWrite);
        pthread_mutex_unlock(&physicalMemoryLock);
        bytesRemaining -= bytesToWrite;
    }
}


/*
Given a virtual address, this function copies the contents of the page to val.
*/
void GetVal(void *va, void *val, int size)
{
    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB, always check first the presence of translation
    in TLB before proceeding forward. */

    //printf("Putting value at %x into physical memory pointed to by virtual address %x.\n", (int) va, (int) val);

    // Calculate the number of pages needed to hold the data.
    int numPages = (int) ceil((double) size / PGSIZE);
    //printf("\tNumber of pages: %d.\n", numPages);

    int i;
    int bytesRemaining = size, bytesToWrite;

    // For each virtual page, find the corresponding physical page and write the data.
    for (i = 0; i < numPages; i++)
    {
        // Get the Physical Page.
        void *physicalPage = Translate(pageDirectory, va + i);
        //printf("\tPhysical page address %d: %x.\n", i, (int) physicalPage);
        
        // If there is no entry in the Page Directory for this virtual address, it has not been allocated; return.
        if (physicalPage == NULL)
        {
            //printf("\tReturning: no entry in Page Directory.\n");
            return;
        }

        // Write the data.
        bytesToWrite = bytesRemaining > PGSIZE ? PGSIZE : size % PGSIZE;
        pthread_mutex_lock(&physicalMemoryLock);
        memcpy(val + i * PGSIZE, physicalPage, bytesToWrite);
        pthread_mutex_unlock(&physicalMemoryLock);
        bytesRemaining -= bytesToWrite;
    }
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
    store the result to the "answer array". */

    int i, j;
    int mat1Address = 0, mat2Address = 0, matResultAddress = 0;
    int x, y, z;
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            mat1Address = (unsigned int) mat1 + ((i * size * sizeof(int))) + (j * sizeof(int));
            mat2Address = (unsigned int) mat2 + ((i * size * sizeof(int))) + (j * sizeof(int));
            matResultAddress = (unsigned int) answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            GetVal((void *) mat1Address, &x, sizeof(int));
            GetVal((void *) mat2Address, &y, sizeof(int));
            z = x * y;
            PutVal((void *) matResultAddress, &(z), sizeof(int));
        }
    }
}
