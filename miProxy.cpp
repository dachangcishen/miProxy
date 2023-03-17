#include <arpa/inet.h> //close
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO, FD_SETSIZE macros
#include <time.h>
#include <sys/types.h>
#include <unistd.h> //close
#include <chrono>
//#include "socket.c"
//#include "parse.c"

#define MAXCLIENTS 30
#define HEADERLEN 10000
#define CONTENT 10000

using namespace std;
using namespace chrono;

int client(const char* ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    struct hostent* server = gethostbyname(ip);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(unsigned long*)server->h_addr_list[0];
    addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connection failed");
        exit(1);
    }

    // (4) Return
    printf("Client made successfully, socket: %d\n", sockfd);
    return sockfd;
}

void bitrate_reorder(double* throughputs, int tp_length) {
    int i, j;
    for (i = 0; i < tp_length; i++) {
        for (j = i + 1; j < tp_length; j++) {
            if (throughputs[i] > throughputs[j]) {
                double temp = throughputs[j];
                throughputs[j] = throughputs[i];
                throughputs[i] = temp;
            }
        }

    }
}

//choose a proper bitrate
double get_bitrate(double T_cur, double* throughputs, int len)
{
    int i = 0;
    for (; i < len; ++i)
    {
        printf("tcur:%f\n", T_cur);
        printf("throughputs[i]:%f\n", throughputs[i]);

        if (T_cur < 1.5 * throughputs[i])
        {
            break;
        }
    }
    --i;
    if (i < 0) i = 0;
    return throughputs[i];
}

int get_header_len(char* res) {
    char resp[CONTENT];
    strcpy(resp, res);
    int index;
    for (int i = 3; i < strlen(resp); i++) {
        if (resp[i] == '\n' && resp[i - 1] == '\r' && resp[i - 2] == '\n' && resp[i - 3] == '\r') {
            index = i;
        }
    }
    return index + 1;
}

int get_content_len(char* res) {
    int len = -1;
    char resp[CONTENT];
    strcpy(resp, res);
    char* tok = strtok(resp, "\r\n");
    char tmp[50] = { 0 };
    while (tok)
    {
        if (strstr(tok, "Content-Length")) {
            for (int i = 16; i < strlen(tok); i++) {
                tmp[i - 16] = tok[i];
            }
            len = atoi(tmp);
            break;
        }
        tok = strtok(NULL, "\r\n");
    }
    return len;
}

int readLine(char* from, int offset, int length, char* to) {
    int n;
    char* tmp = from + offset;
    for (n = 0; n < length; n++)
    {
        to[n] = tmp[n];
        if (tmp[n] == '\n' || tmp[n] == '\0')
        {
            break;
        }
    }
    return n;
}

int request_send(char* buf, int valread, int proxy_client_sock, int client_sock) {
    int nbytes = (int)send(proxy_client_sock, buf, valread, 0);
    memset(buf, 0, CONTENT);
    int readed;
    readed = read(proxy_client_sock, buf, CONTENT);
    if (readed == -1)
    {
        perror("Error receiving response");
        close(proxy_client_sock);
        return -1;
    }
    nbytes = (int)send(client_sock, buf, readed, 0);
    printf("Send to client: %d\n", nbytes);
    if (nbytes == -1)
    {
        perror("Error sending response");
        close(proxy_client_sock);
        return -1;
    }

    // Parse content length
    printf("Parse content length\n");
    int remain;
    int content_length = get_content_len(buf);
    int header_length = get_header_len(buf);
    int total = header_length + content_length;

    int offset = 0;
    remain = content_length - (readed - header_length);
    printf("cont: %d\n", content_length);
    printf("header_length: %d\n", header_length);
    //printf("buffer length: %d\n",(int)strlen(buf));
    printf("remain: %d\n", remain);
    memset(buf, 0, CONTENT);
    while (remain > 0) {
        readed = (int)read(proxy_client_sock, buf, CONTENT);

        if (readed == -1)
        {
            perror("Error receiving response");
            close(proxy_client_sock);
            break;
        }
        remain -= readed;
        printf("Readed: %d ", readed); buf[readed] = '\0';
        send(client_sock, buf, readed, 0);
        memset(buf, 0, CONTENT);
        printf("remain: %d\n", remain);
    }
    return total;
}

