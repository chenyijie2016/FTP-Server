CC = gcc
CFLAGS = -O2 -Wall -std=c99
BIN = server
OBJS = server.o

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) -o server $(OBJS) 

server.o: server.c log.h
	$(CC) $(CFLAGS) -c server.c

.PHONY:clean
clean:
	rm -f *.o $(OBJS)

test:
	./server -port 1234