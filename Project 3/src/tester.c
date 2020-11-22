#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

int numPageBitsOuter = 10;
int numPageBitsInner = 10;               
int numPageBitsOffset = 12;

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

    printBits(result);
    return result;
}

uint32_t getBits(void *addr, int start, int end)
{
    uint32_t result;
    uint32_t mask = createBitMask(start, end);

    result = ((uint32_t) addr) & mask;
    result >>= (32 - end);

    printBits(result);
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

int main()
{
    int x;
    printBits((uint32_t) &x);
    printf("%u\n\n", extractInnerBits((void *) &x));
    printf("%u\n\n", extractOuterBits((void *) &x));
    printf("%u\n\n", extractOffsetBits((void *) &x));
    // printf("%u\n\n", getBits((void *) &x, 1, 10));
    // printf("%u\n\n", getBits((void *) &x, 11, 20));
    // printf("%u\n\n", getBits((void *) &x, 21, 32));

    return 0;
}