int forward_request_get_bitrates(char* buf, int valread, double* throughputs, int proxy_client_sock) {
    char xml[CONTENT * 10] = { 0 };
    int nbytes = (int)send(proxy_client_sock, buf, valread, 0);
    memset(buf, 0, CONTENT);
    int readed;
    readed = read(proxy_client_sock, buf, CONTENT);
    for (int i = 0; i < readed; i++) {
        xml[i] = buf[i];
    }
    if (readed == -1) {
        perror("Error receiving response");
        close(proxy_client_sock);
        return -1;
    }
    if (nbytes == -1) {
        perror("Error sending response");
        close(proxy_client_sock);
        return -1;
    }

    printf("Parse content length\n");
    int remain;
    int content_length = get_content_len(buf);
    int header_length = get_header_len(buf);
    int offset = 0;
    remain = content_length - (readed - header_length);
    printf("cont: %d\n", content_length);
    printf("header_length: %d\n", header_length);
    printf("remain: %d\n", remain);
    int total = remain + readed;
    while (remain > 0) {
        printf("Modify Throughts 2\n");
        memset(buf, 0, CONTENT);
        readed = (int)read(proxy_client_sock, buf, remain);
        if (readed == -1) {
            perror("Error receiving response");
            close(proxy_client_sock);
            break;
        }
        remain -= readed;
        printf("Readed: %d ", readed); buf[readed] = '\0';
        strcat(xml, buf);
        memset(buf, 0, CONTENT);
        printf("remain: %d\n", remain);
    }
    int index = 0;
    char* token = strtok(xml, " ");
    while (token != NULL) {
        if (strstr(token, "bitrate")) {
            char tmp[12] = { 0 };
            for (int i = 9; i < strlen(token) - 1; i++) {
                tmp[i - 9] = token[i];
            }
            int bitr = atoi(tmp);
            throughputs[index] = bitr;
            index++;
        }
        token = strtok(NULL, " ");
    }
    return index;
}


