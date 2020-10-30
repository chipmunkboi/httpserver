#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdlib.h>

#include <string.h>     //memset
#include <stdlib.h>     //atoi
#include <unistd.h>     //write
#include <fcntl.h>      //open
#include <ctype.h>      //isalnum

#include <errno.h>
#include <err.h>

#include <netinet/in.h> //inet_aton
#include <arpa/inet.h>  //inet_aton

#include <stdio.h>      //printf, perror

#define SIZE 5000

struct httpObject {
    char type[4];           //PUT, GET
    char httpversion[9];    //HTTP/1.1
    char filename[12];      //10 character ASCII name
    int status_code;        //200, 201, 400, 403, 404, 500
    ssize_t content_length;
};

char* getCode(int status_code){
    if(status_code==200){
        return "200 OK";
    }else if(status_code==201){
        return "201 Created";
    }else if(status_code==400){
        return "400 Bad Request";
    }else if(status_code==403){
        return "403 Forbidden";
    }else if(status_code==404){
        return "404 Not Found";
    }else if(status_code==500){
        return "500 Internal Server Error";
    }
}

void construct_response(ssize_t comm_fd, struct httpObject* request){
    char length_string[20];
    sprintf(length_string, "%ld", request->content_length);
    
    char response[50];
    strcpy (response, request->httpversion);
    strcat (response, " ");
    strcat (response, getCode(request->status_code));
    strcat (response, "\r\n");
    strcat (response, "Content-Length: ");
    strcat (response, length_string);
    strcat (response, "\r\n\r\n");
    send(comm_fd, response, strlen(response), 0);
}

//returns TRUE if name is valid and FALSE if name is invalid
bool valid_name(struct httpObject* request){
    if(request->filename[0] != '/') return false;
    else memmove(request->filename, request->filename+1, strlen(request->filename));
    
    //check that name is 10 char long
    if(strlen(request->filename) != 10){
        return false;
    }

    //check that all chars are ASCII chars
    for(int i=0; i<10; i++){
        if(!isalnum(request->filename[i])) return false;
    }

    return true;
}

void parse_request(ssize_t comm_fd, struct httpObject* request, char* buf){
    sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

    //check that httpversion is "HTTP/1.1"
    if(strcmp(request->httpversion, "HTTP/1.1") != 0){
        request->status_code = 400;
        construct_response(comm_fd, request);
    
    //check that filename is made of 10 ASCII characters
    }else if(!valid_name(request)){
        request->status_code = 400;
        construct_response(comm_fd, request);

    }else{
        char* token = strtok(buf, "\r\n");
        while(token){
            if(strncmp(token, "Content-Length:", 15) == 0){
                sscanf(token, "%*s %ld", &request->content_length);
            }else if(strncmp(token, "\r\n", 2)==0){
                break;
            }
            token = strtok(NULL, "\r\n");
        }
    }
}

void get_request(ssize_t comm_fd, struct httpObject* request, char* buf){
    memset(buf, 0, sizeof(buf));
    int file = open(request->filename, O_RDONLY);

    if (file == -1){
        // warn("%s", request->filename);
        if(errno==ENOENT){ 
            request->status_code = 404;
        }else if(errno==EACCES){ 
            request->status_code = 403;
        }else{
            request->status_code = 400;
        }
        construct_response(comm_fd, request);
    }else{
        request->status_code = 200;
        struct stat size;
        fstat(file, &size);
        request->content_length = size.st_size;

        construct_response(comm_fd, request);

        while(read(file, buf, 1) != 0){
            int send_check = send(comm_fd, buf, 1, 0); //printing on server
            // if (send_check == -1){
            //     warn("%s", request->filename);
            // } 
        } 
    }
    
    close(file);
}

void put_request(ssize_t comm_fd, struct httpObject* request, char* buf){
    memset(buf, 0, sizeof(buf));
    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    if(file == -1){
        request->status_code = 500;
        construct_response(comm_fd, request);

    }else{
        ssize_t bytes_read;

        if(request->content_length != 0){          
            while(request->content_length > 0){
                bytes_read = recv(comm_fd, buf, SIZE, 0);
                int write_check = write(file, buf, bytes_read);
                request->content_length = request->content_length - bytes_read;
            }

        }else{
            while(bytes_read = recv(comm_fd, buf, SIZE, 0) > 0){
                int write_check = write(file, buf, bytes_read);
            }
        }

        //if all bytes were received and written, success
        if(request->content_length == 0) request->status_code = 201;
        else request->status_code = 500;
        

        construct_response(comm_fd, request);

    } 
    close(file); 
}

void executeFunctions(ssize_t comm_fd, struct httpObject* request, char* buf){
    //use comm_fd to comm with client
    recv(comm_fd, buf, SIZE, 0);    //while bytes are still being received...
    parse_request(comm_fd, request, buf);   //Parses the header to the request variables
    if(strcmp(request->type, "GET") == 0){
        get_request(comm_fd, request, buf);

    }else if(strcmp(request->type, "PUT") == 0){
        put_request(comm_fd, request, buf);

    }else{
        request->status_code = 400;
        construct_response(comm_fd, request);
    }
    // fflush(stdin);
    // send(comm_fd, buf, n, 0);               //...send buf contents to comm_fd... (client)
    // write(STDOUT_FILENO, buf, n);           //...and write buf contents to stdout (server)
}

//port is set to user-specified number or 80 by default
int getPort (char argtwo[]){
    int port;

    if (argtwo != NULL){
        port = atoi(argtwo);

        if (port < 1024){
            printf("Port Error: Port numbers must be above 1024\n");
            exit(EXIT_FAILURE);
        }
    }else{
        port = 80;
    }

    return port;
}

int main (int argc, char *argv[]){
    int port = getPort(argv[2]);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
        
    server_addr.sin_family = AF_INET;   
    inet_aton(argv[1], &server_addr.sin_addr);
    server_addr.sin_port = htons(port);
    socklen_t addrlen = sizeof(server_addr);

    //create server socket file descriptor
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("server_socket");
        exit(EXIT_FAILURE);
    }

    //setsockopt: helps in reusing address and port
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

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

    struct sockaddr client_addr;
    socklen_t client_addrlen;
    char buf[SIZE];
    struct httpObject request;
       
    while(true){
        //accept incoming connection
        int comm_fd = accept(server_socket, &client_addr, &client_addrlen); //accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
        executeFunctions(comm_fd, &request, buf);
        //while(true){
            //use comm_fd to comm with client
            //int n = recv(comm_fd, buf, SIZE, 0);    //while bytes are still being received...
            //parse_request(comm_fd, &request, buf); 
            //fflush(stdin);
            //if(n == 0) break;
            //send(comm_fd, buf, n, 0);               //...send buf contents to comm_fd... (client)
            //write(STDOUT_FILENO, buf, n);           //...and write buf contents to stdout (server)
        //}
        //receive header
        //send http response
        close(comm_fd);
    }
    return EXIT_SUCCESS;
}
