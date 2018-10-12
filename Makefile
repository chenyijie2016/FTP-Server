CC = gcc
CFLAGS = -O2 -Wall
BIN = server
OBJS = server.o

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) -o server $(OBJS) 

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

.PHONY:clean
clean:
	rm -f *.o $(OBJS)