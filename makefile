CC = gcc
CFLAGS = -Wall
TARGETS = library.o server client

all: $(TARGETS)

#library.o: library.o
#	$(CC) $(CFLAGS) -c library.c

server.o: server.c
	$(CC) $(CFLAGS) -c server.c 
	
server : server.o
	$(CC) $(CFLAGS) -o server server.o library.o
	
client.o: client.c
	$(CC) $(CFLAGS) -c client.c 
	
client : client.o
	$(CC) $(CFLAGS) -o client client.o library.o

target: dependencies
	action
	
clean:
	$(RM) *.o
	
clobber:
	$(RM) $(TARGETS)
