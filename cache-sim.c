/*
 * =====================================================================================
 *
 *       Filename:  cache-sim.c
 *
 *    Description:  Cache Simulator
 *
 *        Version:  1.1
 *        Created:  04/22/2019 00:02:20
 *       Revision:  1.1 – 12/20/2021
 *                  Version 1.1 expands the basic cache simulation infrastructure
 *                  developed in version 1.0 to include support for an L2 cache,
 *                  along with various other improvements and cleanup.
 *       Compiler:  gcc
 *
 *         Author:  Gregory Giovannini (Student), gregory.giovannini@rutgers.edu
 *   Organization:  Rutgers University
 *
 * =====================================================================================
 */

#include "cache-sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isPowerOfTwo(int n);
int getAssociativity(char *cacheAssociativity);
void printCounters(int memoryReads, int memoryWrites, int l1CacheHits, int l1CacheMisses, int l2CacheHits, int l2CacheMisses);
int logBase2(int n);
unsigned long long int getTag(unsigned long long int address, int setBits, int tagBits, int blockOffset);
unsigned long int getSet(unsigned long long int address, int setBits, int tagBits, int blockOffset);
int fetch(Cache *cache, int prefetching, unsigned long long int tag, unsigned long int set, int numLines, int numSets);
void updateLRU(Cache *cache, int tagIndex, int setIndex, int numLines);
int evict(Cache *cache, unsigned long long int tag, int setIndex, int numLines);
void printCache(Cache *cache, int numSets, int numLines);
void printUsage();

