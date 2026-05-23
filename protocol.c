#include "protocol.h"
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
/************************************************* 
 *Client handling functions, all the game logic will
 * be implemented after  recieve payload data is valid
*****************************************************/
void handle_client_communication(ServerState *state, Client *client)
{
    uint8_t buffer[BUFFER_SIZE];
    ssize_t n = read(client->client_fd, buffer, sizeof(buffer));
    if (n < 0) {
        perror("ERROR reading from socket");
        client->connected = 0;
        return;
    }
    if (n==0){
            client->connected = 0;
            return;
        }
    MessageType type;
    PlayerAction action;
    if (receive_payload(buffer, n, &type, &action) == 0) {
        if (type == MSG_TYPE_PLAYER_ACTION)
        {
            // ********************************************
            // game logic goes here
            //recommendation: attempt the move with validation

            if(tryMove(&state->game, &state->deck, action.playerID, action.move, action.amount))
            {
                if(!state->game.handPlaying && remainingPlayers(&state->game) == 1)
                {
                    //final winner stuff
                }
                broadcast_game_state(state); // After processing the action, broadcast the updated game state to all clients
            }
            else
            {
                //send error back to the client
                uint8_t error_buffer[BUFFER_SIZE];
                uint32_t error_len = prepare_payload(error_buffer, MSG_TYPE_ERROR_MESSAGE, "Invalid move ");
                send_to_client(client, error_buffer, error_len);
            }
            
            /********************************************* */
        }


        
    } else {
        fprintf(stderr, "ERROR processing received payload\n");
     } 

}
//this function will handle server incomming packages and update to the client data.
//DO NOT IMPLEMENT UI and user input in this function. You can but it will not work well with UI.
void handle_server_communication(ClientState *client)
{
    uint8_t buffer[BUFFER_SIZE];
    ssize_t n = read(client->socket_fd, buffer, sizeof(buffer));
    if (n < 0) {
        perror("ERROR reading from socket");
        client->connected = 0;
        return;
    }
    MessageType type;
    if (n==0){
            client->connected = 0;
            return;
        }
    if (receive_payload(buffer, n, &type, &client->game) == 0) {
    } else {
        fprintf(stderr, "ERROR processing received payload\n");
    }

}

