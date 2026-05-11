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
#include "uds.h"

int main(int argc, char *argv[])
{
    char* hostname;
    int sockfd;
    ClientState client;
    init_client_state(&client); // Initialize the client state
    if (argc < 3) {// Check if the hostname and port number are provided as command-line arguments
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    int port = atoi(argv[2]);// Convert the port number from string to integer

    sockfd = connect_to_server(hostname, port);
    if (sockfd < 0)
        exit(1);
    client.socket_fd = sockfd;
    client.connected = 1;
    while (client.running)
    {
        fd_set read_fds; // Set of file descriptors for select
        int max_fd;
        FD_ZERO(&read_fds); // Clear the set of file descriptors

        FD_SET(client.socket_fd, &read_fds); // Add the client socket to the set of file descriptors
        FD_SET(STDIN_FILENO, &read_fds); // Add standard input to the set of file descriptors
        max_fd = sockfd; // Initialize max_fd to the client socket file descriptor
        if (STDIN_FILENO > max_fd) {
            max_fd = STDIN_FILENO; // Update max_fd if standard input is greater
    }
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL); // Wait for activity on the sockets
        if (activity < 0) {
            perror("select error");
            continue; // Continue to the next iteration of the loop if select fails
        }
        if (FD_ISSET(client.socket_fd, &read_fds)) {
            receive_data_from_server(&client); // Handle incoming data from the server
            //ui update goes here
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            //handle_user_input(&client); // Handle user input from standard input
            // also update ui here if needed
            printf("enter message: ");
            memset(client.input_buffer, 0, sizeof(client.input_buffer));
            if (fgets(client.input_buffer, sizeof(client.input_buffer), stdin) == NULL) {
                client.running = 0;
                continue;
            }
            send_data_to_server(&client, client.input_buffer); // Send the user input to the server
        }
    }
    
    close(client.socket_fd);
    return 0;

}
/* int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return 0;
}*/
