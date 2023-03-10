#include <arpa/inet.h> //close
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO, FD_SETSIZE macros
#include <sys/types.h>
#include <unistd.h> //close
#include "socket.c"
#include "parse.c"
#define MAXCLIENTS 30

int main(int argc, const char **argv){
    int listen_socket, addrlen, activity, valread;
    int client_sockets[MAXCLIENTS] = {0};
    double throughputs[MAXCLIENTS] = {0};
    unsigned short port = (unsigned short) atoi(argv[2]);
    char* wwwip = argv[3];
    double alpha = strtod(argv[4]);
    char* log = argv[5];
    int client_sock;

    struct sockaddr_in address;
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(port);
    listen_socket = open_listen_socket(port);

    char buffer[1025]; 

    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    // set of socket descriptors
    fd_set readfds;
    while (1) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(listen_socket, &readfds);
        for (int i = 0; i < MAXCLIENTS; i++) {
            client_sock = client_sockets[i];
            if (client_sock != 0) {
                FD_SET(client_sock, &readfds);
            }
        }
        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);


        // If something happened on the master socket ,
        // then its an incoming connection, call accept()
        if (FD_ISSET(listen_socket, &readfds)) {
            int new_socket = accept(listen_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);

            // inform user of socket number - used in send and receive commands
            printf("\n---New host connection---\n");
            printf("socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));


            // add new socket to the array of sockets
            for (int i = 0; i < MAXCLIENTS; i++) {
                // if position is empty
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        // else it's some IO operation on a client socket
        for (int i = 0; i < MAXCLIENTS; i++) {
            client_sock = client_sockets[i];
            // Note: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds)) {
                // Check if it was for closing , and also read the incoming message
                getpeername(client_sock, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                valread = read(client_sock, buffer, 1024);
                if (valread == 0) {
                    // Somebody disconnected , get their details and print
                    printf("\n---Host disconnected---\n");
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                } else {
                    // send the same message back to the client, hence why it's called
                    // "echo_server"
                    buffer[valread] = '\0';
                    printf("\n---New message---\n");
                    printf("Message %s", buffer);
                    printf("Received from: ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                }
            }
        }
        //not finished yet
    }
    return 0;    
}