int create_socket(Client *client)
{
    (void)client;

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        error("ERROR setting socket options");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, 5) < 0)
        error("ERROR on listen");

    return sockfd;

}
void init_server(ServerState *state)
{
    state->listen_fd = -1;
    state->running = 1; // Set the server running flag to true
    for (int i = 0; i < MAX_CLIENTS; i++) {
        state->clients[i].client_fd = -1;
        state->clients[i].sock_fd = -1;
        state->clients[i].connected = 0; // Initialize all client connections as not connected
        state->clients[i].id = i; // Assign an ID to each client
        memset(&state->clients[i].address, 0, sizeof(state->clients[i].address));
        memset(state->clients[i].name, 0, sizeof(state->clients[i].name));
    }

    //init deck and gamestate
    memset(&state->game, 0, sizeof(GameState));
    initDeck(&state->deck);
}
void add_connection(ServerState *state, Client *client)
{
    struct sockaddr_in address;
    socklen_t clilen = sizeof(address); // Get the size of the client address structure
    int client_fd = accept(state->listen_fd, (struct sockaddr *)&address, &clilen); // Accept the incoming connection
    if (client_fd < 0) {
        perror("ERROR on accept");
        return; // Return if there is an error accepting the connection
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!state->clients[i].connected) {
            state->clients[i].client_fd = client_fd;
            state->clients[i].sock_fd = client_fd;
            state->clients[i].connected = 1; // Mark the client as connected
            state->clients[i].address = address;
            state->clients[i].id = i; // Assign the matching 0-based player index

            //create a new player on gamestate
            initPlayer(&state->game.players[i], i, "Player", 1000);
            state->game.playerCount++;

            if (client != NULL)
                *client = state->clients[i];

            printf("New client connected: %d\n", state->clients[i].id ); // Print a message indicating a new client has connected
            if (!state->game.handPlaying && state->game.playerCount >= 1) {
                addBot(&state->game);
                newHand(&state->game, &state->deck);
            }
            broadcast_game_state(state);
            return;
        }
    }

    fprintf(stderr, "Server full; rejecting connection\n");
    close(client_fd);
}
// this function will not send to full game state. But rather sending raw bytes to client socket
void send_to_client(Client *client, const uint8_t *data, uint32_t len)
{
    ssize_t n = write(client->client_fd, data, len);
    if (n < 0) {
        perror("ERROR writing to socket");
        client->connected = 0;
    }
}
void hide_card_info_for_others(GameState *game, uint8_t player_id)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].id != player_id)
           { game->players[i].hand[0].rank = UNKNOW_R;
            game->players[i].hand[0].suit = UNKNOW_S;
            game->players[i].hand[1].rank = UNKNOW_R;
            game->players[i].hand[1].suit = UNKNOW_S;
            }
           }
    
}
void broadcast_game_state(ServerState *state)
{
    uint8_t buffer[BUFFER_SIZE];
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected) {
            GameState tempGame = state->game; // Create a temporary copy of the game state
            hide_card_info_for_others(&tempGame, state->clients[i].id); //
            uint32_t payload_len = prepare_payload(buffer, MSG_TYPE_GAME_STATE, &tempGame); // Prepare the payload with the modified game state
            send_to_client(&state->clients[i], buffer, payload_len);
        }
    }
}
void send_to_server(ClientState *client, const uint8_t *data, uint32_t len)
{
    
    ssize_t n = write(client->socket_fd, data, len);
    if (n < 0) {
        perror("ERROR writing to socket");
        client->connected = 0;
    }

}
void send_action(ClientState *client, const PlayerAction *action)
{
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_PLAYER_ACTION, action);
    send_to_server(client, buffer, len);
}
void remove_client(ServerState *state, Client *client)
{
    // Remove a client from the server state and close the connection
    printf("Client disconnected: %d\n", client->id); // Print a message indicating the client has disconnected
    close(client->client_fd); // Close the client's socket connection
    if (client->id >= 0 && client->id < MAX_PLAYERS) {
        state->game.players[client->id].status = PLAYER_DISCONNECTED;
        if (state->game.playerCount > 0)
            state->game.playerCount--;
    }
    client->client_fd = -1;
    client->sock_fd = -1;
    client->connected = 0; // Mark the client as not connected
}
void cleanup_server(ServerState *state)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected)
            remove_client(state, &state->clients[i]);
    }

    if (state->listen_fd >= 0) {
        close(state->listen_fd);
        state->listen_fd = -1;
    }
}
void init_client_state(ClientState *client)
{
    client->connected = 0; // Initialize the client state as not connected
    client->running = 1;
    client->socket_fd = -1; // Initialize the client socket file descriptor to an invalid value
    client->my_player_id = 0;
    memset(&client->game, 0, sizeof(client->game));
    memset(client->input_buffer, 0, sizeof(client->input_buffer)); // Clear the client's input buffer
}
int connect_to_server(const char *hostname, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a new socket
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1; // Return -1 if there is an error opening the socket
    }
    server = gethostbyname(hostname); // Get the server's host information
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1; // Return -1 if the server host cannot be found
    }
    memset(&serv_addr, 0, sizeof(serv_addr)); // Zero out the server address structure
    serv_addr.sin_family = AF_INET; // Set the address family to AF_INET
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length); // Copy the server's IP address to the server address structure
    serv_addr.sin_port = htons(port); // Set the server port number
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        return -1; // Return -1 if there is an error connecting to the server
    }
    return sockfd; // Return the socket file descriptor for the connected client

}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}
