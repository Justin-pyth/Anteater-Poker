#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include "protocol.h"
#include "com.h"
#include "uds.h"

// display game state to the player
// TODO: implement UI
static void display_game_state(ClientState *client)
{
    (void)client;
}

// read move from stdin, encode, and send to server
// TODO: implement user input handling
void handle_user_input(ClientState *client)
{
    (void)client;
    //PlayerAction action;// ask and store the user input into this function
    //implementation goes here



    //display_game_state(client); // re-display game state to show updated info after each move   
    //send_action(client, &action);
}
void handle_chat_message(const char *message)
{
    (void)message;
    //to be implemented.
}
void handle_error_message(const char *message)
{
    (void)message;
    //to be implemented.    
}
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s hostname port\n", argv[0]);
        exit(0);
    }

    ClientState client;
    init_client_state(&client);

    int sockfd = connect_to_server(argv[1], atoi(argv[2]));
    if (sockfd < 0)
        exit(1);

    client.socket_fd = sockfd;
    client.connected = 1;

    while (client.running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client.socket_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        int max_fd = client.socket_fd > STDIN_FILENO ? client.socket_fd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(client.socket_fd, &read_fds)) {
            handle_server_communication(&client); // decode incoming bytes into client.game
            if (!client.connected) break;
            display_game_state(&client);         // display state, prompt if our turn
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            handle_user_input(&client);           // read move, encode, send to server
        }
    }

    close(client.socket_fd);
    printf("Disconnected.\n");
    return 0;
}