int run_miProxy(unsigned short port, char* wwwip, double alpha, char* log) {
    int listen_socket, addrlen, activity, valread;
    int client_sockets[MAXCLIENTS] = { 0 };
    double throughputs[MAXCLIENTS] = { 0 };
    int tp_length = 0;
    int bitrate;
    int proxy_server_socket, client_sock, proxy_client_sock;
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;

    double T_cur = 0.0001;
    double T_new = 0.0001;

    char buf[CONTENT];

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if ((proxy_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket creation failed");
        exit(1);
    }
    if (bind(proxy_server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(1);
    }
    if (listen(proxy_server_socket, 10) < 0) {
        perror("listen failed\n");
        exit(1);
    }
    printf("Listen on port %d\n", port);

    // set of socket descriptors

    if ((proxy_client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket creation failed");
        exit(1);
    }

    printf("Connect to Server.\n");

    proxy_client_sock = client(wwwip, 80);

    // initialize the client sockets and throughputs
    for (int i = 0; i < MAXCLIENTS; i++) {
        client_sockets[i] = 0;
        throughputs[i] = 0;
    }

    //listen
    while (1) {
        // clear the socket set
        FD_ZERO(&readfds);
        // add master socket to set
        FD_SET(proxy_server_socket, &readfds);
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
        if (FD_ISSET(proxy_server_socket, &readfds)) {
            socklen_t server_addr_len = sizeof(address);
            int new_socket = accept(proxy_server_socket, (struct sockaddr*)&address, &server_addr_len);

            if (new_socket < 0) {
                perror("Accept Error");
                exit(1);
            }

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
            memset(buf, 0, CONTENT);
            client_sock = client_sockets[i];
            // Note: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds)) {

                // Check if it was for closing , and also read the incoming message
                //getpeername(client_sock, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                valread = (int)read(client_sock, buf, CONTENT);
                printf("\nNew Valread: %d\n", valread);
                if (valread == 0) {
                    // Somebody disconnected , get their details and print
                    printf("\n---Host disconnected---\n");
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                }
                else {
                    // Parse http request
                    puts("Parse request");
                    int nbytes = valread;
                    if (nbytes == -1) {
                        perror("Error receiving");
                        close(client_sock);
                        continue;
                    }

                    //Revised request(both nolist f4m and video chunk)
                    char request[CONTENT];  memset(request, 0, CONTENT * sizeof(char));
                    //Request Line buffer
                    char line[HEADERLEN];  memset(line, 0, HEADERLEN * sizeof(char));
                    int offset = 0;
                    // Method / URI / Version
                    char method[HEADERLEN];   memset(method, 0, HEADERLEN * sizeof(char));
                    char url[HEADERLEN];   memset(url, 0, HEADERLEN * sizeof(char));
                    char version[HEADERLEN];   memset(version, 0, HEADERLEN * sizeof(char));

                    //store rest of header;
                    char rest[HEADERLEN];   memset(rest, 0, HEADERLEN * sizeof(char));

                    char path[1000];   memset(path, 0, 1000 * sizeof(char));
                    char chunk[1000];   memset(chunk, 0, 1000 * sizeof(char));

                    //parse first line

                    nbytes = readLine(buf, offset, sizeof(buf), line);
                    offset = offset + nbytes + 1;
                    sscanf(line, "%s %s %s\r\n", method, url, version);
                    //store rest of them
                    while (nbytes)
                    {
                        memset(line, 0, HEADERLEN * sizeof(char));
                        nbytes = readLine(buf, offset, sizeof(buf), line);
                        strcat(rest, line);

                        offset += nbytes + 1;
                    }
                    nbytes = readLine(buf, offset, sizeof(buf), line);
                    strcat(rest, line);

                    printf("Finish parse header: \nmethod: %s, uri: %s, version: %s\n", method, url, version);

                    //IF it is a f4m request
                    if (strstr(url, ".f4m")) {
                        char xml[CONTENT] = { 0 };
                        char* newUrl = strcat(strtok(url, ".f4m"), "_nolist.f4m");
                        sprintf(request, "%s %s %s\r\n", method, newUrl, version);
                        strcat(request, rest);
                        tp_length = forward_request_get_bitrates(buf, valread, throughputs, proxy_client_sock);
                        memset(buf, 0, CONTENT);
                        bitrate_reorder(throughputs, tp_length);
                        T_cur = throughputs[0];
                        request_send(request, valread + 7, proxy_client_sock, client_sock);
                    }
                    //IF it is a video request
                    else if (strstr(url, "Seg") && strstr(url, "Frag")) {
                        bitrate = get_bitrate(T_cur, throughputs, tp_length);
                        //pathBitrateChunk of url
                        int old = strlen(url);
                        sscanf(url, "%[^0-9]%*d%s", path, chunk);
                        char ggg[1000];
                        sprintf(ggg, "%s%d%s", path, bitrate, chunk);
                        int ne = strlen(ggg);
                        sprintf(request, "%s %s %s\r\n%s", method, ggg, version, rest);
                        //send revised request

                        auto start = chrono::system_clock::now();

                        int total;

                        int header_len, remain, cont_len;
                        // direct the request to the server directly

                        nbytes = send(proxy_client_sock, request, valread - old + ne, 0);

                        if (nbytes == -1) {
                            perror("Error in sending chunk request to server");
                            exit(EXIT_FAILURE);
                        }

                        int total_len = 0;
                        // receive data from server
                        memset(buf, 0, CONTENT);

                        nbytes = read(proxy_client_sock, buf, CONTENT);
                        printf("%s\n", buf);
                        if (nbytes == -1) {
                            perror("Error in receving chunk request to server");
                            exit(EXIT_FAILURE);
                        }
                        total_len += nbytes;
                        printf("Receive bytes: %d\n", nbytes);
                        buf[nbytes] = '\0';
                        send(client_sock, buf, nbytes, 0);

                        // get content length
                        cont_len = get_content_len(buf);
                        printf("Content length: %d\n", cont_len);
                        // get header length
                        header_len = get_header_len(buf);
                        printf("Response header lenght: %d\n", header_len);
                        printf("Buffer length: %zu\n", strlen(buf));
                        // get remain content length
                        remain = cont_len - (nbytes - header_len);
                        memset(buf, 0, CONTENT);
                        printf("Response remain length: %d\n", remain);
                        // receive from the server if there is still sth
                        while (remain > 0) {
                            nbytes = read(proxy_client_sock, buf, CONTENT);
                            total_len += nbytes;
                            printf("Receive bytes: %d\n", nbytes);
                            buf[nbytes] = '\0';
                            remain -= nbytes;
                            send(client_sock, buf, nbytes, 0);
                            memset(buf, 0, CONTENT);
                            printf("Response remain length: %d\n", remain);
                        }

                        printf("total: %d\n", total_len);
                        auto end = chrono::system_clock::now();
                        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                        double period = double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den ;
                        

                        T_new = (total_len * 8) / (period * 1000);
                        T_cur = (1 - alpha) * T_cur + alpha * T_new;

                        FILE* logFile;
                        logFile = fopen(log, "a");

                        fprintf(logFile, "%s %d%s %s %.3f %.3f %.3f %d\n", inet_ntoa(address.sin_addr), bitrate, chunk, wwwip, period, T_new, T_cur, bitrate);

                        fclose(logFile);
                    }
                    else {
                        //send revised request
                        request_send(buf, valread, proxy_client_sock, client_sock);
                    }
                }
            }
        }
        //not finished yet
    }
    return 0;
}


int main(int argc, const char** argv) {
    if (argc == 6) {
        unsigned short port = (unsigned short)atoi(argv[2]);
        const char* wwwip = argv[3];
        double alpha = atof(argv[4]);
        const char* log = argv[5];

        run_miProxy(port, (char*)wwwip, alpha, (char*)log);

        return 0;

    }
    else {

        printf("Error! Please Run with proper argument");

        return -1;
    }
}