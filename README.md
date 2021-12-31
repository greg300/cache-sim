# ECE563_Final_Project
Computer Architecture (ECE 332:563) Final Project.

---

## Overview

A L1-L2-DRAM memory hierarchy system remains a frequently-used model for modern CPUs. In this investigation, a simple L1-L2 cache simulator is developed in the C programming language.  C was selected for its low-level access to memory and fast runtime.

The goal of this project is not to simulate memory itself, but rather, to simulate the behavior of the cache hierarchy to demonstrate how various configurations of cache size, associativity, and block size, along with prefetching adjacent memory addresses, impact the performance of a multilevel cache among different benchmarks.

For this investigation, development and testing has taken place on Ubuntu 20.04.2 LTS, running on the Rutgers University iLab machines. The cache simulator is written in the C programming language, compiled using the GNU Compiler Collection (gcc). For generation of memory access traces, Python 3.7+ and Valgrind are required; Valgrind analyzes a program’s memory usage, and Python is used for scraping the output from Valgrind and converting it to a memory trace for the simulator.

## Usage

The usage for running the cache simulator is enumerated below, along with all options / requirements for each parameter:

`$ ./bin/cache-sim l1_cache_size l1_assoc l1_replace_policy l1_block_size l2_cache_size l2_assoc l2_replace_policy l2_block_size trace_file`

* l1_cache_size: int - size of L1 cache in bytes; must be a power of 2
* l1_assoc: str - associativity of L1 cache; can be one of:
    * direct - direct mapped cache
    * assoc - fully associative cache
    * assoc:n - n-way associative cache, where n is a power of 2
* l1_replace_policy: str - L1 cache replacement policy (lru only is supported)
* l1_block_size: int - size of L1 cache block in bytes; must be a power of 2
* l2_cache_size: int - size of L2 cache in bytes; must be a power of 2
* l2_assoc: str - associativity of L2 cache; can be one of:
    * direct - direct mapped cache
    * assoc - fully associative cache
    * assoc:n - n-way associative cache, where n is a power of 2
* l2_replace_policy: str - L2 cache replacement policy (lru only is supported
* l2_block_size: int - size of L2 cache block in bytes; must be a power of 2
* trace_file: str - path to trace file used as input to the simulator

## Commands
1.	Build the cache simulator:

make

2.	Create a memory access trace file from a compiled binary (requires Valgrind):

python mem_trace.py <prog_name>

(Note: may need to use “python3” if “python” references Python 2, e.g. on some macOS installations.)

3.	Run L1 cache evaluation on benchmark “test2.txt”:

./bin/cache-sim 32 direct lru 4 1024 assoc lru 8 tests/test2.txt

./bin/cache-sim 64 direct lru 4 1024 assoc lru 8 tests/test2.txt

./bin/cache-sim 128 direct lru 4 1024 assoc lru 8 tests/test2.txt

./bin/cache-sim 256 direct lru 4 1024 assoc lru 8 tests/test2.txt

./bin/cache-sim 512 direct lru 4 1024 assoc lru 8 tests/test2.txt

4.	Run L1 cache evaluation on benchmark “test3.txt”:

./bin/cache-sim 32 direct lru 4 1024 assoc lru 8 tests/test3.txt

./bin/cache-sim 64 direct lru 4 1024 assoc lru 8 tests/test3.txt

./bin/cache-sim 128 direct lru 4 1024 assoc lru 8 tests/test3.txt

./bin/cache-sim 256 direct lru 4 1024 assoc lru 8 tests/test3.txt

./bin/cache-sim 512 direct lru 4 1024 assoc lru 8 tests/test3.txt

5.	Run L2 cache evaluation on benchmark “test2.txt”:

./bin/cache-sim 32 direct lru 4 1024 assoc:1 lru 8 tests/test2.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:2 lru 8 tests/test2.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:4 lru 8 tests/test2.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:8 lru 8 tests/test2.txt

(For the remaining 12 commands, vary the size [1024] between [2048], [4096], and [8192].)

6.	Run L2 cache evaluation on benchmark “test3.txt”:

./bin/cache-sim 32 direct lru 4 1024 assoc:1 lru 8 tests/test3.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:2 lru 8 tests/test3.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:4 lru 8 tests/test3.txt

./bin/cache-sim 32 direct lru 4 1024 assoc:8 lru 8 tests/test3.txt

(For the remaining 12 commands, vary the size [1024] between [2048], [4096], and [8192].)

7.	Run overall cache evaluation on all benchmarks:

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/test1.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/test2.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/test3.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/test4.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/matrix_mult_test.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/pi_test.txt

./bin/cache-sim 32 direct lru 4 4096 assoc:4 lru 8 tests/looping_test.txt
