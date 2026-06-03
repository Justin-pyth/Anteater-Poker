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
        handle_player_disconnect(state, client);
        return;
    }
    if (n==0){
        handle_player_disconnect(state, client);
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
                //commented out below (most likely a bug)
                //apply(&state->game, data.action.playerID, data.action.move, data.action.amount); //apply the move on the server if valid
                //processMove(&state->game, &state->deck, data.action.playerID); //FSM transition: apply already done, decide next phase
                broadcast_move(state, data.action.playerID, data.action.move, data.action.amount);
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
        else if (data.type == MSG_TYPE_JOIN)
        {
            char name[MAX_NAME_LENTH];
            int i;
            //iterate until u hit terminator character while creating name char by char
            for (i = 0; i < MAX_NAME_LENTH - 1 && data.chat[i] != '\0'; i++)
                name[i] = data.chat[i];
            name[i] = '\0';//add terminator char

            strcpy(state->clients[client->id].name, name);
            strcpy(state->game.players[client->id].name, name);

            //welcome message
            char welcome[MAX_PAYLOAD_SIZE];
            snprintf(welcome, sizeof(welcome), "Welcome to Anteater Poker %s!", name);
            broadcast_chat_message(state, MAX_PLAYERS, welcome);
            broadcast_game_state(state);
        }
        else if (data.type == MSG_TYPE_READY)
        {
            //don't allow a ready to occur while gaming is playing
            if (state->game.handPlaying) {
                broadcast_game_state(state);
                return;
            }

            //if any player sends a ready message, check for disconnected players and set them as empty
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (!state->clients[i].connected &&
                    state->game.players[i].status == PLAYER_DISCONNECTED)
                {
                    memset(&state->game.players[i], 0, sizeof(Player));
                    state->game.players[i].id = i;
                    state->game.players[i].status = PLAYER_EMPTY;
                }
            }

            //player sends /ready (set busted players as spectatating)
            state->game.players[client->id].status = PLAYER_READY;

            //whenever someone readies, send a public broadcast
            const char *name = state->game.players[client->id].name;
            char msg[MAX_PAYLOAD_SIZE];
            //is ready if not busted, otherwise spectating
            snprintf(msg, sizeof(msg), "%s is ready!", name);
            broadcast_chat_message(state, MAX_PLAYERS, msg);

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
                addBot(&state->game, true);
                newHand(&state->game, &state->deck);    //start new hand
                broadcast_game_state(state);
                broadcast_cd_signal(state, state->game.currentPlayer); //send countdown signal to the next player
                return;
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

    //no timed FSM action pending yet
    state->timer_active = 0;
    state->time_between_hands = 0;
    memset(&state->timer_deadline, 0, sizeof(state->timer_deadline));

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
void hide_card_info_for_others(GameState *client_state, uint8_t recipient_id)
{
    if (recipient_id >= MAX_PLAYERS ||
        client_state->players[recipient_id].status == PLAYER_SPECTATING)
        return;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Find everyone who is NOT the person receiving this network packet
        if (client_state->players[i].id != recipient_id) {
            
            // Only hide cards for players who are actively playing or folded during the live hand.
            // If they are empty or spectating, their cards don't matter.
            uint8_t status = client_state->players[i].status;
            if (status == PLAYER_PLAYING || status == PLAYER_FOLDED || status == PLAYER_ALL_IN) {
                for (int c = 0; c < HAND_SIZE; c++) {
                    client_state->players[i].hand[c].suit = UNKNOW_S;
                    client_state->players[i].hand[c].rank = UNKNOW_R;
                    client_state->players[i].hand[c].value = 0;
                }
            }
        }
    }
}
void broadcast_game_state(ServerState *state)
{
    uint8_t buffer[BUFFER_SIZE];
    Message temp_data;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].connected) {
            temp_data.gameState = state->game; 
            temp_data.gameState.yourPlayerID = state->clients[i].id;
            
            // Hide hole cards only while betting is still live (HAND_BETTING).
            // Once betting is settled the hand enters HAND_RUNOUT/HAND_COMPLETE and
            // all remaining hands are turned face-up so the all-in runout and the
            // showdown are visible as the board rolls out.
            if (state->game.phase == HAND_BETTING) {
                hide_card_info_for_others(&temp_data.gameState, state->clients[i].id);
            }
            
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

void handle_player_disconnect(ServerState *state, Client *client)
{
    uint8_t playerID = client->id;
    bool wasPlaying = state->game.handPlaying; //track if the hand was active
    bool wasCurrent = wasPlaying && state->game.currentPlayer == playerID; //track the current player

    remove_client(state, client);

    //if the game wasn't active, then just broadcast the player leaving
    if (!wasPlaying) 
    {
        broadcast_game_state(state);
        return;
    }

    //if the game was active
    int activeIDs[MAX_PLAYERS];
    int activeCount = findActive(&state->game, activeIDs, true);
    if (activeCount <= 1 || wasCurrent) // and the player who left was the current player
    {   //handle their move
        processMove(&state->game, &state->deck, playerID);
        handle_after_move(state);
        return;
    }
    //if the hand was playing, then just broadcast
    broadcast_game_state(state);
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
#define RUNOUT_TICK_MS             1000  //one community card revealed per second during an all-in runout
#define TIME_BETWEEN_HANDS_SECONDS   5
#define INTERHAND_MS               1000  //one countdown tick before the next hand

//arm the non-blocking FSM timer to fire `ms` milliseconds from now.
static void arm_timer(ServerState *state, long ms)
{
    gettimeofday(&state->timer_deadline, NULL);
    state->timer_deadline.tv_sec  += ms / 1000;
    state->timer_deadline.tv_usec += (ms % 1000) * 1000;
    if (state->timer_deadline.tv_usec >= 1000000) {
        state->timer_deadline.tv_sec  += 1;
        state->timer_deadline.tv_usec -= 1000000;
    }
    state->timer_active = 1;
}

void broadcast_time_between_hands(ServerState *state, int seconds)
{
    char msg[MAX_PAYLOAD_SIZE];
    snprintf(msg, sizeof(msg), "Next hand in %d...", seconds);
    broadcast_chat_message(state, MAX_PLAYERS, msg);
}

void broadcast_side_pot_summary(ServerState *state)
{
    //this function pretty much mirrors awardSidePot from game
    //but it just creates a message to send to the chat
    //**** LOOK AT GAME.C for AWARD SIDE POT FUNCTION  */
    GameState *g = &state->game;
    uint32_t threshold[MAX_PLAYERS];
    int thresholdCount = 0;
    int activeIDs[MAX_PLAYERS];
    int activeCount = findActive(g, activeIDs, true);
    bool hasAllIn = false;

    //side pots cannot be active with 2 players
    if (activeCount < 2) return;

    //side pots only exist when there is atleast one allin player
    for (int i = 0; i < activeCount; i++)
    {
        if (g->players[activeIDs[i]].status == PLAYER_ALL_IN)
        {
            hasAllIn = true;
            break;
        }
    }
    if (!hasAllIn) return;

    //create a list of unique contribution thresholds
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        uint32_t bet = g->players[i].total_bet;
        if (bet == 0) continue;

        bool dupe = false;
        for (int j = 0; j < thresholdCount; j++)
        {
            if (threshold[j] == bet)
            {
                dupe = true;
                break;
            }
        }
        if (!dupe)
            threshold[thresholdCount++] = bet;
    }

    //ignore the first threshold (aka normal situation that would not require sidepots)
    if (thresholdCount <= 1)
        return;

    //bubble sort ascending
    for (int i = 0; i < thresholdCount - 1; i++)
    {
        for (int j = 0; j < thresholdCount - i - 1; j++)
        {
            if (threshold[j] > threshold[j + 1])
            {
                uint32_t temp = threshold[j];
                threshold[j] = threshold[j + 1];
                threshold[j + 1] = temp;
            }
        }
    }

    //find the eligible players
    uint32_t previousThreshold = threshold[0];
    for (int i = 1; i < thresholdCount; i++)
    {
        uint32_t thresholdBet = threshold[i];
        uint32_t slice = thresholdBet - previousThreshold;
        uint32_t sidePot = 0;
        char msg[MAX_PAYLOAD_SIZE];
        int write;
        int eligCount = 0;

        for (int p = 0; p < MAX_PLAYERS; p++)
        {
            if (g->players[p].total_bet >= thresholdBet)
                sidePot += slice;
        }

        if (sidePot == 0)
        {
            previousThreshold = thresholdBet;
            continue;
        }

        write = snprintf(msg, sizeof(msg), "Side Pot [$%u]; Eligible Players: ", sidePot);
        if (write < 0 || (size_t)write >= sizeof(msg))
            return;

        for (int p = 0; p < activeCount; p++)
        {
            Player *player = &g->players[activeIDs[p]];
            //player must be past threshold bet and playing(not folded)
            if (player->total_bet < thresholdBet) continue;

            write += snprintf(msg + write, sizeof(msg) - (size_t)write,  "%s%s", eligCount ? ", " : "", player->name[0] ? player->name : "Player");
            if ((size_t)write >= sizeof(msg))
            {
                msg[sizeof(msg) - 1] = '\0'; //terminator
                break;
            }
            eligCount++;
        }

        if (eligCount >= 2 && eligCount < activeCount)
            broadcast_chat_message(state, MAX_PLAYERS, msg);

        previousThreshold = thresholdBet;
    }
}

//the hand has reached HAND_COMPLETE: announce the result, end the tournament if
//only one player has chips left, otherwise arm the inter-hand delay.
static void finish_hand(ServerState *state)
{
    GameState *g = &state->game;

    //tournament over: a single player holds all the chips
    if (remainingPlayers(g) == 1)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            Player *p = &g->players[i];
            if (p->chips > 0 &&
                p->status != PLAYER_EMPTY &&
                p->status != PLAYER_DISCONNECTED)
            {
                g->winnerID = p->id;
                p->place = 1; //set the winner's place
                break;
            }
        }

        g->gameOver = 1;
        if (g->winnerID < MAX_PLAYERS) {
            char msg[MAX_PAYLOAD_SIZE];
            snprintf(msg, sizeof(msg), "%s wins the game!", g->players[g->winnerID].name);
            broadcast_chat_message(state, MAX_PLAYERS, msg);
        }
        broadcast_game_state(state);
        return; //no timer: game is over, wait for a new game
    }

    //hand winner announcement for chat
    //if a single winner, then output their name
    if (g->winnerCount == 1)
    {
        char msg[MAX_PAYLOAD_SIZE];
        snprintf(msg, sizeof(msg), "%s wins the hand.", g->players[g->winnerID].name);
        broadcast_chat_message(state, MAX_PLAYERS, msg);
    } //fallback for multiple winners **Note that the animated announcement will have highest earner as winner
    else if (g->winnerCount > 1)
    {
        char msg[MAX_PAYLOAD_SIZE];
        snprintf(msg, sizeof(msg), "%s earned the most points but several players won a pot.", g->players[g->winnerID].name);
        broadcast_chat_message(state, MAX_PLAYERS, msg);
    }
    
    broadcast_game_state(state);

    //non-blocking countdown before the next hand
    broadcast_side_pot_summary(state);
    state->time_between_hands = TIME_BETWEEN_HANDS_SECONDS;
    broadcast_time_between_hands(state, state->time_between_hands); //show initial time(starts at 5s)
    state->time_between_hands--; //decrement
    arm_timer(state, INTERHAND_MS); //start the 1s timer
}

