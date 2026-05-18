CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2

.PHONY: all clean

all: server client

server: server.o protocol.o game.o rules.o bot.o
	$(CC) $(CFLAGS) $^ -o $@

client: client.o protocol.o game.o rules.o
	$(CC) $(CFLAGS) $^ -o $@

server.o: server.c protocol.h game.h rules.h uds.h bot.h
protocol.o: protocol.c protocol.h game.h rules.h uds.h
client.o: client.c protocol.h game.h rules.h uds.h
game.o: game.c game.h rules.h uds.h
rules.o: rules.c rules.h uds.h
bot.o: bot.c bot.h game.h rules.h uds.h

clean:
	rm -f server client *.o
