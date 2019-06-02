//
//  main.c
//  Socket server
//
//  Created by Peter on 29/9/18.
//

/* A simple server in the internet domain using TCP
 The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/_select.h>    // select.h was not including the select() declaration, not sure why
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

void converse(int sockfd);   // Messages handler
int read_from_client (int filedes);

void error(char *msg)
{
    perror(msg);
    exit(errno);
}

int read_from_client (int filedes)
{
    const ssize_t maxMsgSize = 512;
    char buffer[maxMsgSize];
    ssize_t nbytes;
    nbytes = read (filedes, buffer, maxMsgSize);
    if (nbytes < 0)
    {
        perror ("read");
        exit (EXIT_FAILURE);
    }
    else if (nbytes == 0)
    {
        return EOF;
    }
    else
    {
        fprintf (stderr, "Server: got message: '%s'\n", buffer);
        return 0;
    }
}


int main(int argc, char *argv[])
{
    int sockfd;         // file descriptor of socket used to listen for connection requests
    int newsockfd;      // file descriptor of socket used for data transfer
    uint16_t portno=51717;   // default port optionally overridden by user input
    const int request_queue_size=5;   // maximum number of pending requests
    struct sockaddr_in serv_addr_in;  // Server internet namespace socket address
    struct sockaddr_in cli_addr_in;   // Client internet namespace socket address
    struct sockaddr *serv_addr = (struct sockaddr *) &serv_addr_in;     // Generic address for passing to system calls
    struct sockaddr *cli_addr = (struct sockaddr *) &cli_addr_in;       // Generic address for passing to system calls
    socklen_t sockaddr_len = sizeof(struct sockaddr_in);    // Actual size of address structures when passing pointer to generic address
    fd_set active_fd_set, read_fd_set;
    
    // Specify port number
    if (argc > 1)
        portno = atoi(argv[1]);
    else
        printf("No port provided, using default %d\n", portno);
    
    // Create socket with internet namespace (Protocol Family), stream communication style, default protocol (TCP)
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        error("Error creating socket");
    
    // Server address
    memset((char *) &serv_addr_in, 0, sizeof(serv_addr_in));  // Clear the address object
    serv_addr_in.sin_family = AF_INET; // Address family is internet
    serv_addr_in.sin_addr.s_addr = htonl(INADDR_ANY); // Accept any incoming address (network byte order long)
    serv_addr_in.sin_port = htons(portno); // convert port number from host byte order to network byte order (short).
    
    // Bind socket to server address, interpreting address according to its sin_family value
    if (bind(sockfd, serv_addr, sockaddr_len) == -1)
        error("Error on binding");
    
    // Enable socket to accept connection requests
    if (listen(sockfd,request_queue_size) == -1)
        error("Error enabling socket to accept connection requests");
    
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    
    // Requests processing
    while (true) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
            error ("select");
        /* Service all the sockets with input pending. */
        for (int i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == sockfd) {
                    /* Connection request on original socket. */
                    // Accept a connection from the queue, blocking until the connection is established.
                    // Address interpreted according to sin_family value
                    newsockfd = accept(sockfd, cli_addr, &sockaddr_len);
                    if (newsockfd == -1)
                        error("Error accepting connection request");

                    fprintf (stderr,
                             "Server: connect from host %s, port %hu.\n",
                             inet_ntoa (cli_addr_in.sin_addr),
                             ntohs (cli_addr_in.sin_port));

                    FD_SET (newsockfd, &active_fd_set);
                }
                else {
                    /* Data arriving on an already-connected socket. */
                    if (read_from_client (i) == EOF) {
                        fprintf(stderr, "EOF on file %d", i);
                        close (i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }
    /*
        // Create process to handle this connection
        pid_t pid = fork(); // process id
        if (pid == -1)
            error("Error creating connection process");
    
        // Process messages
        if (pid == 0) {
            // this is the child process
            close(sockfd);
            converse(newsockfd);
            exit(EXIT_SUCCESS);
        }
        else {
            // this is the parent process
            close(newsockfd);
        }
    }
*/
}

void converse(int sockfd) {
    const int bufsize=10;
    char buffer[bufsize];   // Received message
    char response[] = "I got your message";  // Response from server to client

    // Wait for data from client
    memset(buffer, 0, bufsize);
    ssize_t message_size;
    message_size = read(sockfd, buffer, bufsize-1);
    if (message_size == -1)
        error("Error reading from socket");

    // Display the received message
    printf("Here is the message:\n\n%s", buffer);
    
    while ( message_size == bufsize-1 ) {
        memset(buffer, 0, bufsize);
        message_size = read(sockfd, buffer, bufsize-1);
        if (message_size == -1)
            error("Error reading from socket");
        fputs(buffer, stdout);
    }
    
    // Respond to client
    if (write(sockfd, response, sizeof(response)) == -1)
        error("Error writing to socket");
}
