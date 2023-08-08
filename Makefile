CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic -O2

all: httpserver

httpserver: asgn4.o queue.o ll.o node.o hash.o
	$(CC) -pthread -o httpserver asgn4.o queue.o ll.o node.o hash.o

queue.o: queue.c
	$(CC) $(CFLAGS) -c queue.c

ll.o: ll.c
	$(CC) $(CFLAGS) -c ll.c

hash.o: hash.c
	$(CC) $(CFLAGS) -c hash.c

node.o: node.c
	$(CC) $(CFLAGS) -c node.c

asgn4.o: asgn4.c
	$(CC) $(CFLAGS) -c asgn4.c

clean:
	rm -f httpserver asgn4.o queue.o ll.o node.o hash.o
