#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

int open_listen_socket(unsigned short port){
    int listen_fd;
    int opt_value = 1;
    struct sockaddr_in server_address;

    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt_value , sizeof(int));
	
    memset((char *)&server_address,0,sizeof(server_address));
    
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = INADDR_ANY; 
    server_address.sin_port = htons(port); 
    bind(listen_fd, (struct sockaddr *)&server_address, sizeof(server_address));

    listen(listen_fd, 30);
	
    return listen_fd;
}

int open_socket_to_server(char *myip, char *serverip) {
    int server_sock;
    struct sockaddr_in my_address;
    struct sockaddr_in server_address;
     
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    memset((char *)&my_address, 0, sizeof(my_address));
    my_address.sin_family = AF_INET;
    my_address.sin_port = 0;
    inet_aton(myip, &(my_address.sin_addr));
    bind(server_sock, (struct sockaddr *)&my_address, sizeof(my_address));
    

    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET; 
    server_address.sin_port = 80; 
    inet_pton(AF_INET, serverip, &(server_address.sin_addr)); 
    connect(server_sock, (struct sockaddr *)&server_address, sizeof(server_address));
    
    return server_sock;
}