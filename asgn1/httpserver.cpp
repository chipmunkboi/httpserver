#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>

#include <string.h>     //memset
#include <stdlib.h>     //atoi
#include <unistd.h>     //write
#define SIZE 1000
// temp
#include <stdio.h>      //printf
//

//port is set to user-specified number or 80 by default
int getport(char argone[]){
    int port;

    if (argone != NULL){
        port = atoi(argone);
        if (port < 1024){
            printf("Port Error: Port numbers must be above 1024\n");
            exit(EXIT_FAILURE);
        }
    }else{
        port = 80;
    }

    return port;
}

int main(int argc, char *argv[]){
    int opt = 1;
    int port = getport(argv[1]);
    printf("port = %d\n", port);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
        
    //create server socket file descriptor
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("server_socket");
        exit(EXIT_FAILURE);
    }

    //setsockopt: helps in reusing address and port
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    socklen_t addrlen = sizeof(server_addr);

    //bind server address to open socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, addrlen) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    //listen for incoming connections
    if (listen(server_socket, 500) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    char buf[SIZE];
    while(true){
        //accept incoming connections
        int comm_fd = accept(server_socket, NULL, NULL);
        
        while(1){
            //use comm_fd to comm with client
            int n = recv(comm_fd, buf, SIZE, 0);
            if(n == 0) break;
            send(comm_fd, buf, n, 0);
            write(STDOUT_FILENO, buf, n);
            //
        }

        //send/recv?
    }
}