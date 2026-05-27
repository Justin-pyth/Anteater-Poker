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
    Message data;
    if (receive_payload(buffer, n,  &data) == 0) {
        if (data.type == MSG_TYPE_PLAYER_ACTION)
        {
            // ********************************************
            // game logic goes here
            //recommendation: attempt the move with validation
            
            if(state->game.gameOver) //if the game is already over, do not let moves go through
            {
                broadcast_game_state(state);
                return;
            }

            if(tryMove(&state->game, &state->deck, data.action.playerID, data.action.move, data.action.amount))
            {   
                        
                handle_after_move(state);
              
            }
            else
            {
                //send error back to the client
                uint8_t error_buffer[BUFFER_SIZE];
                Message error_message;
                strncpy(error_message.error, "Invalid move", MAX_PAYLOAD_SIZE);
                error_message.type = MSG_TYPE_ERROR_MESSAGE;
                uint32_t error_len = prepare_payload(error_buffer, MSG_TYPE_ERROR_MESSAGE, &error_message);
               
                send_to_client(client, error_buffer, error_len);
            }
            
            /********************************************* */
        }
        else if (data.type == MSG_TYPE_CHAT_MESSAGE)
        {
            
            broadcast_chat_message(state, data.sender_id, data.chat);
        }
           else if (data.type == MSG_TYPE_READY)
        {
            //don't allow a ready to occur while gaming is playing
            if (state->game.handPlaying) {
                broadcast_game_state(state);
                return;
            }

            //player sends /ready
            state->game.players[client->id].status = PLAYER_READY;

            //server counts # of ready vs connected
            int connectedClients = 0;
            int readyClients = 0;
            for (int i = 0; i < MAX_PLAYERS; i++) 
            {
                if (state->clients[i].connected) 
                {
                    connectedClients++;

                if (state->game.players[i].status == PLAYER_READY)
                    readyClients++;
                }
            }


            //if everyone readies up, start the hand
            if((readyClients == connectedClients) && readyClients >= 1)
            {
                //if the game is over
                if(state->game.gameOver)
                {
                    //reset it
                    resetGame(&state->game);
                }

                //make every connected player ready again
                for(int i = 0; i < MAX_PLAYERS; i++)
                {
                    if(state->clients[i].connected)
                        state->game.players[i].status = PLAYER_READY;
                    else
                    {
                        memset(&state->game.players[i], 0, sizeof(Player));
                        state->game.players[i].id = i;
                        state->game.players[i].status = PLAYER_EMPTY;
                    }
                }

                //fill empty seats with bots
                addBot(&state->game);
                newHand(&state->game, &state->deck);    //start new hand
                broadcast_cd_signal(state, state->game.currentPlayer); //send countdown signal to the next player
            }

            broadcast_game_state(state);
        }

        
    } else {
        fprintf(stderr, "ERROR processing received payload\n");
     } 

}

//this function will handle server incomming packages and update to the client data.
//DO NOT IMPLEMENT UI and user input in this function. You can but it will not work well with UI.
int handle_server_communication(ClientState *client, Message *data)
{
    uint8_t buffer[BUFFER_SIZE];
    ssize_t n = read(client->socket_fd, buffer, sizeof(buffer));
    if (n < 0) { perror("ERROR reading from socket"); client->connected = 0; return -1; }
    if (n == 0) { client->connected = 0; return -1; }

    if (receive_payload(buffer, n, data) != 0) {
        fprintf(stderr, "ERROR processing received payload\n");
        return -1;
    }
    return 0;
}

void send_action(ClientState *client, const PlayerAction *action)
{   
    Message data;
    data.type = MSG_TYPE_PLAYER_ACTION;
    data.action = *action;
    uint8_t buffer[BUFFER_SIZE];
    uint32_t len = prepare_payload(buffer, MSG_TYPE_PLAYER_ACTION, &data);
    send_to_server(client, buffer, len);
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
            initPlayer(&state->game.players[i], i, "Player", INIT_CHIPS);
            if (state->game.handPlaying) 
                state->game.players[i].status = PLAYER_SPECTATING;//set to spectating if hand is playing
            else
                state->game.players[i].status = PLAYER_CONNECTED; //if the client is a player, then set them as connected instead of bot default(READY)

            state->game.playerCount++;

            if (client != NULL)
                *client = state->clients[i];

            printf("New client connected: %d\n", state->clients[i].id ); // Print a message indicating a new client has connected
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
    Message temp_data;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected) {
            temp_data.gameState = state->game; // Create a temporary copy of the game state
            temp_data.gameState.yourPlayerID = state->clients[i].id;
            hide_card_info_for_others(&temp_data.gameState, state->clients[i].id); //
            temp_data.type = MSG_TYPE_GAME_STATE;
            uint32_t payload_len = prepare_payload(buffer, MSG_TYPE_GAME_STATE, &temp_data);
            send_to_client(&state->clients[i], buffer, payload_len);
        }
    }
}
void broadcast_chat_message(ServerState *state, uint8_t sender_id, const char *message)
{
    uint8_t buffer[BUFFER_SIZE];
    Message temp_data;
    temp_data.sender_id = sender_id;
    strncpy(temp_data.chat, message, MAX_PAYLOAD_SIZE - 1);
    temp_data.chat[MAX_PAYLOAD_SIZE - 1] = '\0'; // Ensure null-termination of the chat message
    uint32_t payload_len = prepare_payload(buffer, MSG_TYPE_CHAT_MESSAGE, &temp_data); // Prepare the payload with the chat message
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected) {
            send_to_client(&state->clients[i], buffer, payload_len);
        }
    }
}
void broadcast_cd_signal(ServerState *state, uint8_t target_id)
{
    uint8_t buffer[BUFFER_SIZE];
    Message temp_data;
    temp_data.sender_id = target_id; // Use sender_id to indicate the target player for the countdown signal
    temp_data.type = MSG_CD_SIGNAL;
    uint32_t payload_len = prepare_payload(buffer, MSG_CD_SIGNAL, &temp_data); // Prepare the payload with the countdown signal
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected) {
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
//return -1 if game over or invalid, next player id ow.
void handle_after_move(ServerState *state)
{   
    //don't let moves go through if game is already over
    if(state->game.gameOver)
    {
        broadcast_game_state(state);
        return;
    }

    //if the game is not active and a move is made
    if (!state->game.handPlaying)
    {
        //check for winner
        if (remainingPlayers(&state->game) == 1)
        {
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                Player *p = &state->game.players[i];

                if (p->chips > 0 &&
                    p->status != PLAYER_EMPTY &&
                    p->status != PLAYER_DISCONNECTED &&
                    p->status != PLAYER_SPECTATING)
                {
                    state->game.winnerID = p->id;
                    break;
                }
            }
            
            //find the winner above and set gameOver to true
            state->game.gameOver = 1;
            broadcast_game_state(state);
            return;
        }

        //if there is no winner after the move, then do a new hand
        newHand(&state->game, &state->deck);
    }

    //if the game is active, then just broadcast the state, otherwise
    //have to reset the hand if game is not active
    
    broadcast_cd_signal(state, state->game.currentPlayer); //send countdown signal to the next player
    broadcast_game_state(state);

}