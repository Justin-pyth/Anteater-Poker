CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2

.PHONY: all clean

all: server client

server: server.o protocol.o game.o rules.o bot.o com.o
	$(CC) $(CFLAGS) $^ -o $@

client: client.o protocol.o game.o rules.o com.o
	$(CC) $(CFLAGS) $^ -o $@

test_server: test_server.o protocol.o game.o rules.o bot.o com.o
	$(CC) $(CFLAGS) $^ -o $@

test_client: test_client.o protocol.o game.o rules.o com.o
	$(CC) $(CFLAGS) $^ -o $@

server.o: server.c protocol.h game.h rules.h uds.h bot.h
protocol.o: protocol.c protocol.h game.h rules.h uds.h com.h
client.o: client.c protocol.h game.h rules.h uds.h
game.o: game.c game.h rules.h uds.h
rules.o: rules.c rules.h uds.h
bot.o: bot.c bot.h game.h rules.h uds.h
com.o: com.c com.h uds.h
test_server.o: test_server.c protocol.h com.h uds.h
test_client.o: test_client.c protocol.h com.h uds.h

clean:
	rm -f server client test_server test_client *.o
