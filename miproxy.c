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
#define HEADERLEN 10000
#define CONTENT 100000

int open_listen_socket(unsigned short port) {
    int listen_fd;
    int opt_value = 1;
    struct sockaddr_in server_address;


    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_value, sizeof(int));

    memset((char*)&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    bind(listen_fd, (struct sockaddr*)&server_address, sizeof(server_address));

    listen(listen_fd, MAXCLIENTS);

    return listen_fd;
}

//choose a proper bitrate
int bitrate(double T_cur) {

}

int readLine(char* from, int offset, int length, char* to ) {
    int i;
    for (i = 0; i < length; i++) {
        to[i] = from[i];
        if (from[i] == '\0' || from[i] == '\n') {
            break;
        }
    }
    return i;
}

int run_miProxy(unsigned short port, char* wwwip, double alpha, char* log) {
    int listen_socket, addrlen, activity, valread;
    int client_sockets[MAXCLIENTS] = { 0 };
    double throughputs[MAXCLIENTS] = { 0 };
    int client_sock;

    //Open log file;
    FILE* logFile;
    logFile = fopen(log, "w");
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    listen_socket = open_listen_socket(port);

    char buffer[1025];

    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    // set of socket descriptors
    fd_set readfds;

    double T_cur = 0.0001;
    double T_new = 0.0001;

    //listen
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
            int new_socket = accept(listen_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);

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
                getpeername(client_sock, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                valread = read(client_sock, buffer, 1024);
                if (valread == 0) {
                    // Somebody disconnected , get their details and print
                    printf("\n---Host disconnected---\n");
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                }
                else {
                    int nbytes = 0;
                    char buffer[CONTENT];
                    
                    //Revised request(both nolist f4m and video chunk)
                    char request[HEADERLEN];  memset(request, 0, HEADERLEN * sizeof(char));
                    //Request Line buffer
                    char line[HEADERLEN];  memset(line, 0, HEADERLEN * sizeof(char));
                    int offset = 0;
                    // Method / URI / Version
                    char method[HEADERLEN];   memset(method, 0, HEADERLEN * sizeof(char));
                    char url[HEADERLEN];   memset(url, 0, HEADERLEN * sizeof(char));
                    char version[HEADERLEN];   memset(version, 0, HEADERLEN * sizeof(char));

                    //store rest of header;
                    char rest[HEADERLEN];   memset(rest, 0, HEADERLEN * sizeof(char));

                    //receive request
                    nbytes = (int)recv(client_sock, buffer, CONTENT, MSG_NOSIGNAL);

                    //delete all IF if there is something wrong here.
                    if (nbytes < 1) { 
                        perror("Error in receving");
                        close(client_sock);
                        client_sockets[i] = 0;
                        continue; 
                    }

                    //parse first line

                    nbytes = readLine(buffer, offset, line, sizeof(buffer));
                    offset = offset + nbytes + 1;
                    sscanf(line, "%s %s %s\r\n", method, url, version);

                    //store rest of them
                    while (nbytes)
                    {
                        memset(line, 0, HEADERLEN * sizeof(char));
                        nbytes = readLine(buffer, offset , line, sizeof(buffer));
                        strcat(rest, line);

                        offset += nbytes + 1;
                    }
                    nbytes = readLine(buffer, offset, line, sizeof(buffer));
                    strcat(rest, line);
                    
                    printf("Finish parse header: \nmethod: %s, url: %s, version: %s\n %s", method, url, version,rest);
                    
                    //IF it is a f4m request
                    if (strstr(url, ".f4m")){
                        char* newTail = "_nolist.f4m";
                        char* newUrl = strcat(strtok(url, ".f4m"), newTail);
                        sprintf(request, "%s %s %s", method, newUrl, version);
                        strcat(request, rest);

                        //send revised request(both)
                        nbytes = (int)send(listen_socket, request, sizeof(request), 0);
                        nbytes = (int)send(listen_socket, buffer, sizeof(buffer), 0);
                    }
                    //IF it is a video request
                    else if (strstr(url, "Seg") && strstr(url, "Frag")) {

                        //get a bitrate, not done yet
                        int bitrate = bitrate(T_cur);

                        //pathBitrateChunk of url
                        char path[1000];   memset(path, 0, 1000 * sizeof(char));
                        char chunk[1000];   memset(chunk, 0, 1000 * sizeof(char));
                        sscanf(url, "%[^0-9]%*d%s", path, chunk);
                        sprintf(request, "%s %s%d%s %s", method, path, bitrate, chunk, version);
                        strcat(request, rest);
                        //send revised request
                        nbytes = (int)send(listen_socket, request, sizeof(request), 0);
                    }
                    else {
                        strcpy(request, buffer);
                        //send revised request
                        nbytes = (int)send(listen_socket, request, sizeof(request), 0);
                    }
                    memset(buffer, 0, CONTENT * sizeof(char));

                    //Receive Response

                    time_t startTime;
                    time_t endTime;

                    time(&startTime);
                    nbytes = (int)recv(listen_socket, buffer,HEADERLEN * sizeof(char), 0);
                    if (nbytes == -1)
                    {
                        perror("Error receiving response");
                        close(listen_socket);
                        break;
                    }
                    int readed = nbytes;
                    int content_Length = 0;
                    char val[1000];   memset(val, 0, 1000*sizeof(char));
                    get_header_val(buffer, sizeof(buffer), "Content_Length", 14, val);
                    content_Length = atoi(val);
                    int left = content_Length;
                    int total = content_Length + readed;

                    char* buffer_ptr = buffer + readed;
                    while (1) {
                        nbytes = (int)recv(client_sock, buffer_ptr, left, MSG_NOSIGNAL);
                        if (nbytes < 0) {
                            perror("Error receiving response");
                            close(client_sock);
                            client_sockets[i] = 0;
                            break;
                        }
                        left = left - nbytes;
                        buffer_ptr = buffer_ptr + nbytes;
                    }

                }
            }
        }
        //not finished yet
    }
    fclose(logFile);
    return 0;
}


int main(int argc, const char **argv){
    if (argc == 6 && strcmp(argv[1],"-nodns")==0) {
        unsigned short port = (unsigned short)atoi(argv[2]);
        char* wwwip = argv[3];
        char* errptr;
        double alpha = strtod(argv[4], errptr);
        char* log = argv[5];

        run_miProxy(port, wwwip, alpha, log);

        return 0;

       }
    else {

        printf("Error! Please Run with proper argument");

        return -1;
    }
}