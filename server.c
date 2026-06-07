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

#ifdef ENABLE_SERVER_GUI
#include <gtk/gtk.h>
#include "server_gui.h"
#endif



int main(int argc, char *argv[])
{
#ifdef ENABLE_SERVER_GUI
    gtk_init(&argc, &argv);
#else
    (void)argc;
    (void)argv;
#endif

    int listen_fd; // Socket file descriptor for listening
    ServerState state; // Server state to hold client information

    init_server(&state); // Initialize the server state
    listen_fd = create_socket(NULL); // Create a socket for listening
    state.listen_fd = listen_fd; // Store the listening socket in the server state
#ifdef ENABLE_SERVER_GUI
    server_gui_init(&state);
#endif
    printf("Server is running on port %d\n", PORT);

    //wall-clock deadline that paces bot moves (~1s apart). Using a deadline instead
    //of relying on select() blocking for a full second lets the server GUI build cap
    //the select timeout for responsiveness without speeding up the bots.
    struct timeval next_bot_action = {0, 0};

    while (state.running) {
#ifdef ENABLE_SERVER_GUI
        server_gui_pump();
        server_gui_refresh(&state.game);
#endif
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

        if (state.timer_active)
        {
            //a timed FSM action is pending (runout reveal / inter-hand delay):
            //wake up when its deadline is reached
            struct timeval now;
            gettimeofday(&now, NULL);
            long sec  = state.timer_deadline.tv_sec  - now.tv_sec;
            long usec = state.timer_deadline.tv_usec - now.tv_usec;
            if (usec < 0) { sec -= 1; usec += 1000000; }
            if (sec < 0) { sec = 0; usec = 0; } //already due
            timeout.tv_sec = sec;
            timeout.tv_usec = usec;
            timeout_ptr = &timeout;
        }
        else if (state.game.handPlaying &&
                 state.game.currentPlayer < MAX_PLAYERS &&
                 isBot(state.game.players[state.game.currentPlayer].name))
        {
            //bot's turn: wait 1.0 seconds so the GUI can show the action
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            timeout_ptr = &timeout;
        }
#ifdef ENABLE_SERVER_GUI
        else
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
            timeout_ptr = &timeout;
        }

        //Keep the monitor responsive: never block longer than the GUI refresh
        //interval (~50ms), even during bot-only play or the inter-hand pause.
        //The real bot/timer pacing is enforced by wall-clock deadlines below, so
        //waking early here costs nothing but a cheap GTK pump.
        if (timeout_ptr == NULL || timeout.tv_sec > 0 || timeout.tv_usec > 50000)
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
            timeout_ptr = &timeout;
        }
#endif

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, timeout_ptr); // Wait for activity on the sockets
        if (activity < 0) {
            perror("select error");
            continue; // Continue to the next iteration of the loop if select fails
        }
        if (activity == 0) {
            struct timeval now;
            gettimeofday(&now, NULL);

            //timer takes priority: it only runs during runout / inter-hand pauses,
            //when no player or bot is due to act. Fire only once its deadline is
            //actually reached (the GUI build may wake early due to the timeout cap).
            if (state.timer_active)
            {
                if (now.tv_sec > state.timer_deadline.tv_sec ||
                    (now.tv_sec == state.timer_deadline.tv_sec &&
                     now.tv_usec >= state.timer_deadline.tv_usec))
                {
                    service_timer(&state);
                }
                continue;
            }

            //bot's turn: pace moves ~1s apart using the wall clock (independent of how
            //often select() wakes), so the headless build is unchanged while the GUI
            //build stays responsive.
            if (state.game.handPlaying &&
                state.game.currentPlayer < MAX_PLAYERS &&
                isBot(state.game.players[state.game.currentPlayer].name) &&
                (now.tv_sec > next_bot_action.tv_sec ||
                 (now.tv_sec == next_bot_action.tv_sec &&
                  now.tv_usec >= next_bot_action.tv_usec)))
            {
                uint8_t botID;
                MoveType move;
                uint32_t amount;

                if (doOneBotTurn(&state.game, &state.deck, &botID, &move, &amount))
                {
                    broadcast_move(&state, botID, move, amount);
                    handle_after_move(&state);
                }

                //schedule the next bot action ~1s out
                gettimeofday(&next_bot_action, NULL);
                next_bot_action.tv_sec += 1;
            }
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