//dispatch after a move (or a current player leaving) based on the hand FSM phase.
void handle_after_move(ServerState *state)
{
    GameState *g = &state->game;

    //don't let anything proceed once the game is over
    if (g->gameOver)
    {
        broadcast_game_state(state);
        return;
    }

    switch (g->phase)
    {
        case HAND_BETTING:
            //normal turn hand-off: show state, prompt the next player
            broadcast_game_state(state);
            broadcast_cd_signal(state, g->currentPlayer);
            break;

        case HAND_RUNOUT:
            //all-in: show the board so far, then let the timer pace the reveal
            broadcast_game_state(state);
            arm_timer(state, RUNOUT_TICK_MS);
            break;

        case HAND_COMPLETE:
            //hand decided: announce winner, end game or arm the inter-hand delay
            finish_hand(state);
            break;

        case HAND_IDLE:
        default:
            broadcast_game_state(state);
            break;
    }
}

//run the pending timed FSM action. Called by the server loop when the timer fires.
void service_timer(ServerState *state)
{
    GameState *g = &state->game;
    state->timer_active = 0; //consume it; re-armed below if more pacing is needed

    if (g->phase == HAND_RUNOUT)
    {
        bool more = runoutStep(g, &state->deck);
        if (more)
        {
            broadcast_game_state(state); //show the freshly revealed street
            arm_timer(state, RUNOUT_TICK_MS);
        }
        else
        {
            finish_hand(state); //runout reached the showdown -> result + inter-hand delay
        }
        return;
    }

    if (g->phase == HAND_COMPLETE)
    {
        //once a hand is complete, check if there is still a cooldown for next round
        if (state->time_between_hands > 0)
        {
            //broadcast it
            broadcast_time_between_hands(state, state->time_between_hands);
            //decrement (5->4->3->2->1)
            state->time_between_hands--;
            //start 1s timer
            arm_timer(state, INTERHAND_MS);
            return;
        }

        //timer ticked from 5->0s at an interval of 1s per new timer
        start_new_hand(state); //start next hand
        return;
    }
}

