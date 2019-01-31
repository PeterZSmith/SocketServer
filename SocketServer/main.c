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
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

void converse(int sockfd);   // Messages handler

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd;         // file descriptor of socket used to listen for connection requests
    int newsockfd;      // file descriptor of socket used for data transfer
    int portno=51717;   // default port optionally overridden by user input
    const int request_queue_size=5;   // maximum number of pending requests
    struct sockaddr_in serv_addr_in;  // Server internet namespace socket address
    struct sockaddr_in cli_addr_in;   // Client internet namespace socket address
    struct sockaddr *serv_addr = (struct sockaddr *) &serv_addr_in;     // Generic address for passing to system calls
    struct sockaddr *cli_addr = (struct sockaddr *) &cli_addr_in;       // Generic address for passing to system calls
    socklen_t sockaddr_len = sizeof(struct sockaddr_in);    // Actual size of address structures when passing pointer to generic address
    pid_t pid;  // process id returned by fork()
    
    // Specify port number
    if (argc > 1)
        portno = atoi(argv[1]);
    else
        printf("No port provided, using default %d\n", portno);
    
    // Create socket with internet namespace, stream type, default protocol (TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        error("Error creating socket");
    
    // Server address
    memset((char *) &serv_addr_in, 0, sizeof(serv_addr_in));  // Clear the address object
    serv_addr_in.sin_family = AF_INET; // Address family is internet
    serv_addr_in.sin_addr.s_addr = INADDR_ANY; // Accept any incoming address
    serv_addr_in.sin_port = htons(portno); // convert port number from host byte order to network byte order.
    
    // Bind socket to server address, interpreting address according to its sin_family value
    if (bind(sockfd, serv_addr, sockaddr_len) == -1)
        error("Error on binding");
    
    // Listen for connection requests, blocking
    if (listen(sockfd,request_queue_size) == -1)
        error("Error turning on listening for connection requests");
    
    // Requests processing
    while (true) {
        // Accept a connection, blocking until the connection is established. Address interpreted according to sin_family value
        newsockfd = accept(sockfd, cli_addr, &sockaddr_len);
        if (newsockfd == -1)
            error("Error accepting connection request");
    
        // Create process to handle this connection
        pid = fork();
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
