CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2

PKG_CONFIG := pkg-config
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 2>/dev/null)
TAR := $(shell command -v gtar 2>/dev/null || command -v tar)

SRC_DIR := src
BUILD_DIR := $(SRC_DIR)/build
BIN_DIR := bin

PACKAGE := Poker_V1.0_src
SRC_TAR := $(PACKAGE).tar.gz
DIST_DIR := .dist
DIST_STAGE := $(DIST_DIR)/$(PACKAGE)

SERVER_BIN := $(BIN_DIR)/poker_server
SERVER_HEADLESS_BIN := $(BIN_DIR)/poker_server_headless
CLIENT_BIN := $(BIN_DIR)/poker_client
TEST_UNIT_BIN := $(BIN_DIR)/test_unit
TEST_PROTOCOL_BIN := $(BIN_DIR)/test_protocol
TEST_SYSTEM_BIN := $(BIN_DIR)/test_system
TEST_GAME_FLOW_BIN := $(BIN_DIR)/test_game_flow
TEST_SERVER_BIN := $(BIN_DIR)/test_server
TEST_CLIENT_BIN := $(BIN_DIR)/test_client

COMMON_OBJS := \
	$(BUILD_DIR)/protocol.o \
	$(BUILD_DIR)/game.o \
	$(BUILD_DIR)/rules.o \
	$(BUILD_DIR)/bot.o \
	$(BUILD_DIR)/com.o \
	$(BUILD_DIR)/specialCards.o

SERVER_GUI_OBJS := $(BUILD_DIR)/server_gui_main.o $(BUILD_DIR)/server_gui.o $(COMMON_OBJS)
SERVER_HEADLESS_OBJS := $(BUILD_DIR)/server.o $(COMMON_OBJS)
CLIENT_OBJS := \
	$(BUILD_DIR)/client.o \
	$(BUILD_DIR)/gui.o \
	$(BUILD_DIR)/gui_helpers.o \
	$(COMMON_OBJS)
TEST_UNIT_OBJS := $(BUILD_DIR)/test_unit.o $(COMMON_OBJS)
TEST_PROTOCOL_OBJS := $(BUILD_DIR)/test_protocol.o $(COMMON_OBJS)
TEST_SYSTEM_OBJS := $(BUILD_DIR)/test_system.o $(COMMON_OBJS)
TEST_GAME_FLOW_OBJS := $(BUILD_DIR)/test_game_flow.o $(COMMON_OBJS)
TEST_SERVER_OBJS := $(BUILD_DIR)/test_server.o $(COMMON_OBJS)
TEST_CLIENT_OBJS := $(BUILD_DIR)/test_client.o $(COMMON_OBJS)

.PHONY: all test test-unit test-logic test-comm test-gui test_gui \
	test_server test_client clean dirs src-objects tar

all: $(SERVER_BIN) $(SERVER_HEADLESS_BIN) $(CLIENT_BIN) \
	$(TEST_UNIT_BIN) $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN) \
	$(TEST_GAME_FLOW_BIN) $(TEST_SERVER_BIN) $(TEST_CLIENT_BIN)

test: $(TEST_UNIT_BIN) $(TEST_PROTOCOL_BIN) $(TEST_GAME_FLOW_BIN)
	./$(TEST_UNIT_BIN)
	./$(TEST_PROTOCOL_BIN)
	./$(TEST_GAME_FLOW_BIN)

test-unit: $(TEST_UNIT_BIN)
	./$(TEST_UNIT_BIN)

test-logic: $(TEST_UNIT_BIN) $(TEST_GAME_FLOW_BIN)
	./$(TEST_UNIT_BIN)
	./$(TEST_GAME_FLOW_BIN)

test-comm: $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN) $(TEST_SERVER_BIN) $(TEST_CLIENT_BIN)
	./$(TEST_PROTOCOL_BIN)
	./$(TEST_SYSTEM_BIN)

test-gui: $(CLIENT_BIN)
	@echo "GUI client built: $(CLIENT_BIN)"
	@echo "Run it from $(BIN_DIR) with: ./poker_client"

test_gui: test-gui

test_server: $(TEST_SERVER_BIN)
	@echo "Communication test server built: $(TEST_SERVER_BIN)"

test_client: $(TEST_CLIENT_BIN)
	@echo "Communication test client built: $(TEST_CLIENT_BIN)"

src-objects:
	$(MAKE) -C $(SRC_DIR) all

dirs:
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(SERVER_GUI_OBJS) $(GTK_LIBS) -o $@

$(SERVER_HEADLESS_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(SERVER_HEADLESS_OBJS) -o $@

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

$(TEST_SERVER_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_SERVER_OBJS) -o $@

$(TEST_CLIENT_BIN): | dirs src-objects
	$(CC) $(CFLAGS) $(TEST_CLIENT_OBJS) -o $@

clean:
	$(MAKE) -C $(SRC_DIR) clean
	rm -f $(SERVER_BIN) $(SERVER_HEADLESS_BIN) $(CLIENT_BIN) \
	      $(TEST_UNIT_BIN) $(TEST_PROTOCOL_BIN) $(TEST_SYSTEM_BIN) \
	      $(TEST_GAME_FLOW_BIN) $(TEST_SERVER_BIN) $(TEST_CLIENT_BIN)
	rm -f $(SRC_TAR)
	rm -rf $(DIST_DIR)

tar: clean
	mkdir -p $(DIST_STAGE)
	cp -R README.md COPYRIGHT INSTALL Makefile bin doc src $(DIST_STAGE)/
	cp .gitignore $(DIST_STAGE)/ 2>/dev/null || true
	$(TAR) -C $(DIST_DIR) -czf $(SRC_TAR) $(PACKAGE)
	rm -rf $(DIST_DIR)
