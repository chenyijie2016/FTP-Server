CC = gcc
CFLAGS = -O2 -Wall -std=c99
BIN = server
OBJS = server.o handle.o common.o log.o
LIBS = -pthread
$(BIN):$(OBJS)
	$(CC) $(LIBS) $(CFLAGS) -o server $(OBJS) 
	rm -f *.o

server.o: server.c log.h common.h 
	$(CC) $(CFLAGS) -c server.c
handle.o : handle.c common.h
	$(CC) $(CFLAGS) -c handle.c
common.o: common.c
	$(CC) $(CFLAGS) -c common.c
log.o: log.c log.h
	$(CC) $(CFLAGS) -c log.c
.PHONY:clean
clean:
	rm -f *.o $(OBJS)

test:
	./server -port 1236