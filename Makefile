all: cache-sim

cache-sim: cache-sim.o
	gcc -Wall -Werror -fsanitize=address cache-sim.o -o cache-sim

cache-sim.o: cache-sim.c cache-sim.h
	gcc -Wall -Werror -fsanitize=address -g -c cache-sim.c

clean:
	rm -f *o cache-sim
