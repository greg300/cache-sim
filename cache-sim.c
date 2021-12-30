/*
 * =====================================================================================
 *
 *       Filename:  cache-sim.c
 *
 *    Description:  Cache Simulator
 *
 *        Version:  1.1
 *        Created:  04/22/2019 00:02:20
 *       Revision:  12/20/2021
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
void printCounters(int memoryReads, int memoryWrites, int cacheHits, int cacheMisses);
int logBase2(int n);
unsigned long long int getTag(unsigned long long int address, int setBits, int tagBits, int blockOffset);
unsigned long int getSet(unsigned long long int address, int setBits, int tagBits, int blockOffset);
int fetch(Cache *cache, int prefetching, unsigned long long int tag, unsigned long int set, int numLines, int numSets);
void updateLRU(Cache *cache, int tagIndex, int setIndex, int numLines);
int evict(Cache *cache, unsigned long long int tag, int setIndex, int numLines);
void printCache(Cache *cache, int numSets, int numLines);

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

    /* The total size of the cache in bytes; should be a power of 2 */
    int cacheSize;
    /* The associativity of the cache; either direct (1), accoc (2), or assoc:n (3);
     * n should be a power of 2 */
    char *cacheAssociativity; int cacheAssociativityType = 0, cacheAssociativityN = 1;
    /* The cache policy for eviction (always lru) */
    char *cachePolicy;
    /* The size of the cache block in bytes; should be a power of 2 */
    int cacheBlockSize;
    /* The name of the trace file */
    char *traceFile; FILE *traceFP;

    /* Assume argv[1] is an int representing the cache size */
    cacheSize = atoi(argv[1]);
    /* Assume argv[2] is a string representing the cache associativity */
    cacheAssociativity = argv[2];
    /* Assume argv[3] is a string representing the cache policy */
    cachePolicy = argv[3];
    /* Assume argv[4] is an int representing the cache block size */
    cacheBlockSize = atoi(argv[4]);
    /* Assume argv[5] is a string representing the name of the trace file */
    traceFile = argv[5];

    /* printf("%d\n%s\n%s\n%d\n%s\n", cacheSize, cacheAssociativity, cachePolicy, cacheBlockSize, traceFile); */


    /* Error Checking: */

    /* Cache Size */
    /* Must be a positive power of 2 */
    if (cacheSize <= 0 || !isPowerOfTwo(cacheSize))
    {
        printf("error");
        return 0;
    }

    /* Associativity */
    /* Either direct, assoc, or assoc:n, where n is a positive power of 2 */
    /* If cache is direct (strcmp returns 0 if strings are equal), cache is direct (1) */
    if (!strcmp(cacheAssociativity, "direct"))
    {
        cacheAssociativityType = 1;
    }
    /* Otherwise, determine the associativity */
    else
    {
        /* Get the associativity */
        int cacheAssociativityResult = getAssociativity(cacheAssociativity);

        /* If cacheAssociativityResult is 0, cache is fully associative (2) */
        if (cacheAssociativityResult == 0)
        {
            cacheAssociativityType = 2;
            cacheAssociativityN = cacheSize / cacheBlockSize;
        }

        /* If cacheAssociativtyResult is positive, cache is n-way associative (3) */
        if (cacheAssociativityResult > 0)
        {
            cacheAssociativityType = 3;
            cacheAssociativityN = cacheAssociativityResult;
        }

        /* If cacheAssociativityResult is -1, error */
        if (cacheAssociativityResult < 0)
        {
            printf("error");
            return 0;
        }
    }

    /* Cache Policy */
    /* Only valid policy is lru (least recently used) */
    if (strcmp(cachePolicy, "lru"))
    {
        printf("error");
        return 0;
    }

    /* Block Size */
    /* Must be a positive power of 2, less than or equal to the cache size */
    if (cacheBlockSize > cacheSize || !isPowerOfTwo(cacheBlockSize))
    {
        printf("error");
        return 0;
    }

    /* Trace File */
    traceFP = fopen(traceFile, "r");
    /* fopen returns 0, the NULL pointer, on failure */
    if (traceFP == 0)
    {
        printf("error");
        return 0;
    }

    /* printf("%d\n%d\n", cacheAssociativityType, cacheAssociativityN); */


    /* Create Cache Model */
    /* Assume addresses are 48 bits */
    int addressLength = 48;
    /* Determine the number of bits for the Block Offset */
    int blockOffsetBits = logBase2(cacheBlockSize);
    /* Determine the number of Lines per Set */
    int numLines = cacheAssociativityN;
    /* Determine the number of Sets in the Cache */
    int numSets = cacheSize / (cacheBlockSize * numLines);
    /* Determine the number of bits for the Set */
    int setBits = logBase2(numSets);
    /* Determine the number of bits for the Tag */
    int tagBits = addressLength - blockOffsetBits - setBits;

    /* printf("Set Bits %d, Tag Bits %d, Block Offset Bits %d, Number of Lines %d, Number of Sets %d\n", setBits, tagBits, blockOffsetBits, numLines, numSets); */

    /* Allocate the Cache structs */
    Cache *noPrefetchCache = (Cache *) malloc(sizeof(Cache));
    Cache *withPrefetchCache = (Cache *) malloc(sizeof(Cache));
    noPrefetchCache -> size = cacheSize;
    withPrefetchCache -> size = cacheSize;
    noPrefetchCache -> blockSize = cacheBlockSize;
    withPrefetchCache -> blockSize = cacheBlockSize;
    noPrefetchCache -> associativity = cacheAssociativityType;
    withPrefetchCache -> associativity = cacheAssociativityType;
    noPrefetchCache -> nSets = cacheAssociativityN;
    withPrefetchCache -> nSets = cacheAssociativityN;

    /* Allocate the Caches' hash table of Sets */
    noPrefetchCache -> sets = (Set *) malloc(numSets * sizeof(Set));
    withPrefetchCache -> sets = (Set *) malloc(numSets * sizeof(Set));

    /* Allocate each Set's Lines */
    int i, j;
    for (i = 0; i < numSets; i++)
    {
        noPrefetchCache -> sets[i].lines = (Line *) malloc(numLines * sizeof(Line));
        withPrefetchCache -> sets[i].lines = (Line *) malloc(numLines * sizeof(Line));
        noPrefetchCache -> sets[i].numItems = 0;
        withPrefetchCache -> sets[i].numItems = 0;


        /* Initialize each Line */
        for (j = 0; j < numLines; j++)
        {
            noPrefetchCache -> sets[i].lines[j].valid = 0;
            withPrefetchCache -> sets[i].lines[j].valid = 0;
            noPrefetchCache -> sets[i].lines[j].usage = 0;
            withPrefetchCache -> sets[i].lines[j].usage = 0;
            /* printf("Set %d Line %d\n", i, j); */
        }
    }


    /* Simulation */

    /* Establish no-prefetch counters */
    /* The number of reads from memory */
    int noPrefetchMemoryReads = 0;
    /* The number of writes to memory*/
    int noPrefetchMemoryWrites = 0;
    /* The number of cache hits */
    int noPrefetchCacheHits = 0;
    /* The number of cache misses */
    int noPrefetchCacheMisses = 0;

    /* Establish with-prefetch counters */
    /* The number of reads from memory */
    int withPrefetchMemoryReads = 0;
    /* The number of writes to memory */
    int withPrefetchMemoryWrites = 0;
    /* The number of cache hits */
    int withPrefetchCacheHits = 0;
    /* The number of cache misses */
    int withPrefetchCacheMisses = 0;


    /* Read in each line from the trace file, until the end */
    char line[100];
    long int instruction;
    char operation;
    unsigned long long int address = 0;
    unsigned long long int addressTag = 0;
    unsigned long int addressSet = 0;
    int hit = 0;
    unsigned long int count = 0;

    fscanf(traceFP, "%99[^\n]", line);
    getc(traceFP);
    while (strcmp(line, "#eof"))
    {
        sscanf(line, "%lx: %c %llx\n", &instruction, &operation, &address);
        /* printf("%c %lx\n", operation, address); */
        
        count++;

        /* Get the Tag and the Set from the Address */
        addressTag = getTag(address, setBits, tagBits, blockOffsetBits);
        addressSet = getSet(address, setBits, tagBits, blockOffsetBits);
        
        /*
        printf("Instruction: %s\n", line);
        printf("Set: %lu, NumSets: %d, SetIndex: %d\n", addressSet, numSets, hash(addressSet, numSets));
        printf("Tag: %llx, NumLines: %d, TagIndex: %d\n", addressTag, numLines, hash(addressTag, numLines));
        */

        /* If operation is a Read */
        if (operation == 'R')
        {
            /* Read Without Prefetching */
            hit = fetch(noPrefetchCache, 0, addressTag, addressSet, numLines, numSets);

            /* If Cache Hit */
            if (hit)
            {
                noPrefetchCacheHits++;
            }
            /* If Cache Miss */
            else
            {
                noPrefetchCacheMisses++;
                noPrefetchMemoryReads++;
            }


            /* Read With Prefetching */
            hit = fetch(withPrefetchCache, 0, addressTag, addressSet, numLines, numSets);

            /* If Cache Hit */
            if (hit)
            {
                withPrefetchCacheHits++;
            }
            /* If Cache Miss */
            else
            {
                withPrefetchCacheMisses++;
                withPrefetchMemoryReads++;

                /* Prefetch */
                /* Get the new address by adding the Block Size */
                address += cacheBlockSize;
                /* Get the Tag and the Set from the new Address */
                addressTag = getTag(address, setBits, tagBits, blockOffsetBits);
                addressSet = getSet(address, setBits, tagBits, blockOffsetBits);

                hit = fetch(withPrefetchCache, 1, addressTag, addressSet, numLines, numSets);

                /* If Cache Hit */
                if (hit)
                {
                    /* withPrefetchCacheHits++; */
                }
                /* If Cache Miss */
                else
                {
                    /* withPrefetchCacheMisses++; */
                    withPrefetchMemoryReads++;
                }
            }
        }


        /* If operation is a Write */
        else if (operation == 'W')
        {
            /* Write Without Prefetching */
            hit = fetch(noPrefetchCache, 0, addressTag, addressSet, numLines, numSets);

            /* If Cache Hit */
            if (hit)
            {
                noPrefetchCacheHits++;
                noPrefetchMemoryWrites++;
            }
            /* If Cache Miss */
            else
            {
                noPrefetchCacheMisses++;
                noPrefetchMemoryReads++;
                noPrefetchMemoryWrites++;
            }


            /* Write With Prefetching */
            hit = fetch(withPrefetchCache, 0, addressTag, addressSet, numLines, numSets);

            /* If Cache Hit */
            if (hit)
            {
                withPrefetchCacheHits++;
                withPrefetchMemoryWrites++;
            }
            /* If Cache Miss */
            else
            {
                withPrefetchCacheMisses++;
                withPrefetchMemoryReads++;
                withPrefetchMemoryWrites++;

                /* Prefetch */
                /* Get the new address by adding the Block Size */
                address += cacheBlockSize;
                /* Get the Tag and the Set from the new Address */
                addressTag = getTag(address, setBits, tagBits, blockOffsetBits);
                addressSet = getSet(address, setBits, tagBits, blockOffsetBits);

                hit = fetch(withPrefetchCache, 1, addressTag, addressSet, numLines, numSets);

                /* If Cache Hit */
                if (hit)
                {
                    /* withPrefetchCacheHits++; */
                }
                /* If Cache Miss */
                else
                {
                    /* withPrefetchCacheMisses++; */
                    withPrefetchMemoryReads++;
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
    printf("no-prefetch\n");
    printCounters(noPrefetchMemoryReads, noPrefetchMemoryWrites, noPrefetchCacheHits, noPrefetchCacheMisses);

    printf("with-prefetch\n");
    printCounters(withPrefetchMemoryReads, withPrefetchMemoryWrites, withPrefetchCacheHits, withPrefetchCacheMisses);

    /* printf("Count: %lu\n", count); */

    /* Close file */
    fclose(traceFP);

    /* Free memory */
    /* Free each Set's Lines */
    for (i = 0; i < numSets; i++)
    {
        /* Free each Line */
        free(noPrefetchCache -> sets[i].lines);
        free(withPrefetchCache -> sets[i].lines);
    }

    /* Free the Caches' hash table of Sets */
    free(noPrefetchCache -> sets);
    free(withPrefetchCache -> sets);

    /* Free the Cache structs */
    free(noPrefetchCache);
    free(withPrefetchCache);


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
        return -1;
    }

    /* Get the second token */
    cacheAssociativityToken = strtok(NULL, ":");

    /* If there is nothing after the ":", improper formatting error */
    if (cacheAssociativityToken == 0)
    {
        return -1;
    }

    /* Get the n-way associativity */
    int n = atoi(cacheAssociativityToken);

    /* If n is not a power of 2, error */
    if (!isPowerOfTwo(n))
    {
        return -1;
    }

    return n;
}


void printCounters(int memoryReads, int memoryWrites, int cacheHits, int cacheMisses)
{
    printf("Memory reads: %d\n", memoryReads);
    printf("Memory writes: %d\n", memoryWrites);
    printf("Cache hits: %d\n", cacheHits);
    printf("Cache misses: %d\n", cacheMisses);
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
