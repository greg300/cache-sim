BIN=./bin/

all: cache-sim

cache-sim: cache-sim.o
	gcc -Wall -Werror -fsanitize=address cache-sim.o -o cache-sim
	mv cache-sim.o $(BIN)
	mv cache-sim $(BIN)

cache-sim.o: cache-sim.c cache-sim.h
	gcc -Wall -Werror -fsanitize=address -g -c cache-sim.c

clean:
	rm -f bin/*o bin/cache-sim
