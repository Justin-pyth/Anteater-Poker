#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "uds.h"
#include "game.h"

#define MAX_CLIENTS 6
#define PORT 10160 // port 10160
#define BUFFER_SIZE 256

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
} ServerState;

typedef struct {
    int socket_fd;

    uint8_t running;
    uint8_t connected;
    uint8_t my_player_id;

    GameState game;

    char input_buffer[BUFFER_SIZE];
} ClientState;
//server functions
int create_socket(Client *client);

void init_server(ServerState *state);

void add_connection(ServerState *state, Client *client);

void handle_client_communication(ServerState *state, Client *client);

void remove_client(ServerState *state, Client *client);

void cleanup_server(ServerState *state);


//client functions
void init_client_state(ClientState *client);

int connect_to_server (const char *hostname, int port);

void send_data_to_server(ClientState *client, const char *data);

void handle_user_input(ClientState *client);

void receive_data_from_server(ClientState *client);


//shared functions
void error(const char *msg);
void decode(const char *data, GameState *game);
char *encode(const GameState *game, size_t buffer_size);

#endif