void start_new_hand(ServerState *state)
{
    if (state->game.gameOver) {
        broadcast_game_state(state);
        return;
    }

    resetHand(&state->game);

    // Re-sync human seats with connection state, but keep existing bots in
    // their seats between hands. Bot names only shuffle on a new game.
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->clients[i].connected && state->game.players[i].chips > 0)
            state->game.players[i].status = PLAYER_READY;
        else if (isBot(state->game.players[i].name)) {
            if (state->game.players[i].chips > 0)
                state->game.players[i].status = PLAYER_READY;
            else
                state->game.players[i].status = PLAYER_SPECTATING;
        }
        else if (!state->clients[i].connected) {
            memset(&state->game.players[i], 0, sizeof(Player));
            state->game.players[i].id = i;
            state->game.players[i].status = PLAYER_EMPTY;
        }
    }

    addBot(&state->game, false);
    newHand(&state->game, &state->deck);
    broadcast_chat_message(state, MAX_PLAYERS, "Starting next hand.");

    //dispatch on the new hand's phase: usually HAND_BETTING (broadcast + prompt),
    //but blinds can immediately force a runout or decide the hand.
    handle_after_move(state);
}

void broadcast_move(ServerState *state, uint8_t playerID, MoveType move, uint32_t amount)
{
    const char *name = state->game.players[playerID].name;
    char moveMsg[MAX_PAYLOAD_SIZE];

    switch (move)
    {
        case FOLD:
            snprintf(moveMsg, sizeof(moveMsg), "%s folded.", name);
            break;
        case CHECK:
            snprintf(moveMsg, sizeof(moveMsg), "%s checked.", name);
            break;
        case CALL:
            snprintf(moveMsg, sizeof(moveMsg), "%s called.", name);
            break;
        case RAISE:
            snprintf(moveMsg, sizeof(moveMsg), "%s raised by $%u.", name, amount);
            break;
        case ALL_IN:
            snprintf(moveMsg, sizeof(moveMsg), "%s went all in.", name);
            break;
        default:
            snprintf(moveMsg, sizeof(moveMsg), "%s made a move.", name);
            break;
    }

    broadcast_chat_message(state, MAX_PLAYERS, moveMsg);
}
