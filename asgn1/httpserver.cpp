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

#define SIZE 4096

struct httpObject {
    char type[4];           //PUT, GET
    char httpversion[9];    //HTTP/1.1
    char filename[12];      //10 character ASCII name
    int status_code;        //200, 201, 400, 403, 404, 500    
    ssize_t content_length; //length of file
};

struct flags {
    bool exists;            //flag for whether file already exists
    bool first_parse;       //flag for parsing first line of header
    bool good_name;         //flag for whether name is valid
};

void printStruct(struct httpObject* request){
    printf("type: %s\n", request->type);
    printf("ver: %s\n", request->httpversion);
    printf("file: %s\n", request->filename);
    printf("content length: %ld\n", request->content_length);
}

void clearStruct(struct httpObject* request){
    memset(request->type, 0, SIZE);
    memset(request->httpversion, 0, SIZE);
    memset(request->filename, 0, SIZE);
    request->status_code = 0;
    request->content_length = NULL;
}

void clearFlags(struct flags* flag){
    flag->exists = false;
    flag->first_parse = true;
    flag->good_name = false;
}

const char* getCode (int status_code){
    char const *code;
    if(status_code==200){
        code = "200 OK";
    }else if(status_code==201){
        code =  "201 Created";
    }else if(status_code==400){
        code = "400 Bad Request";
    }else if(status_code==403){
        code = "403 Forbidden";
    }else if(status_code==404){
        code = "404 Not Found";
    }else if(status_code==500){
        code = "500 Internal Server Error";
    }else{
        code = "";
    }

    return code;
}

void construct_response (int comm_fd, struct httpObject* request){
    printStruct(request);
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

    //
    printf("RESPONSE:\n");
    printf("%s\n", response);
    fflush(stdout);
    //
    send(comm_fd, response, strlen(response), 0);
}

void syscallError(int fd, struct httpObject* request){
    printf("\n\nin syscall error\n");
    fflush(stdout);
    if(fd == -1){
        request->status_code = 500;
    }

    printf("errno: %s\n", strerror(errno));
    fflush(stdout);
    construct_response(fd, request);
}

//returns TRUE if name is valid and FALSE if name is invalid
bool valid_name (struct httpObject* request, struct flags* flag){
    if(request->filename[0] == '/'){
        memmove(request->filename, request->filename+1, strlen(request->filename));
    }
    
    //check that name is 10 char long
    if(strlen(request->filename) != 10){
        flag->good_name = false;
        return false;
    }

    //check that all chars are ASCII chars
    for(int i=0; i<10; i++){
        if(!isalnum(request->filename[i])){
            flag->good_name = false;
            return false;
        }
    }

    flag->good_name = true;
    return true;
}

void get_request (int comm_fd, struct httpObject* request, char* buf){
    memset(buf, 0, SIZE);
    int file = open(request->filename, O_RDONLY);

    if (file == -1){
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
            send(comm_fd, buf, 1, 0); 
        } 
    }
    
    close(file);
}

void put_request (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    printf("in PUT\n");
    fflush(stdout);
    memset(buf, 0, SIZE);

    //check whether file already exists
    if(access(request->filename, F_OK) != -1){
        flag->exists = true;
    }else{
        flag->exists = false;
    }

    printf("before open\n");
    fflush(stdout);
    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    syscallError(file, request);
    printf("after open\n");
    fflush(stdout);

    int bytes_recv;

    if(request->content_length != 0){          
        while(request->content_length > 0){
            if(request->content_length < SIZE){
                bytes_recv = recv(comm_fd, buf, request->content_length, 0);
            }else{
                bytes_recv = recv(comm_fd, buf, SIZE, 0);
            }
            syscallError(bytes_recv, request);

            int wfile = write(file, buf, bytes_recv);
            syscallError(wfile, request);
            request->content_length = request->content_length - bytes_recv;
        }

        //if file already exists return 200, if not return 201
        if(flag->exists == true) request->status_code = 200;
        else request->status_code = 201;

    //server copies data until read() reads EOF
    }else{
        bytes_recv = read(comm_fd, buf, SIZE);
        syscallError(bytes_recv, request);
        int wfile = write(file, buf, bytes_recv);
        syscallError(wfile, request);

        while(bytes_recv == SIZE){
            bytes_recv = read(comm_fd, buf, SIZE);
            int wfile2 = write(file, buf, bytes_recv);
            syscallError(wfile2, request);
        }

        if(flag->exists == true) request->status_code = 200;
        else request->status_code = 201;
    }       

    construct_response(comm_fd, request);
    close(file); 
}

void set_flag (struct flags* flag, bool value){
    flag->first_parse = value;
}

void parse_request (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    if(flag->first_parse){
        sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

        //check that httpversion is "HTTP/1.1"
        if(strcmp(request->httpversion, "HTTP/1.1") != 0){
            request->status_code = 400;
            construct_response(comm_fd, request);
            // return;
        
        //check that filename is made of 10 ASCII characters
        }else if(!valid_name(request, flag)){
            request->status_code = 400;
            construct_response(comm_fd, request);
            // return;
        }

        set_flag(flag, false);
    }

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

void executeFunctions (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    memset(buf, 0, SIZE);
    int bytes_recv = recv(comm_fd, buf, SIZE, 0);   //recv and parse once
    printf("Bytes recv: %d\n", bytes_recv);
    fflush(stdout);
    syscallError(bytes_recv, request);
    parse_request(comm_fd, request, buf, flag);

    if(!valid_name(request, flag)) return;
    if(strcmp(request->httpversion, "HTTP/1.1") != 0) return;

    while(bytes_recv == SIZE){                      //if buf is completely filled
        bytes_recv = recv(comm_fd, buf, SIZE, 0);   //recv again
        syscallError(bytes_recv, request);

        parse_request(comm_fd, request, buf, flag);       //parse for content length
        memset(buf, 0, SIZE);
    }

    if(strcmp(request->type, "GET") == 0){
        get_request(comm_fd, request, buf);

    }else if(strcmp(request->type, "PUT") == 0){
        put_request(comm_fd, request, buf, flag);

    }else{
        request->status_code = 500;
        construct_response(comm_fd, request);
    }
}

//port is set to user-specified number or 80 by default
int getPort (char argtwo[]){
    int port;

    if (argtwo != NULL){
        port = atoi(argtwo);

        if (port < 1024){
            exit(EXIT_FAILURE);
        }
    }else{
        port = 80;
    }

    return port;
}

int main (int argc, char *argv[]){
    (void)argc; //get rid of unused argc warning
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
    struct flags flag;

    while(true){
        //accept incoming connection
        int comm_fd = accept(server_socket, &client_addr, &client_addrlen);
        set_flag(&flag, true);
        executeFunctions(comm_fd, &request, buf, &flag);
        memset(buf, 0, SIZE);
        clearStruct(&request);
        close(comm_fd);
    }
    return EXIT_SUCCESS;
}
