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

#define SIZE 4096       //4 KB

struct httpObject {
    char type[4];           //PUT, GET
    char httpversion[9];    //HTTP/1.1
    char filename[50];      //10 character ASCII name
    char body[SIZE];
    char* collector;
    int status_code;        //200, 201, 400, 403, 404, 500
    ssize_t content_length; //length of file
};

struct flags {
    bool exists;            //flag for whether file already exists
    bool first_parse;       //flag for parsing first line of header
    bool good_name;         //flag for whether name is valid
    bool fileB;
    bool fileR;
    bool fileL;
};

//used to check that all parts of struct are correct
void printStruct(struct httpObject* request){
    printf("type: %s\n", request->type);
    printf("ver: %s\n", request->httpversion);
    printf("file: %s\n", request->filename);
    printf("content length: %ld\n", request->content_length);
}

void clearStruct(struct httpObject* request){
    memset(request->type, 0, 4);
    memset(request->httpversion, 0, 9);
    memset(request->filename, 0, 50);
    memset(request->body, 0, SIZE);
    request->collector = NULL;
    request->status_code = 0;
    request->content_length = 0;
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

void syscallError(int fd, struct httpObject* request){
    if(fd == -1){
        request->status_code = 500;
        // printf("errno %d: %s\n", errno, strerror(errno));
        // fflush(stdout);
        construct_response(fd, request);
    }
}

//returns TRUE if name is valid and FALSE if name is invalid
bool valid_name (char* filename, struct flags* flag){
    //remove "/" from front of file name
    if(filename[0] == '/'){
        memmove(filename, filename+1, strlen(filename));
    }

    // check that name is 10 char long; if not, check if name is a special request
    if(strlen(filename) != 10){
        if((strlen(filename) == 1) && (strncmp(filename, "b", 1) == 0)){ 
            flag->good_name = true;
            flag->fileB = true;
            return true;

        }else if(strncmp(filename, "r", 1) == 0){ //might not be just "r" (i.e. r/[timestamp])
            flag->good_name = true;
            flag->fileR = true;
            return true;

        }else if((strlen(filename) == 1) && (strncmp(filename, "l", 1) == 0)){
            flag->good_name = true;
            flag->fileL = true;
            return true;
            
        }else{
            flag->good_name = false;
            return false;
        }
    }

    // check that all chars are ASCII chars
    for(int i=0; i<10; i++){
        if(!isalnum(filename[i])){
            flag->good_name = false;
            return false;
        }
    }

    flag->good_name = true;
    return true;
}

void get_request (int comm_fd, struct httpObject* request, char* buf){
    memset(buf, 0, SIZE);
    
    //special request files are assumed not to exist, so no point in trying to open() them
    if(fileB){
        //create name for folder (append timestamp to "./backup-")
        //create new folder
        //copy files from cwd to new folder

    }else if(fileR){
        if(strlen(request->filename) == 1){ //recover timestamp not specified
            //determine which backup is the most recent one
            //copy files from that backup folder to cwd

        }else{                              //recovery timestamp specified
            //look through all backup folders to find the specified timestamp
            //if found
                //copy files from that backup folder to cwd
            //else
                //request->status_code = 404;
        }
        
    }else if(fileL){
        //look through directory and return timestamps of backups
    }
    //construct response; no need to do rest of GET
    //close the file

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
    int wfile;      //to check if write() is successful/how many bytes got written
    int check;

    //check whether file already exists
    if((check = access(request->filename, F_OK)) != -1){
        flag->exists = true;
    }else{
        flag->exists = false;
    }
    // syscallError(check, request);

    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    if(file == -1){perror("open");}
    // syscallError(file, request);

    int bytes_recv;
    //no body bytes in buf, safe to recv
    if((request->content_length != 0) && (strlen(request->body) == 0)){    
        while(request->content_length > 0){
            if(request->content_length < SIZE){                                 //buf has stuff in it, but not completely full
                bytes_recv = recv(comm_fd, buf, request->content_length, 0);    //recv content_length bytes of body

            }else{                                                              //buf is completely full, can recv SIZE bytes
                bytes_recv = recv(comm_fd, buf, SIZE, 0);                       //recv SIZE bytes of body
            }

            if(request->status_code != 400){                                    //if bad request, don't write 
                wfile = write(file, buf, bytes_recv);
                // syscallError(comm_fd, wfile, request);
            }

            request->content_length = request->content_length - bytes_recv;     //decrement content_length by # bytes written
        }

        //if not bad req: if file already exists return 200, else return 201
        if(request->status_code != 400){
            if(flag->exists == true) request->status_code = 200;
            else request->status_code = 201;
        }

    //at least part of body was in buf, need to write() before checking whether we need to recv() again
    }else if((request->content_length != 0) && (strlen(request->body) != 0)){            
        if(request->status_code != 400){                                        //write what was in buf already
            wfile = write(file, request->body, strlen(request->body));
            // syscallError(comm_fd, wfile, request);
        }

        request->content_length = request->content_length - strlen(request->body);

        if((request->collector != NULL) && (strlen(request->collector) != 0)){  //ensure that we write everything from buf
            if(request->status_code != 400){
                wfile = write(file, request->collector, strlen(request->collector));
                // syscallError(comm_fd, wfile, request);
            }
            request->content_length = request->content_length - strlen(request->collector);
        }

        //done writing from buf, can now start recv-ing
        if(request->content_length != 0){                                           
            while(request->content_length > 0){
                if(request->content_length < SIZE){                                 
                    bytes_recv = recv(comm_fd, buf, request->content_length, 0);  

                }else{                                                             
                    bytes_recv = recv(comm_fd, buf, SIZE, 0);                       
                }

                if(request->status_code != 400){
                    wfile = write(file, buf, bytes_recv);                           
                    // syscallError(comm_fd, wfile, request);
                }

                request->content_length = request->content_length - bytes_recv;
            }            
        }

        //if not bad req: if file already exists return 200, else return 201
        if(request->status_code != 400){
            if(flag->exists == true) request->status_code = 200;
            else request->status_code = 201;
        }

    //no content length specified: server copies data until read() reads EOF
    }else{
        //make sure to write already recv-ed body before recving more to write
        if(strlen(request->body) != 0){
            if(request->status_code != 400){
                write(file, request->body, strlen(request->body));
            }
        }

        //to ensure that we have written EVERYTHING previously recv-ed
        if((request->collector != NULL) && (strlen(request->collector) != 0)){
            if(request->status_code != 400){
                write(file, request->collector, strlen(request->collector));
            }
        }

        //continue recv-ing and writing until EOF
        while((bytes_recv = recv(comm_fd, buf, SIZE, 0)) > 0){
            if(request->status_code != 400){
                write(file, buf, bytes_recv);
            }
        }
    }     

    construct_response(comm_fd, request);
    close(file); 
}

void set_first_parse (struct flags* flag, bool value){
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
        }else if(!valid_name(request->filename, flag)){
            request->status_code = 400;
            construct_response(comm_fd, request);
            // return;
        }

        set_first_parse(flag, false);
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
    int bytes_recv = recv(comm_fd, buf, SIZE, 0); //first recv
    // syscallError(comm_fd, bytes_recv, request);

    sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

    //check that httpversion is "HTTP/1.1"
    if(strcmp(request->httpversion, "HTTP/1.1") != 0){
        request->status_code = 400;
        //If get req is 400 no need to execute to completion
        if(strcmp(request->type, "GET") == 0){
            construct_response(comm_fd, request); //commented to help fix invalid name problem
            return;
        }
    }

    //check that filename is made of 10 ASCII characters
    else if(!valid_name(request->filename, flag)){
        request->status_code = 400;
        //If get req is 400 no need to execute to completion
        if(strcmp(request->type, "GET") == 0){
            construct_response(comm_fd, request); //commented to help fix invalid name problem
            return;
        }
    }

    char temp[SIZE];
    strncpy(temp, buf, SIZE); //protects buf from being modified
    size_t token_counter = 0;

    //parse buf contents until end of header (\r\n\r\n) is reached
    char* token = strtok(temp, "\n");
    char* ptr = buf;
    while(token != NULL){
        if(strncmp(token, "Content-Length:", 15) == 0){
            sscanf(token, "%*s %ld", &request->content_length);
        }

        if(strncmp(token, "\r", 2) == 0){
            token_counter += 2;                         //+2 is to make up for the two \n's that are going to be cut off
            break;                                      //break: don't need to tokenize any further
        }
        token_counter += strlen(token) + 1;             //+1 is to make up for the one \n that gets cut off
        token = strtok(NULL, "\n");
    }

    ptr = ptr + token_counter;                          //move ptr to front of body  

    size_t copyBytes = (strlen(buf)) - token_counter;   //copyBytes = # of body bytes that have been recv-ed

    strncpy(request->body, ptr, copyBytes);             //strncpy copyBytes into ptr (whole body)

    //CHECK: experimental
    if(copyBytes != strlen(request->body)){
        if(copyBytes == (strlen(request->body)+1)){     //if body is missing one byte, it's probably due to an \n being cut
        strcat(request->body, "\n");
        
        }else{                                          //if # of body bytes != # body bytes we have...
            request->collector = &buf[strlen(request->body)+token_counter]; //...retrieve the rest from the original buffer buf
        }
    }

    if(strcmp(request->type, "PUT") == 0){ //body is present, read message body
        put_request(comm_fd, request, buf, flag);
    }else if(strcmp(request->type, "GET") == 0){
        get_request(comm_fd, request, buf);
    }
    else{
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
    inet_aton(argv[1], &server_addr.sin_addr); //get host address (e.g. localhost) (CHECK)
    server_addr.sin_port = htons(port);
    socklen_t addrlen = sizeof(server_addr);

    //create server socket file descriptor
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        // perror("server_socket");
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
        set_first_parse(&flag, true);
        executeFunctions(comm_fd, &request, buf, &flag);
        memset(buf, 0, SIZE);
        clearStruct(&request);
        close(comm_fd);
    }
    return EXIT_SUCCESS;
}
