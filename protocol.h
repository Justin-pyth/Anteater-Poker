#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "uds.h"
#include "game.h"
#include "bot.h"
#include "com.h"

#define MAX_CLIENTS 6
#define PORT 10160 // port 10160
#define BUFFER_SIZE (MAX_PAYLOAD_SIZE + PROTOCOL_HEADER_SIZE) // buffer size for receiving data

typedef struct{
    int id;
    int client_fd;
    int sock_fd;
    int connected;
    struct sockaddr_in address;
    
    char name[32];
} Client;
// Server state structure to hold server information and client list
typedef struct{
    int listen_fd;
    Client clients[MAX_CLIENTS];
    int running;
    
    GameState game;
    Deck deck;

    //non-blocking FSM timer: paces the all-in runout and the inter-hand delay.
    int timer_active;               //whether a timed action is pending
    struct timeval timer_deadline;  //when it should fire
    int time_between_hands;         //seconds left before the next hand starts
} ServerState;

typedef struct {
    int socket_fd;

    uint8_t running;
    uint8_t connected;
    uint8_t my_player_id;

    GameState game;

    char input_buffer[BUFFER_SIZE];
} ClientState;


void init_server(ServerState *state);

void add_connection(ServerState *state, Client *client);

void handle_client_communication(ServerState *state, Client *client);

void hide_card_info_for_others(GameState *game, uint8_t player_id);

void send_to_client(Client *client, const uint8_t *data, uint32_t len);


void remove_client(ServerState *state, Client *client);
void handle_player_disconnect(ServerState *state, Client *client);

void cleanup_server(ServerState *state);

void broadcast_game_state(ServerState *state);
void broadcast_chat_message(ServerState *state, uint8_t sender_id, const char *message);
void broadcast_cd_signal(ServerState *state, uint8_t target_id);
//client functions
void init_client_state(ClientState *client);

int connect_to_server (const char *hostname, int port);

void send_to_server(ClientState *client, const uint8_t *data, uint32_t len);

int handle_server_communication(ClientState *client, Message *data);
void send_action(ClientState *client, const PlayerAction *action);

int create_socket(Client *client);

void handle_after_move(ServerState *state);
void service_timer(ServerState *state); //run the pending timed FSM action (runout tick / new hand)
void start_new_hand(ServerState *state);
void broadcast_move(ServerState *state, uint8_t playerID, MoveType move, uint32_t amount);
void broadcast_time_between_hands(ServerState *state, int seconds);

//shared functions
void error(const char *msg);

#endif
