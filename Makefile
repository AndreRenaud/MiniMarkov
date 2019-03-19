CFLAGS=-g -Wall -pipe -O0

default: markov_test

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

markov_test: markov.o markov_test.o
	$(CC) -o $@ markov.o markov_test.o

clean:
	rm -f *.o markov_test