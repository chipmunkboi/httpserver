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

#define SIZE 100

struct httpObject {
    char type[4];           //PUT, GET
    char httpversion[9];    //HTTP/1.1
    char filename[12];      //10 character ASCII name
    int status_code;        //200, 201, 400, 403, 404, 500
    bool exists;            //flag for whether file already exists
    bool first_parse;       
    ssize_t content_length;
};

// void clear_struct(struct httpObject* request){
//     request->type = NULL;
//     request->httpversion = NULL;
//     request->filename = NULL;
//     request->status_code = 0;
//     request->
// }

void print_struct(struct httpObject* request){
    printf("type = %s\n", request->type);
    printf("ver = %s\n", request->httpversion);
    printf("name = %s\n", request->filename);
    printf("code = %d\n", request->status_code);
    printf("content length = %ld\n", request->content_length);
}

const char* getCode(int status_code){
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

// void parse_request(ssize_t comm_fd, struct httpObject* request, char* buf){
//     printf("in parse req\n");
//     sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

//     //check that httpversion is "HTTP/1.1"
//     if(strcmp(request->httpversion, "HTTP/1.1") != 0){
//         request->status_code = 400;
//         construct_response(comm_fd, request);
    
//     //check that filename is made of 10 ASCII characters
//     }else if(!valid_name(request)){
//         request->status_code = 400;
//         construct_response(comm_fd, request);

//     }else{
//         char* token = strtok(buf, "\r\n");
//         while(token){
//             if(strncmp(token, "Content-Length:", 15) == 0){
//                 sscanf(token, "%*s %ld", &request->content_length);
//             }else if(strncmp(token, "\r\n", 2)==0){
//                 break;
//             }
//             token = strtok(NULL, "\r\n");
//         }
//     }
// }

void get_request(ssize_t comm_fd, struct httpObject* request, char* buf){
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
            send(comm_fd, buf, 1, 0); //printing on server
        } 
    }
    
    close(file);
}

void put_request(ssize_t comm_fd, struct httpObject* request, char* buf){
    printf("start of PUT\n");

    memset(buf, 0, SIZE);

    //check whether file already exists
    if(access(request->filename, F_OK) != -1){
        request->exists = true;
    }else{
        request->exists = false;
    }

    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    if(file == -1){
        request->status_code = 500;
        construct_response(comm_fd, request);

    }else{
        ssize_t bytes_recv;

        if(request->content_length != 0){          
            while(request->content_length > 0){
                bytes_recv = recv(comm_fd, buf, SIZE, 0);
                write(file, buf, bytes_recv);
                request->content_length = request->content_length - bytes_recv;
            }

            //if all bytes were received and written, success
            if(request->content_length == 0){
                //if file already exists return success if not return 201
                if(request->exists == true) request->status_code = 200;
                else request->status_code = 201;
                
            //connection closed before all bytes were sent
            }else{
                request->status_code = 500;
            }

        //server copies data until read() reads EOF
        }else{
            printf("in broken else\n");
            // while((bytes_read = read(comm_fd, buf, SIZE)) > 0){
            //     write(file, buf, bytes_read);
            // }

            while(!EOF){
                bytes_recv = recv(comm_fd, buf, SIZE, 0);
                write(file, buf, bytes_recv);
            }
            if(request->exists == true) request->status_code = 200;
            else request->status_code = 201;
        }       

        construct_response(comm_fd, request);
    } 
    printf("end of PUT\n");
    close(file); 
}

void set_flag(struct httpObject* request, bool value){
    request->first_parse = value;

    if(request->first_parse == true) printf("true\n");
    else printf("false\n");
}

void parse_request(ssize_t comm_fd, struct httpObject* request, char* buf){
    printf("in parse req\n");
    fflush(stdout);
    if(request->first_parse){
        printf("in first parse\n");
        sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

        printf("type = %s\nfile = %s\nver = %s\n", request->type, request->filename, request->httpversion);
        fflush(stdout);

        //check that httpversion is "HTTP/1.1"
        if(strcmp(request->httpversion, "HTTP/1.1") != 0){
            request->status_code = 400;
            construct_response(comm_fd, request);
        
        //check that filename is made of 10 ASCII characters
        }else if(!valid_name(request)){
            request->status_code = 400;
            construct_response(comm_fd, request);

        }

        set_flag(request, false);
    }

    char* token = strtok(buf, "\r\n");
    while(token){
        printf("token: %s\n", token);
        if(strncmp(token, "Content-Length:", 15) == 0){
            sscanf(token, "%*s %ld", &request->content_length);
        }else if(strncmp(token, "\r\n", 2)==0){
            break;
        }
        token = strtok(NULL, "\r\n");
    }

    //
    printf("content length = %ld\n", request->content_length);
    //
}

void executeFunctions(ssize_t comm_fd, struct httpObject* request, char* buf){
    memset(buf, 0, SIZE);
    int bytes_recv = recv(comm_fd, buf, SIZE, 0); 
    parse_request(comm_fd, request, buf);

    while(bytes_recv == SIZE){
        printf("bytes = %d\n", bytes_recv);
        bytes_recv = recv(comm_fd, buf, SIZE, 0);
        parse_request(comm_fd, request, buf);
    }

    print_struct(request);

    // //DANGER ZONE
    // memset(buf, 0, SIZE);

    // printf("before while\n");
    // fflush(stdout);

    // while(true){
    //     // printf("in while!\n");
    //     fflush(stdout);
    //     int bytes_recv = recv(comm_fd, buf, SIZE-1, 0);    //while bytes are still being received...
    //     printf("bytes = %d\n", bytes_recv);
    //     fflush(stdout);

    //     if(bytes_recv < 0){
    //         printf("error\n");
    //         fflush(stdout);
    //     }
    //     if(bytes_recv <= 0){
    //         printf("before break\n");
    //         break;
    //     }
    //     parse_request(comm_fd, request, buf);   //Parses the header to the request variables
    //     printf("out of parse\n");
        
    //     memset(buf, 0, SIZE);
    //     printf("-----\n");
    // }
    
    // printf("outside of while\n");
    // fflush(stdout);
    // //
    

    if(strcmp(request->type, "GET") == 0){
        printf("GET REQ\n");
        get_request(comm_fd, request, buf);

    }else if(strcmp(request->type, "PUT") == 0){
        printf("PUT REQ\n");

        put_request(comm_fd, request, buf);

    }else{
        printf("not GET or PUT\n");
        request->status_code = 400;
        construct_response(comm_fd, request);
    }
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
    
    while(true){
        //accept incoming connection
        int comm_fd = accept(server_socket, &client_addr, &client_addrlen);
        set_flag(&request, true);
        executeFunctions(comm_fd, &request, buf);
        memset(buf, 0, SIZE);
        close(comm_fd);
    }
    return EXIT_SUCCESS;
}
