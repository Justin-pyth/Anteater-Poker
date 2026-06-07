CC := cc
CFLAGS := -Wall -Wextra -std=c11 -g -O2

PKG_CONFIG := pkg-config
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 2>/dev/null)

BUILD_DIR ?= build

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

ALL_OBJS := \
	$(SERVER_GUI_OBJS) \
	$(SERVER_HEADLESS_OBJS) \
	$(CLIENT_OBJS) \
	$(TEST_UNIT_OBJS) \
	$(TEST_PROTOCOL_OBJS) \
	$(TEST_SYSTEM_OBJS) \
	$(TEST_GAME_FLOW_OBJS) \
	$(TEST_SERVER_OBJS) \
	$(TEST_CLIENT_OBJS)

.PHONY: all clean dirs

all: $(ALL_OBJS)

dirs:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server_gui_main.o: server.c protocol.h game.h rules.h uds.h bot.h server_gui.h | dirs
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DENABLE_SERVER_GUI -c $< -o $@

$(BUILD_DIR)/server_gui.o: server_gui.c server_gui.h protocol.h game.h rules.h bot.h | dirs
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/client.o: client.c gui.h gui_assets.h protocol.h | dirs
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui.o: gui.c gui.h gui_helpers.h gui_assets.h protocol.h com.h uds.h | dirs
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui_helpers.o: gui_helpers.c gui_helpers.h gui_assets.h protocol.h com.h uds.h | dirs
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/server.o: server.c protocol.h game.h rules.h uds.h bot.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/protocol.o: protocol.c protocol.h game.h rules.h uds.h com.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/game.o: game.c game.h rules.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/rules.o: rules.c rules.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bot.o: bot.c bot.h game.h rules.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/com.o: com.c com.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/specialCards.o: specialCards.c specialCards.h uds.h game.h rules.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_unit.o: tests/test_unit.c game.h rules.h uds.h | dirs
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(BUILD_DIR)/test_protocol.o: tests/test_protocol.c com.h uds.h | dirs
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(BUILD_DIR)/test_system.o: tests/test_system.c protocol.h com.h game.h uds.h | dirs
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(BUILD_DIR)/test_game_flow.o: tests/test_game_flow.c game.h rules.h bot.h uds.h | dirs
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(BUILD_DIR)/test_server.o: test_server.c protocol.h com.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_client.o: test_client.c protocol.h com.h uds.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
