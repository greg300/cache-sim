/*
 * =====================================================================================
 *
 *       Filename:  cache-sim.h
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

typedef struct line Line;
struct line
{
    int valid;
    unsigned long long int tag;
    unsigned long int block;
    unsigned long int usage;
};

typedef struct set Set;
struct set
{
    int numItems;
    Line *lines;
};

typedef struct cache Cache;
struct cache
{
    Set *sets;
    int size;
    int blockSize;
    int associativity;
    int nSets;
};

int hash(unsigned long long int n, int size)
{
    return n % size;
}
