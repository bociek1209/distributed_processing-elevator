SOURCES=$(wildcard *.c)
HEADERS=$(SOURCES:.c=.h)
FLAGS=-DDEBUG -g

all: main test

main: $(SOURCES) $(HEADERS)
	mpicc $(SOURCES) $(FLAGS) -o main

clear: clean

clean:
	rm main a.out
					 		#numberOfProceses elevatorCapacity
run: main
	mpirun -default-hostfile hosts -np 10 ./main 20 | tee result.txt

test: main
	mpirun --oversubscribe -np 3 ./main 15 | tee result.txt
