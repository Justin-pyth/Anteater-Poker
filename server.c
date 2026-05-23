/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "protocol.h"



int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int listen_fd; // Socket file descriptor for listening
    ServerState state; // Server state to hold client information

    init_server(&state); // Initialize the server state
    listen_fd = create_socket(NULL); // Create a socket for listening
    state.listen_fd = listen_fd; // Store the listening socket in the server state
    printf("Server is running on port %d\n", PORT);
    while (state.running) {
        // After sucessfully create the socker, server loop begins here
        //it will accept incoming connections and handle client communication

        fd_set read_fds; // Set of file descriptors for select
        int max_fd;

        FD_ZERO(&read_fds); // Clear the set of file descriptors
        FD_SET(state.listen_fd, &read_fds); // Add the listening socket to the
        max_fd = state.listen_fd; // Initialize max_fd to the listening socket

        // Add client sockets to the set of file descriptors
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (state.clients[i].connected) {
                int fd= state.clients[i].client_fd; // Get the client socket file descriptor
                FD_SET(fd, &read_fds); // Add the client socket to the set of file descriptors
                if (fd > max_fd) {
                    max_fd = fd; // Update max_fd if the client socket is greater
                }
            }
        }
        struct timeval timeout;
        struct timeval *timeout_ptr = NULL;

        if (state.game.handPlaying && isBot(state.game.players[state.game.currentPlayer].name)) 
        {
            //wait 1.0 seconds
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            timeout_ptr = &timeout; //set when bot's turn
        }

        //integrate bot's timeout into select, aka when its a bot's turn wait 1 second to display to gui
        //if its a player's turn, then you don't need delay
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, timeout_ptr); // Wait for activity on the sockets
        if (activity < 0) {
            perror("select error");
            continue; // Continue to the next iteration of the loop if select fails
        }
        if (activity == 0) {
            if (doOneBotTurn(&state.game, &state.deck))
                broadcast_game_state(&state);
            continue;
        }
        if (FD_ISSET(state.listen_fd, &read_fds)) {
            // If there is activity on the listening socket, it means a new client is trying to connect
            Client new_client; // Create a new client structure
            add_connection(&state, &new_client); // Add the new client connection to the server state
        }
        // Check for activity on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (state.clients[i].connected){
                int fd = state.clients[i].client_fd; // Get the client socket file descriptor
                //game logic goes here, handle client communication
                if (FD_ISSET(fd, &read_fds)) {
                    // If there is activity on the client socket, handle communication with the client
                    handle_client_communication(&state, &state.clients[i]);


                    //=============================================================
                    //GAME LOGIC (should probably be in handle_client_communcation)
                    //-------------------------------------------------------------
                    //dummy variables

                    //IF MESSAGE == READY
                    /*
                    -set playerID
                    -set player status as READY
                    -if more than 2 players are ready
                        -newHand()
                    -broadcast gamestate
                    */

                    //IF MESSAGE == MOVE
                    //uint8_t _playerID = i;
                    //MoveType _move; //decoded move
                    //uint32_t _amount; //decoded amount
                    //if(tryMove(&state.game, &state.deck, _playerID, _move, _amount))
                    //{
                        //BROADCAST UPDATED GAMESTATE TO CLIENTS
                    //}
                    //else
                    //{
                    //    //SEND "INVALID"
                    //}
                    //=============================================================


                }
            }
        }
    }
    
    cleanup_server(&state); // Clean up server resources

    return 0;
}
/*     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);
     return 0; */
