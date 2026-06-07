CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2

PKG_CONFIG := pkg-config
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 2>/dev/null)

SRC_DIR ?= src
BUILD_DIR := $(SRC_DIR)/build
BIN_DIR ?= bin

SERVER_BIN := $(BIN_DIR)/poker_server
CLIENT_BIN := $(BIN_DIR)/poker_client
TEST_UNIT_BIN := $(BIN_DIR)/test_unit
TEST_PROTOCOL_BIN := $(BIN_DIR)/test_protocol
TEST_SYSTEM_BIN := $(BIN_DIR)/test_system
TEST_GAME_FLOW_BIN := $(BIN_DIR)/test_game_flow

COMMON_OBJS := \
	$(BUILD_DIR)/protocol.o \
	$(BUILD_DIR)/game.o \
	$(BUILD_DIR)/rules.o \
	$(BUILD_DIR)/bot.o \
	$(BUILD_DIR)/com.o

SERVER_OBJS := $(BUILD_DIR)/server.o $(COMMON_OBJS)
CLIENT_OBJS := \
	$(BUILD_DIR)/client.o \
	$(BUILD_DIR)/gui.o \
	$(BUILD_DIR)/gui_helpers.o \
	$(COMMON_OBJS)
TEST_UNIT_OBJS := $(BUILD_DIR)/test_unit.o $(BUILD_DIR)/game.o $(BUILD_DIR)/rules.o
TEST_PROTOCOL_OBJS := $(BUILD_DIR)/test_protocol.o $(BUILD_DIR)/protocol.o $(BUILD_DIR)/com.o $(BUILD_DIR)/game.o $(BUILD_DIR)/rules.o $(BUILD_DIR)/bot.o
TEST_SYSTEM_OBJS := $(BUILD_DIR)/test_system.o $(BUILD_DIR)/protocol.o $(BUILD_DIR)/com.o $(BUILD_DIR)/game.o $(BUILD_DIR)/rules.o $(BUILD_DIR)/bot.o
TEST_GAME_FLOW_OBJS := $(BUILD_DIR)/test_game_flow.o $(BUILD_DIR)/game.o $(BUILD_DIR)/rules.o $(BUILD_DIR)/bot.o

.PHONY: all test test-unit test-logic test-comm test-gui clean dirs src-objects tar

all: $(SERVER_BIN) $(CLIENT_BIN) $(TEST_UNIT_BIN) $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN) $(TEST_GAME_FLOW_BIN)

test: $(TEST_UNIT_BIN) $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN) $(TEST_GAME_FLOW_BIN)
	./$(TEST_UNIT_BIN)
	./$(TEST_PROTOCOL_BIN)
	./$(TEST_SYSTEM_BIN)
	./$(TEST_GAME_FLOW_BIN)

test-unit: $(TEST_UNIT_BIN)
	./$(TEST_UNIT_BIN)

test-logic: $(TEST_UNIT_BIN) $(TEST_GAME_FLOW_BIN)
	./$(TEST_UNIT_BIN)
	./$(TEST_GAME_FLOW_BIN)

test-comm: $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN)
	./$(TEST_PROTOCOL_BIN)
	./$(TEST_SYSTEM_BIN)

test-gui: $(CLIENT_BIN)
	./$(CLIENT_BIN)

src-objects:
	$(MAKE) -C $(SRC_DIR) all

dirs:
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $@

$(CLIENT_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(CLIENT_OBJS) $(GTK_LIBS) -rdynamic -o $@

$(TEST_UNIT_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_UNIT_OBJS) -o $@

$(TEST_PROTOCOL_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_PROTOCOL_OBJS) -o $@

$(TEST_SYSTEM_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_SYSTEM_OBJS) -o $@

$(TEST_GAME_FLOW_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_GAME_FLOW_OBJS) -o $@

clean:
	$(MAKE) -C $(SRC_DIR) clean
	rm -rf $(BIN_DIR)

tar: clean
	mkdir -p $(SRC_STAGE)/src $(SRC_STAGE)/bin $(SRC_STAGE)/doc
	cp README README.md COPYRIGHT INSTALL $(SRC_STAGE)/ 2>/dev/null || true
	cp Makefile $(SRC_STAGE)/
	cp $(SRC_DIR)/Makefile $(SRC_STAGE)/src/Makefile
	cp $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(SRC_STAGE)/src/
	cp *.glade *.css $(SRC_STAGE)/ 2>/dev/null || true
	cp -R img $(SRC_STAGE)/ 2>/dev/null || true
	cp doc/*.pdf $(SRC_STAGE)/doc/ 2>/dev/null || true
	cp Poker_UserManual.pdf Poker_SoftwareSpec.pdf $(SRC_STAGE)/doc/ 2>/dev/null || true
	tar -czf $(SRC_TAR) $(SRC_STAGE)
	rm -rf $(SRC_STAGE)

