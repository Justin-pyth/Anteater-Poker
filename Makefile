CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2
ifeq ($(OS),Windows_NT)
NULL := NUL
else
NULL := /dev/null
endif
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>$(NULL))
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>$(NULL))

.PHONY: all test clean run server_gui

all: server_gui_server client

test: tests/test_game tests/flow_demo
	./tests/test_game
	./tests/flow_demo

server: server.o protocol.o game.o rules.o bot.o com.o specialCards.o
	$(CC) $(CFLAGS) $^ -o $@

server_gui: server_gui_server
	./server_gui_server

server_gui_server: server_gui_main.o server_gui.o protocol.o game.o rules.o bot.o com.o specialCards.o
	$(CC) $(CFLAGS) $^ $(GTK_LIBS) -o $@

client: client.o gui.o gui_helpers.o protocol.o game.o rules.o bot.o com.o specialCards.o
	$(CC) $(CFLAGS) $^ $(GTK_LIBS) -rdynamic -o $@


test_server: test_server.o protocol.o game.o rules.o bot.o com.o specialCards.o
	$(CC) $(CFLAGS) $^ -o $@

test_client: test_client.o protocol.o game.o rules.o bot.o com.o specialCards.o
	$(CC) $(CFLAGS) $^ -o $@

tests/test_game: tests/test_game.o game.o rules.o specialCards.o
	$(CC) $(CFLAGS) $^ -o $@

tests/flow_demo: tests/flow_demo.o game.o rules.o bot.o
	$(CC) $(CFLAGS) $^ -o $@

test_game: tests/test_game
	./tests/test_game

server.o: server.c protocol.h game.h rules.h uds.h bot.h
server_gui_main.o: server.c protocol.h game.h rules.h uds.h bot.h server_gui.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DENABLE_SERVER_GUI -c server.c -o server_gui_main.o
server_gui.o: server_gui.c server_gui.h protocol.h game.h rules.h bot.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c server_gui.c -o server_gui.o
protocol.o: protocol.c protocol.h game.h rules.h uds.h com.h
client.o: client.c gui.h gui_assets.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c client.c -o client.o
gui.o: gui.c gui.h gui_helpers.h gui_assets.h protocol.h com.h uds.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c gui.c -o gui.o
gui_helpers.o: gui_helpers.c gui_helpers.h gui_assets.h protocol.h com.h uds.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c gui_helpers.c -o gui_helpers.o
game.o: game.c game.h rules.h uds.h
rules.o: rules.c rules.h uds.h
bot.o: bot.c bot.h game.h rules.h uds.h
com.o: com.c com.h uds.h
specialCards.o: specialCards.c specialCards.h uds.h game.h rules.h
test_server.o: test_server.c protocol.h com.h uds.h
test_client.o: test_client.c protocol.h com.h uds.h
tests/test_game.o: tests/test_game.c game.h rules.h uds.h
	$(CC) $(CFLAGS) -c tests/test_game.c -o tests/test_game.o
tests/flow_demo.o: tests/flow_demo.c bot.h game.h rules.h uds.h
	$(CC) $(CFLAGS) -c tests/flow_demo.c -o tests/flow_demo.o

ifeq ($(OS),Windows_NT)
clean:
	-@del /Q /F server client gui server_gui_server test_server test_client server.exe client.exe gui.exe server_gui_server.exe test_server.exe test_client.exe *.o tests\*.o tests\test_game tests\flow_demo tests\test_game.exe tests\flow_demo.exe >NUL 2>NUL
else
clean:
	rm -f server client gui server_gui_server test_server test_client tests/test_game tests/flow_demo *.o tests/*.o
endif

run: clean all
	./server