int main(int argc, char *argv[])
{
    /* Maintain a cache simulator.
     * The Cache contains a hash table of Sets.
     * A Set contains a hash table of Lines.
     * A Line contains information about the validity of the current block, the tag, and the block itself.
     * Assume all addresses will be 48 bits.
     * To index:
     * The Block Offset (number of bits) is given by log2(cacheBlockSize); discard these bits on the right
     * The Set bits are given by log2(cacheAssociativityN); use these bits on the left
     * The Tag (number of bits) is given by the number of bits in the address (48) - Block Offset bits - Set bits
     */

    /* The total size of the caches in bytes; should be a power of 2 */
    int l1CacheSize;
    int l2CacheSize;
    /* The associativity of the caches; either direct (1), accoc (2), or assoc:n (3);
     * n should be a power of 2 */
    char *l1CacheAssociativity; int l1CacheAssociativityType = 0, l1CacheAssociativityN = 1;
    char *l2CacheAssociativity; int l2CacheAssociativityType = 0, l2CacheAssociativityN = 1;
    /* The cache policies for eviction (always lru) */
    char *l1CachePolicy;
    char *l2CachePolicy;
    /* The size of the cache blocks in bytes; should be a power of 2 */
    int l1CacheBlockSize;
    int l2CacheBlockSize;
    /* The name of the trace file */
    char *traceFile; FILE *traceFP;

    if (argc != 10)
    {
        printf("Error: invalid number of arguments.\n");
        printUsage();
        return -1;
    }

    /* Assume argv[1] is an int representing the L1 cache size */
    l1CacheSize = atoi(argv[1]);
    /* Assume argv[2] is a string representing the L1 cache associativity */
    l1CacheAssociativity = argv[2];
    /* Assume argv[3] is a string representing the L1 cache replacement policy */
    l1CachePolicy = argv[3];
    /* Assume argv[4] is an int representing the L1 cache block size */
    l1CacheBlockSize = atoi(argv[4]);
    /* Assume argv[5] is an int representing the L2 cache size */
    l2CacheSize = atoi(argv[5]);
    /* Assume argv[6] is a string representing the L2 cache associativity */
    l2CacheAssociativity = argv[6];
    /* Assume argv[7] is a string representing the L2 cache replacement policy */
    l2CachePolicy = argv[7];
    /* Assume argv[8] is an int representing the L2 cache block size */
    l2CacheBlockSize = atoi(argv[8]);
    /* Assume argv[9] is a string representing the name of the trace file */
    traceFile = argv[9];

    /* Error Checking: */

    /* Cache Size */
    /* Must be a positive power of 2 */
    if (l1CacheSize <= 0 || !isPowerOfTwo(l1CacheSize))
    {
        printf("Error: L1 cache size must be a power of 2.\n");
        return -1;
    }
    if (l2CacheSize <= 0 || !isPowerOfTwo(l2CacheSize))
    {
        printf("Error: L2 cache size must be a power of 2.\n");
        return -1;
    }

    /* Associativity */
    /* Either direct, assoc, or assoc:n, where n is a positive power of 2 */
    /* If cache is direct (strcmp returns 0 if strings are equal), cache is direct (1) */
    if (!strcmp(l1CacheAssociativity, "direct"))
    {
        l1CacheAssociativityType = 1;
    }
    /* Otherwise, determine the associativity */
    else
    {
        /* Get the associativity */
        int l1CacheAssociativityResult = getAssociativity(l1CacheAssociativity);

        /* If cacheAssociativityResult is 0, cache is fully associative (2) */
        if (l1CacheAssociativityResult == 0)
        {
            l1CacheAssociativityType = 2;
            l1CacheAssociativityN = l1CacheSize / l1CacheBlockSize;
        }

        /* If cacheAssociativtyResult is positive, cache is n-way associative (3) */
        if (l1CacheAssociativityResult > 0)
        {
            l1CacheAssociativityType = 3;
            l1CacheAssociativityN = l1CacheAssociativityResult;
        }

        /* If cacheAssociativityResult is -1, error */
        if (l1CacheAssociativityResult < 0)
        {
            printf("Error: invalid L1 cache associativity.\n");
            return -1;
        }
    }

    if (!strcmp(l2CacheAssociativity, "direct"))
    {
        l2CacheAssociativityType = 1;
    }
    /* Otherwise, determine the associativity */
    else
    {
        /* Get the associativity */
        int l2CacheAssociativityResult = getAssociativity(l2CacheAssociativity);

        /* If cacheAssociativityResult is 0, cache is fully associative (2) */
        if (l2CacheAssociativityResult == 0)
        {
            l2CacheAssociativityType = 2;
            l2CacheAssociativityN = l2CacheSize / l2CacheBlockSize;
        }

        /* If cacheAssociativtyResult is positive, cache is n-way associative (3) */
        if (l2CacheAssociativityResult > 0)
        {
            l2CacheAssociativityType = 3;
            l2CacheAssociativityN = l2CacheAssociativityResult;
        }

        /* If cacheAssociativityResult is -1, error */
        if (l2CacheAssociativityResult < 0)
        {
            printf("Error: invalid L2 cache associativity.\n");
            return -1;
        }
    }

    /* Cache Policy */
    /* Only valid policy is lru (least recently used) */
    if (strcmp(l1CachePolicy, "lru"))
    {
        printf("Error: invalid L1 cache replacement policy.\n");
        return -1;
    }
    if (strcmp(l2CachePolicy, "lru"))
    {
        printf("Error: invalid L2 cache replacement policy.\n");
        return -1;
    }

    /* Block Size */
    /* Must be a positive power of 2, less than or equal to the cache size */
    if (l1CacheBlockSize > l1CacheSize || !isPowerOfTwo(l1CacheBlockSize))
    {
        printf("Error: L1 block size must be a positive power of 2, <= to L1 cache size.\n");
        return -1;
    }
    if (l2CacheBlockSize > l2CacheSize || !isPowerOfTwo(l2CacheBlockSize))
    {
        printf("Error: L2 block size must be a positive power of 2, <= to L2 cache size.\n");
        return -1;
    }

    /* Trace File */
    traceFP = fopen(traceFile, "r");
    /* fopen returns 0, the NULL pointer, on failure */
    if (traceFP == 0)
    {
        printf("Error: trace file not found.\n");
        return -1;
    }

    /* Create Cache Models */
    /* Assume addresses are 48 bits */
    int addressLength = 48;
    /* Determine the number of bits for the Block Offset */
    int l1BlockOffsetBits = logBase2(l1CacheBlockSize);
    int l2BlockOffsetBits = logBase2(l2CacheBlockSize);
    /* Determine the number of Lines per Set */
    int l1NumLines = l1CacheAssociativityN;
    int l2NumLines = l2CacheAssociativityN;
    /* Determine the number of Sets in the Cache */
    int l1NumSets = l1CacheSize / (l1CacheBlockSize * l1NumLines);
    int l2NumSets = l2CacheSize / (l2CacheBlockSize * l2NumLines);
    /* Determine the number of bits for the Set */
    int l1SetBits = logBase2(l1NumSets);
    int l2SetBits = logBase2(l2NumSets);
    /* Determine the number of bits for the Tag */
    int l1TagBits = addressLength - l1BlockOffsetBits - l1SetBits;
    int l2TagBits = addressLength - l2BlockOffsetBits - l2SetBits;

    /* Allocate the Cache structs */
    Cache *l1NoPrefetchCache = (Cache *) malloc(sizeof(Cache));
    Cache *l1WithPrefetchCache = (Cache *) malloc(sizeof(Cache));
    l1NoPrefetchCache -> size = l1CacheSize;
    l1WithPrefetchCache -> size = l1CacheSize;
    l1NoPrefetchCache -> blockSize = l1CacheBlockSize;
    l1WithPrefetchCache -> blockSize = l1CacheBlockSize;
    l1NoPrefetchCache -> associativity = l1CacheAssociativityType;
    l1WithPrefetchCache -> associativity = l1CacheAssociativityType;
    l1NoPrefetchCache -> nSets = l1CacheAssociativityN;
    l1WithPrefetchCache -> nSets = l1CacheAssociativityN;

    Cache *l2NoPrefetchCache = (Cache *) malloc(sizeof(Cache));
    Cache *l2WithPrefetchCache = (Cache *) malloc(sizeof(Cache));
    l2NoPrefetchCache -> size = l2CacheSize;
    l2WithPrefetchCache -> size = l2CacheSize;
    l2NoPrefetchCache -> blockSize = l2CacheBlockSize;
    l2WithPrefetchCache -> blockSize = l2CacheBlockSize;
    l2NoPrefetchCache -> associativity = l2CacheAssociativityType;
    l2WithPrefetchCache -> associativity = l2CacheAssociativityType;
    l2NoPrefetchCache -> nSets = l2CacheAssociativityN;
    l2WithPrefetchCache -> nSets = l2CacheAssociativityN;

    /* Allocate the Caches' hash table of Sets */
    l1NoPrefetchCache -> sets = (Set *) malloc(l1NumSets * sizeof(Set));
    l1WithPrefetchCache -> sets = (Set *) malloc(l1NumSets * sizeof(Set));

    l2NoPrefetchCache -> sets = (Set *) malloc(l2NumSets * sizeof(Set));
    l2WithPrefetchCache -> sets = (Set *) malloc(l2NumSets * sizeof(Set));

    /* Allocate each Set's Lines */
    int i, j;
    for (i = 0; i < l1NumSets; i++)
    {
        l1NoPrefetchCache -> sets[i].lines = (Line *) malloc(l1NumLines * sizeof(Line));
        l1WithPrefetchCache -> sets[i].lines = (Line *) malloc(l1NumLines * sizeof(Line));
        l1NoPrefetchCache -> sets[i].numItems = 0;
        l1WithPrefetchCache -> sets[i].numItems = 0;

        /* Initialize each Line */
        for (j = 0; j < l1NumLines; j++)
        {
            l1NoPrefetchCache -> sets[i].lines[j].valid = 0;
            l1WithPrefetchCache -> sets[i].lines[j].valid = 0;
            l1NoPrefetchCache -> sets[i].lines[j].usage = 0;
            l1WithPrefetchCache -> sets[i].lines[j].usage = 0;
        }
    }
    for (i = 0; i < l2NumSets; i++)
    {
        l2NoPrefetchCache -> sets[i].lines = (Line *) malloc(l2NumLines * sizeof(Line));
        l2WithPrefetchCache -> sets[i].lines = (Line *) malloc(l2NumLines * sizeof(Line));
        l2NoPrefetchCache -> sets[i].numItems = 0;
        l2WithPrefetchCache -> sets[i].numItems = 0;

        /* Initialize each Line */
        for (j = 0; j < l2NumLines; j++)
        {
            l2NoPrefetchCache -> sets[i].lines[j].valid = 0;
            l2WithPrefetchCache -> sets[i].lines[j].valid = 0;
            l2NoPrefetchCache -> sets[i].lines[j].usage = 0;
            l2WithPrefetchCache -> sets[i].lines[j].usage = 0;
        }
    }

    /* Simulation */

    /* Establish no-prefetch counters */
    /* The number of reads from memory */
    int noPrefetchMemoryReads = 0;
    /* The number of writes to memory */
    int noPrefetchMemoryWrites = 0;
    /* The number of cache hits */
    int l1NoPrefetchCacheHits = 0;
    int l2NoPrefetchCacheHits = 0;
    /* The number of cache misses */
    int l1NoPrefetchCacheMisses = 0;
    int l2NoPrefetchCacheMisses = 0;

    /* Establish with-prefetch counters */
    /* The number of reads from memory */
    int withPrefetchMemoryReads = 0;
    /* The number of writes to memory */
    int withPrefetchMemoryWrites = 0;
    /* The number of cache hits */
    int l1WithPrefetchCacheHits = 0;
    int l2WithPrefetchCacheHits = 0;
    /* The number of cache misses */
    int l1WithPrefetchCacheMisses = 0;
    int l2WithPrefetchCacheMisses = 0;

    /* Read in each line from the trace file, until the end */
    char line[100];
    long int instruction;
    char operation;
    unsigned long long int address = 0;
    unsigned long long int l1AddressTag = 0;
    unsigned long long int l2AddressTag = 0;
    unsigned long int l1AddressSet = 0;
    unsigned long int l2AddressSet = 0;
    int hit = 0;
    unsigned long int count = 0;

    fscanf(traceFP, "%99[^\n]", line);
    getc(traceFP);
    while (strcmp(line, "#eof"))
    {
        sscanf(line, "%lx: %c %llx\n", &instruction, &operation, &address);
        
        count++;

        /* Get the Tag and the Set from the Address */
        l1AddressTag = getTag(address, l1SetBits, l1TagBits, l1BlockOffsetBits);
        l1AddressSet = getSet(address, l1SetBits, l1TagBits, l1BlockOffsetBits);
        
        /*
        printf("Instruction: %s\n", line);
        printf("Set: %lu, NumSets: %d, SetIndex: %d\n", addressSet, numSets, hash(addressSet, numSets));
        printf("Tag: %llx, NumLines: %d, TagIndex: %d\n", addressTag, numLines, hash(addressTag, numLines));
        */

        /* If operation is a Read */
        if (operation == 'R')
        {
            /* Read Without Prefetching */
            /* Check L1 cache */
            hit = fetch(l1NoPrefetchCache, 0, l1AddressTag, l1AddressSet, l1NumLines, l1NumSets);

            /* If L1 Cache Hit */
            if (hit)
            {
                l1NoPrefetchCacheHits++;
            }
            /* If L1 Cache Miss */
            else
            {
                l1NoPrefetchCacheMisses++;

                /* Check L2 cache */
                l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                hit = fetch(l2NoPrefetchCache, 0, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                /* If L2 Cache Hit */
                if (hit)
                {
                    l2NoPrefetchCacheHits++;
                }
                /* If L2 Cache Miss */
                else
                {
                    l2NoPrefetchCacheMisses++;
                    noPrefetchMemoryReads++;
                }
            }

            /* Read With Prefetching */
            /* Check L1 cache */
            hit = fetch(l1WithPrefetchCache, 0, l1AddressTag, l1AddressSet, l1NumLines, l1NumSets);

            /* If L1 Cache Hit */
            if (hit)
            {
                l1WithPrefetchCacheHits++;
            }
            /* If L1 Cache Miss */
            else
            {
                l1WithPrefetchCacheMisses++;

                /* Check L2 cache */
                l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                hit = fetch(l2WithPrefetchCache, 0, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                /* If L2 Cache Hit */
                if (hit)
                {
                    l2WithPrefetchCacheHits++;
                }
                /* If L2 Cache Miss */
                else
                {
                    l2WithPrefetchCacheMisses++;
                    withPrefetchMemoryReads++;

                    /* Prefetch */
                    /* Get the new address by adding the Block Size */
                    address += l2CacheBlockSize;
                    /* Get the Tag and the Set from the new Address */
                    l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                    l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                    hit = fetch(l2WithPrefetchCache, 1, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                    /* If Cache Hit */
                    if (hit)
                    {

                    }
                    /* If Cache Miss */
                    else
                    {
                        withPrefetchMemoryReads++;
                    }
                }
            }
        }

        /* If operation is a Write */
        else if (operation == 'W')
        {
            /* Write Without Prefetching */
            /* Check L1 cache */
            hit = fetch(l1NoPrefetchCache, 0, l1AddressTag, l1AddressSet, l1NumLines, l1NumSets);

            /* If Cache Hit */
            if (hit)
            {
                l1NoPrefetchCacheHits++;
                noPrefetchMemoryWrites++;
            }
            /* If Cache Miss */
            else
            {
                l1NoPrefetchCacheMisses++;

                /* Check L2 cache */
                l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                hit = fetch(l2NoPrefetchCache, 0, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                /* If L2 Cache Hit */
                if (hit)
                {
                    l2NoPrefetchCacheHits++;
                    noPrefetchMemoryWrites++;
                }
                /* If L2 Cache Miss */
                else
                {
                    l2NoPrefetchCacheMisses++;
                    noPrefetchMemoryReads++;
                    noPrefetchMemoryWrites++;
                }
            }

            /* Write With Prefetching */
            /* Check L1 cache */
            hit = fetch(l1WithPrefetchCache, 0, l1AddressTag, l1AddressSet, l1NumLines, l1NumSets);

            /* If Cache Hit */
            if (hit)
            {
                l1WithPrefetchCacheHits++;
                withPrefetchMemoryWrites++;
            }
            /* If Cache Miss */
            else
            {
                l1WithPrefetchCacheMisses++;

                /* Check L2 cache */
                l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                hit = fetch(l2WithPrefetchCache, 0, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                /* If L2 Cache Hit */
                if (hit)
                {
                    l2WithPrefetchCacheHits++;
                    withPrefetchMemoryWrites++;
                }
                /* If L2 Cache Miss */
                else
                {
                    l2WithPrefetchCacheMisses++;
                    withPrefetchMemoryReads++;
                    withPrefetchMemoryWrites++;

                    /* Prefetch */
                    /* Get the new address by adding the Block Size */
                    address += l2CacheBlockSize;
                    /* Get the Tag and the Set from the new Address */
                    l2AddressTag = getTag(address, l2SetBits, l2TagBits, l2BlockOffsetBits);
                    l2AddressSet = getSet(address, l2SetBits, l2TagBits, l2BlockOffsetBits);

                    hit = fetch(l2WithPrefetchCache, 1, l2AddressTag, l2AddressSet, l2NumLines, l2NumSets);

                    /* If Cache Hit */
                    if (hit)
                    {
                        
                    }
                    /* If Cache Miss */
                    else
                    {
                        withPrefetchMemoryReads++;
                    }
                }
            }
        }

        /*
        printCache(noPrefetchCache, numSets, numLines);
        printCache(withPrefetchCache, numSets, numLines);
        printf("no-prefetch\n");
        printCounters(noPrefetchMemoryReads, noPrefetchMemoryWrites, noPrefetchCacheHits, noPrefetchCacheMisses);
        printf("with-prefetch\n");
        printCounters(withPrefetchMemoryReads, withPrefetchMemoryWrites, withPrefetchCacheHits, withPrefetchCacheMisses);
        printf("++++++++++++++++++++++++++++++++\n");
        */

        fscanf(traceFP, "%99[^\n]", line);
        getc(traceFP);
    }


    /* Print the results */
    printf("-----\nNo Prefetch\n-----\n");
    printCounters(noPrefetchMemoryReads, noPrefetchMemoryWrites, l1NoPrefetchCacheHits, l1NoPrefetchCacheMisses, l2NoPrefetchCacheHits, l2NoPrefetchCacheMisses);

    printf("-----\nWith Prefetch\n-----\n");
    printCounters(withPrefetchMemoryReads, withPrefetchMemoryWrites, l1WithPrefetchCacheHits, l1WithPrefetchCacheMisses, l2WithPrefetchCacheHits, l2WithPrefetchCacheMisses);

    /* printf("Count: %lu\n", count); */

    /* Close file */
    fclose(traceFP);

    /* Free memory */
    /* Free each Set's Lines */
    for (i = 0; i < l1NumSets; i++)
    {
        /* Free each Line */
        free(l1NoPrefetchCache -> sets[i].lines);
        free(l1WithPrefetchCache -> sets[i].lines);
    }
    for (i = 0; i < l2NumSets; i++)
    {
        /* Free each Line */
        free(l2NoPrefetchCache -> sets[i].lines);
        free(l2WithPrefetchCache -> sets[i].lines);
    }

    /* Free the Caches' hash table of Sets */
    free(l1NoPrefetchCache -> sets);
    free(l1WithPrefetchCache -> sets);
    free(l2NoPrefetchCache -> sets);
    free(l2WithPrefetchCache -> sets);

    /* Free the Cache structs */
    free(l1NoPrefetchCache);
    free(l1WithPrefetchCache);
    free(l2NoPrefetchCache);
    free(l2WithPrefetchCache);

    return 0;
}


int isPowerOfTwo(int n)
{
    /* Return 0 if n is not a power of 2, 1 otherwise */

    /* If n is 0, n is not a power of 2 */
    if (n == 0)
    {
        return 0;
    }
    /* Continue dividing n by 2 until a remainder is found (return 0) or n is 1 (return 1) */
    while (n != 1)
    {
        /* If a remainder is found, n is not a power of 2 */
        if (n % 2 != 0)
        {
            return 0;
        }
        /* Divide n by 2 */
        n = n / 2;
    }

    /* Here, n must be a power of 2 */
    return 1;
}


int getAssociativity(char *cacheAssociativity)
{
    /* Return 0 if cache is fully associative, n if n-way associative, and -1 on error */

    /* If cache is fully associative */
    if (!strcmp(cacheAssociativity, "assoc"))
    {
        return 0;
    }

    /* Split cacheAssociativity string at ":" */
    char *cacheAssociativityToken = strtok(cacheAssociativity, ":");

    /* If first token is not "assoc", improper formatting error */
    if (strcmp(cacheAssociativityToken, "assoc"))
    {
        printf("Error: improperly formatted cache associativity: missing 'assoc' token.\n");
        return -1;
    }

    /* Get the second token */
    cacheAssociativityToken = strtok(NULL, ":");

    /* If there is nothing after the ":", improper formatting error */
    if (cacheAssociativityToken == 0)
    {
        printf("Error: improperly formatted cache associativity: missing value after ':'.\n");
        return -1;
    }

    /* Get the n-way associativity */
    int n = atoi(cacheAssociativityToken);

    /* If n is not a power of 2, error */
    if (!isPowerOfTwo(n))
    {
        printf("Error: associativity is not a power of 2.\n");
        return -1;
    }

    return n;
}


void printCounters(int memoryReads, int memoryWrites, int l1CacheHits, int l1CacheMisses, int l2CacheHits, int l2CacheMisses)
{
    printf("Memory reads: %d\n", memoryReads);
    printf("Memory writes: %d\n", memoryWrites);
    printf("L1 cache hits: %d\n", l1CacheHits);
    printf("L1 cache misses: %d\n", l1CacheMisses);
    printf("L2 cache hits: %d\n", l2CacheHits);
    printf("L2 cache misses: %d\n", l2CacheMisses);
}


int logBase2(int n)
{
    /* Continue shifting right until n is 0 */
    int log = 0;
    while (n - 1 > 0)
    {
        n = n >> 1;
        log++;
    }

    return log;
}


unsigned long long int getTag(unsigned long long int address, int setBits, int tagBits, int blockOffset)
{
    /* The mask that will be used to get the ith bit */
    unsigned long long int mask = 1;
    
    /* Shift address right by blockOffset and setBits */
    address = address >> (blockOffset + setBits);

    /* For each bit after the blockOffset and setBits */
    unsigned long long int tag = 0;
    int i;
    for (i = 0; i < tagBits; i++)
    {
        /* AND the ith bit of address with mask, and add it to tag */
        tag += address & mask;

        /* printf("Tag %d, Address & Mask %lu\n", tag, address & mask); */

        /* Shift mask left */
        mask = mask << 1;
    }

    return tag;
}


unsigned long int getSet(unsigned long long int address, int setBits, int tagBits, int blockOffset)
{
    /* The mask that will be used to get the ith bit */
    unsigned long long int mask = 1;
    
    /* Shift address right by blockOffset */
    address = address >> blockOffset;

    /* For each bit after the blockOffset */
    unsigned long int set = 0;
    int i;
    for (i = 0; i < setBits; i++)
    {
        /* AND the ith bit of address with mask, and add it to set */
        set += address & mask;

        /* printf("Set %d, Address & Mask %lu\n", set, address & mask); */

        /* Shift mask left */
        mask = mask << 1;
    }

    return set;
}


int fetch(Cache *cache, int prefetching, unsigned long long int tag, unsigned long int set, int numLines, int numSets)
{
    /* Return 1 on a Cache Hit, 0 on a Cache Miss */
    int hit = 0;

    /* Hash the Tag and the Set */
    int tagIndex = hash(tag, numLines);
    int setIndex = hash(set, numSets);

    /* printf("Tag is %llx. Indexing Set %d Line %d\n", tag, setIndex, tagIndex); */
    /* Index into the Cache and retrieve the proper Valid bit and Tag */
    int validBit = cache -> sets[setIndex].lines[tagIndex].valid;
    unsigned long long int currentTag = cache -> sets[setIndex].lines[tagIndex].tag;
    
    /*
    printf("Fetching SetIndex %d TagIndex %d\n", setIndex, tagIndex);
    printf("Comparing %lu to %lu\n", tag, currentTag);
     */
    
    /* If the Valid bit is 1 and the current Tag at tagIndex matches the new Tag, Cache Hit */
    if (validBit && (currentTag == tag))
    {
        /* printf("HIT\n"); */
        hit = 1;
    }
    /* If the Valid bit is 1 and the current Tag at tagIndex does not match the new Tag */
    else if (validBit && (currentTag != tag))
    {
        /* Linearly probe the adjacent Lines in the Set;
         * If the Valid bit is 1 and the current Tag at ((tagIndex + i) % numLines) matches the new Tag, Cache Hit)
         * If no matches are found, Cache Miss, evict LRU from cache and bring new address into cache
         */
        int i, iIndex, iValid, found = 0;
        unsigned long long int iTag = 0;
        for (i = 1; i < numLines; i++)
        {
            iIndex = hash(tagIndex + i, numLines);
            iValid = cache -> sets[setIndex].lines[iIndex].valid;
            iTag = cache -> sets[setIndex].lines[iIndex].tag;
            
            /* printf("LINEAR PROBE: Comparing %llx to %llx\n", tag, iTag); */
            
            /* If the Valid bit is 1 and the current Tag at ((tagIndex + i) % numLines) matches the new Tag, Cache Hit */
            if (iValid && (iTag == tag))
            {
                /* printf("HIT\n"); */
                hit = 1, found = 1;
                
                /* Update the blocks' Least Recently Used properties */
                /* updateLRU(cache, tagIndex, setIndex, numLines); */
                
                tagIndex = iIndex;
                
                break;
            }
        }
        /* If no matches are found, Cache Miss */
        if (!found)
        {
            /* printf("MISS\n"); */
            hit = 0;
            
            /* If Set is not full */
            if (cache -> sets[setIndex].numItems < numLines)
            {
                /* Search for an empty space */
                for (i = 1; i < numLines; i++)
                {
                    iIndex = hash(tagIndex + i, numLines);
                    iValid = cache -> sets[setIndex].lines[iIndex].valid;
                    
                    /* If the Valid bit is 0, Cache Miss, bring new address into the Cache, write into empty location */
                    if (!iValid)
                    {
                        /* Bring new address into the Cache, write into empty location */
                        cache -> sets[setIndex].lines[iIndex].valid = 1;
                        cache -> sets[setIndex].lines[iIndex].tag = tag;
                        cache -> sets[setIndex].numItems++;
                        
                        /* Update the blocks' Least Recently Used properties */
                        /* updateLRU(cache, tagIndex, setIndex, numLines); */
                        
                        tagIndex = iIndex;
                        break;
                    }
                }
            }
            /* If Set is full */
            else if (cache -> sets[setIndex].numItems == numLines)
            {
                /* Update the blocks' Least Recently Used properties */
                /* updateLRU(cache, tagIndex, setIndex, numLines); */
                
                /* Evict LRU from cache and bring address into cache */
                int evictedIndex = evict(cache, tag, setIndex, numLines);
                
                tagIndex = evictedIndex;
            }
        }
    }
    /* If the Valid bit is not 1, Cache Miss */
    else if (!validBit)
    {
        /* printf("MISS\n"); */
        hit = 0;
        
        /* Bring new address into the Cache, write into empty location */
        cache -> sets[setIndex].lines[tagIndex].valid = 1;
        cache -> sets[setIndex].lines[tagIndex].tag = tag;
        cache -> sets[setIndex].numItems++;
    }
    
    
    if (!prefetching || !hit)
    {
        /* Update the blocks' Least Recently Used properties */
        updateLRU(cache, tagIndex, setIndex, numLines);
    }

    return hit;
}


void updateLRU(Cache *cache, int tagIndex, int setIndex, int numLines)
{
    /* Assume that the block at [setIndex][tagIndex] has been used.
     * Update the usage of each line in the set. */
    int i;
    for (i = 0; i < numLines; i++)
    {
        /* If the current line is the line that was just used */
        if (i == tagIndex)
        {
            /* This line is most recently used */
            cache -> sets[setIndex].lines[i].usage = 1;
        }
        /* Otherwise, if the current line has been used */
        else if (cache -> sets[setIndex].lines[i].usage > 0)
        {
            /* Increment usage */
            cache -> sets[setIndex].lines[i].usage++;
        }
    }
}


int evict(Cache *cache, unsigned long long int tag, int setIndex, int numLines)
{
    /* Find the Least Recently Used block and replace it with the given block */
    int indexOfLRU = 0;
    long int max = 0;
    int i;

    for (i = 0; i < numLines; i++)
    {
        if (cache -> sets[setIndex].lines[i].usage > max)
        {
            max = cache -> sets[setIndex].lines[i].usage;
            indexOfLRU = i;
        }
    }

    /* printf("Evicting %llx\n", cache -> sets[setIndex].lines[indexOfLRU].tag); */
    cache -> sets[setIndex].lines[indexOfLRU].tag = tag;
    
    return indexOfLRU;
}


void printCache(Cache *cache, int numSets, int numLines)
{
    int currentValidBit;
    unsigned long long int currentTag;
    int currentUsage;

    printf("----------------------------------------------------\n");
    int i, j;
    for (i = 0; i < numSets; i++)
    {
        printf("Set %d:\n", i);
        for (j = 0; j < numLines; j++)
        {
            currentValidBit = cache -> sets[i].lines[j].valid;
            currentTag = cache -> sets[i].lines[j].tag;
            currentUsage = cache -> sets[i].lines[j].usage;

            printf("\tLine %d: Valid - %d | Tag - %llx | Usage - %d\n", j, currentValidBit, currentTag, currentUsage);
        }
    }
    printf("----------------------------------------------------\n");
}


void printUsage()
{
    printf("usage: cache-sim l1_cache_size l1_assoc l1_replace_policy l1_block_size l2_cache_size l2_assoc l2_replace_policy l2_block_size trace_file\n");
    printf("\tl1_cache_size: int - size of L1 cache in bytes; must be a power of 2\n");
    printf("\tl1_assoc: str - associativity of L1 cache; can be one of:\n");
    printf("\t\tdirect - direct mapped cache\n");
    printf("\t\tassoc - fully associative cache\n");
    printf("\t\tassoc:n - n-way associative cache, where n is a power of 2\n");
    printf("\tl1_replace_policy: str - L1 cache replacement policy (lru only is supported)\n");
    printf("\tl1_block_size: int - size of L1 cache block in bytes; must be a power of 2\n");
    printf("\tl2_cache_size: int - size of L2 cache in bytes; must be a power of 2\n");
    printf("\tl2_assoc: str - associativity of L2 cache; can be one of:\n");
    printf("\t\tdirect - direct mapped cache\n");
    printf("\t\tassoc - fully associative cache\n");
    printf("\t\tassoc:n - n-way associative cache, where n is a power of 2\n");
    printf("\tl2_replace_policy: str - L2 cache replacement policy (lru only is supported)\n");
    printf("\tl2_block_size: int - size of L2 cache block in bytes; must be a power of 2\n");
    printf("\ttrace_file: str - path to trace file used as input to the simulator\n");
}
