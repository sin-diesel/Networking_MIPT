
CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -pthread -I $(IDIR)
SRC_DIR = source
SOURCES = server.c client.c
HEADERS = my_server.h
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all $(OBJECTS)
all: server.o client.o network.o handlers.o
	$(CC) $(CCFLAGS) server.o network.o handlers.o -o server
	$(CC) $(CCFLAGS) client.o network.o handlers.o -o client

server.o: $(SRC_DIR)/server.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/server.c -c -o server.o

client.o: $(SRC_DIR)/client.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/client.c -c -o client.o	

network.o: $(SRC_DIR)/network.c server.o client.o
	$(CC) $(CCFLAGS) $(SRC_DIR)/network.c -c -o network.o

handlers.o: $(SRC_DIR)/handlers.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/handlers.c -c -o handlers.o

.PHONY: clean
clean:
	-rm *.o
	-rm server
	-rm client