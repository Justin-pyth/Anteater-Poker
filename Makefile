CC := cc
CFLAGS := -Wall -Wextra -std=c11

.PHONY: all clean

all: server client

server: server.o protocol.o game.o bot.o
	$(CC) $(CFLAGS) $^ -o $@

client: client.o protocol.o
	$(CC) $(CFLAGS) $^ -o $@

server.o: server.c protocol.h uds.h
protocol.o: protocol.c protocol.h uds.h
client.o: client.c protocol.h uds.h

clean:
	rm -f server client *.o
