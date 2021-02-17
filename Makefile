
CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -I $(IDIR)
SRC_DIR = source
SOURCES = server.c client.c
HEADERS = my_server.h
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all $(OBJECTS)
all: server.o client.o
	$(CC) $(CCFLAGS) server.o -o server
	$(CC) $(CCFLAGS) client.o -o client

server.o: $(SRC_DIR)/server.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/server.c -c -o server.o

client.o: $(SRC_DIR)/client.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/client.c -c -o client.o	


.PHONY: clean
clean:
	-rm *.o
	-rm server
	-rm client