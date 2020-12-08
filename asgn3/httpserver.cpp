#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>

#include <string>       //c++ strings
#include <string.h>     //memset
#include <stdlib.h>     //atoi
#include <unistd.h>     //write
#include <fcntl.h>      //open
#include <ctype.h>      //isalnum
#include <errno.h>      
#include <err.h>
#include <time.h>       //time
#include <dirent.h>     //for parsing directory

#include <netinet/in.h> //inet_aton
#include <arpa/inet.h>  //inet_aton

#include <stdio.h>      //printf, perror
#include <iostream>     //cout

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

struct flags {
    bool exists;            //flag for whether file already exists
    bool first_parse;       //flag for parsing first line of header
    bool good_name;         //flag for whether name is valid
    bool fileB;
    bool fileR;
    bool fileL;
};

void clearFlags(struct flags* flag){
    flag->exists = false;
    flag->first_parse = true;
    flag->good_name = false;
    flag->fileB = false;
    flag->fileR = false;
    flag->fileL = false;
}

struct LinkedList{
    int data;
    struct LinkedList* next;
};

void append(struct LinkedList** head, int newData){
    struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList)); //alloc new node
    struct LinkedList* last = *head; //point *last to head of list

    newNode->data = newData; //put in data
    newNode->next = NULL; //newNode will be tail, so next == NULL
    
    //if list is empty, newNode is head
    if(*head == NULL){
        *head = newNode;
        return;
    }

    //traverse list until last node is found
    while(last->next != NULL){
        last = last->next;
    }

    last->next = newNode; //update last
    return;
}

void sendList(int comm_fd, struct LinkedList* node){
    while(node != NULL){
        char str[25];
        sprintf(str, "%d", node->data);
        strcat(str, "\n");
        send(comm_fd, str, strlen(str), 0);
        node = node->next;
    }
}

void clearList(struct LinkedList** head){
    struct LinkedList* curr = *head;
    struct LinkedList* next;

    while(curr != NULL){
        next = curr->next;
        free(curr);
        curr = next;
    }

    *head = NULL;
}

//prints out linked list in this format: data1->data2->data3...
void printList(struct LinkedList* node){
    while(node != NULL){
        printf("%d", node->data);
        if(node->next != NULL) printf("->");

        node = node->next;
    }
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

// void syscallError(int fd, struct httpObject* request){
//     if(fd == -1){
//         request->status_code = 500;
//         // printf("errno %d: %s\n", errno, strerror(errno));
//         // fflush(stdout);
//         construct_response(fd, request);
//     }
// }

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
            if(filename[1] == '/'){ //this is safer since we don't remove anything until we are for sure it's "r/" and not a reg file starting with an r
                memmove(filename, filename+2, strlen(filename)); //remove "r/"

                //check that the provided timestamp is made up of all digits
                for(uint i=0; i<strlen(filename); i++){
                    if(!isalnum(filename[i])){
                        flag->good_name = false;
                        return false;
                    }
                }

                flag->good_name = true;
                flag->fileR = true;
                return true;
            }
            else if(strlen(filename) == 1){//only r 
                memmove(filename, filename+1, strlen(filename)); //remove "r" so filename should be empty
                printf("filename is %s\n", filename);
                flag->good_name = true;
                flag->fileR = true;
                return true;
            }           

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

void copyFiles(char* destination, int source){
    char buffer[SIZE];
    int dest = open(destination, O_CREAT | O_RDWR | O_TRUNC);

    //copy content from source to dest
    while(read(source, buffer, 1) != 0){
        int check = write(dest, buffer, 1);
        if(check == -1){
            perror("writing in copyFiles()");
            break;
        } 
    }

    //close files
    close(dest);
}

void get_request (int comm_fd, struct httpObject* request, struct flags* flag, char* buf){
    memset(buf, 0, SIZE);

    //special request files are assumed not to exist, so no point in trying to open() them
    if(flag->fileB){
        //create name for folder (append timestamp to "./backup-")
        time_t t = time(NULL);

        char timestamp[20];
        snprintf(timestamp, 20, "%ld", t);
        char backup[30] = "./backup-";
        strcat(backup, timestamp);

   
        //create new folder
        int folder = mkdir(backup, 0777);
        if(folder == -1) perror("mkdir");

        strcat(backup, "/"); //makes it easier to strcat filenames to the end later

        //copy files from cwd to new folder
        DIR *d;
        struct dirent *dir;
        d = opendir(".");

        if(d){
            while((dir = readdir(d)) != NULL){
                //check whether file is a directory
                struct stat path_stat;
                stat(dir->d_name, &path_stat);
                int isfile = S_ISREG(path_stat.st_mode);

                //file is not a folder, go to next file
                if(isfile == 0){
                    continue;
                }

                //check if filename is valid
                int source = open(dir->d_name, O_RDONLY); //open source to copy from
                if(source == -1){
                    // perror("can't open source file");
                    continue; //skip backing up forbidden files: https://piazza.com/class/kfqgk8ox2mi4a1?cid=471
                }

                //take out later: don't want to accidentally corrupt our working file until everything works correctly
                if(strncmp(dir->d_name, "httpserver", 10) == 0){
                    continue;
                    }

                if(strcmp(dir->d_name, "Makefile") == 0){
                    continue;
                }
                //------------------------------------------------------------------------------------------------------

                char filepath[100];
                strncpy(filepath, backup, 21);
                strcat(filepath, dir->d_name);

                copyFiles(filepath, source);
                close(source);
            }
            closedir(d);

        }else{
            perror("d not a dir");
        }

    }else if(flag->fileR){
        if(strlen(request->filename) != 0){ //recovery timestamp specified
            char backup[50] = "./backup-";
            strcat(backup, request->filename);
            strcat(backup, "/");

            DIR *d;
            struct dirent *dir;
            d = opendir(backup); 
            if(d){
                while((dir = readdir(d)) != NULL){
                    //check if file is a directory
                    char sourcePath[100];
                    strcpy(sourcePath, backup);
                    strcat(sourcePath, dir->d_name);
                    struct stat path_stat;
                    stat(sourcePath, &path_stat);
                    int isfile = S_ISREG(path_stat.st_mode);

                    //file is a folder, go to next file
                    if(isfile == 0){
                        continue;
                    }

                    //check if filename is valid
                    int source = open(sourcePath, O_RDONLY); //open source to copy from
                    if(source == -1){
                        // perror("can't open source file");
                        if(errno == ENOENT){ //no such file or dir
                            request->status_code = 404;
                            construct_response(comm_fd, request);

                        }else{
                            request->status_code = 500;
                            construct_response(comm_fd, request);
                        }
                    }
                    
                    //take out later: don't want to accidentally corrupt our working file until everything works correctly
                    if(strncmp(dir->d_name, "httpserver", 10) == 0){
                    continue;
                    }

                    if(strcmp(dir->d_name, "Makefile") == 0){
                        continue;
                    }
                    //------------------------------------------------------------------------------------------------------

                    char filepath[100];
                    memset(filepath, 0, 100); //commented back in to fix the appending error
                    strncpy(filepath, "./", 2);
                    strcat(filepath, dir->d_name);
                    printf("filepath is %s\n", filepath);
                    copyFiles(filepath, source);
                    close(source);
                }
                closedir(d);

            }else{
                //invalid d
                perror("d not a dir");
                request->status_code = 404;
                construct_response(comm_fd, request);
                return;
            }

        }else{ //recover timestamp not specified                          
            //determine which backup is the most recent one (bigger number = newer)
            int newtime = 0;
            char newest[50];

            DIR *d;
            struct dirent *dird;
            d = opendir(".");
            
            if(d){
                while((dird = readdir(d)) != NULL){
                    //check whether file is a directory
                    struct stat path_stat;
                    stat(dird->d_name, &path_stat);
                    int isfile = S_ISREG(path_stat.st_mode);

                    //file is not a directory, go to next file
                    if(isfile == 1){ 
                        continue;
                    }

                    //look for folders starting with "backup-"
                    if(strncmp(dird->d_name, "backup-", 7) == 0){
                        char folder[50];
                        strncpy(folder, dird->d_name, strlen(dird->d_name));
                        char* token = strtok(folder, "backup-");
                        
                        int timestamp = atoi(token);
                        if(timestamp > newtime){   //if curr timestamp > newest timestamp
                            newtime = timestamp;   //update newest timestamp
                            memset(newest, 0, 50); //insurance
                            strncpy(newest, dird->d_name, strlen(dird->d_name)); //update newest backup folder name
                        }

                    }else{ //don't care about non backup folders
                        continue;
                    }
                }  
                
            }else{
                perror("d not a dir");
            }

            closedir(d);
            
            //copy files from that backup folder to cwd
            char backup[50] = "./";
            strcat(backup, newest);
            strcat(backup, "/"); 

            DIR *b;
            struct dirent *dirb;
            b = opendir(backup);

            if(b){
                while((dirb = readdir(b)) != NULL){
                    //check if file is a directory
                    char sourcePath[100];
                    strcpy(sourcePath, backup);
                    strcat(sourcePath, dirb->d_name);
                    struct stat path_stat;
                    stat(sourcePath, &path_stat);
                    int isfile = S_ISREG(path_stat.st_mode);

                    //file is a folder, go to next file
                    if(isfile == 0){
                        continue;
                    }

                    //check if filename is valid
                    int source = open(sourcePath, O_RDONLY); //open source to copy from
                    if(source == -1){
                        perror("can't open source file");
                    }

                    //take out later: don't want to accidentally corrupt our working file until everything works correctly
                    if(strncmp(dirb->d_name, "httpserver", 10) == 0){
                    continue;
                    }

                    if(strcmp(dirb->d_name, "Makefile") == 0){
                        continue;
                    }
                    //------------------------------------------------------------------------------------------------------

                    char filepath[100];
                    memset(filepath, 0, 100);
                    strncpy(filepath, "./", 2);
                    strcat(filepath, dirb->d_name);

                    copyFiles(filepath, source);
                    close(source);
                }
                closedir(b);
            
            }else{
                perror("b not a dir");
            }
        }
        
    }else if(flag->fileL){
        DIR *d;
        struct dirent *dir;
        d = opendir(".");
        
        request->status_code = 200;
       
        int contentLength = 0;
        struct LinkedList* list = NULL; //new, empty list

        //Loop through all things in the server directory
        if(d){
            while((dir = readdir(d)) != NULL){ 
                //if not backup file, continue
                if(strncmp(dir->d_name, "backup", 6) != 0){
                    continue;
                }

                //check if file is a directory
                struct stat path_stat;
                stat(dir->d_name, &path_stat);
                int isfile = S_ISREG(path_stat.st_mode);

                //file is not a directory, go to next file
                if(isfile!=0){
                    continue;
                }

                //it is a directory named "backup-..."
                char copy[50];
                strcpy(copy, dir->d_name);
                char* timestamp = strtok(copy, "backup-");  //get timestamp by itself
                contentLength += (strlen(timestamp) + 1);   //+1 is for "\n"

                append(&list, atoi(timestamp));             //append timestamp to list
            }

            request->content_length = contentLength;        //set content_length accordingly
            construct_response(comm_fd, request);           //send response

            sendList(comm_fd, list);                        //send timestamps
            clearList(&list);                               //dealloc list

            closedir(d);
            return; 
        }
    }

    //construct response & return: no need to do rest of GET
    if(flag->fileR){
        request->status_code = 200;
        construct_response(comm_fd, request);
        return;
    }

    if(flag->fileB){
        request->status_code = 201;
        construct_response(comm_fd, request);
        return;
    }

    int file;
    file = open(request->filename, O_RDONLY);

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
    int wfile; //to check if write() is successful/how many bytes got written
    int check;

    //check whether file already exists
    if((check = access(request->filename, F_OK)) != -1){
        flag->exists = true;
    }else{
        flag->exists = false;
    }

    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    if(file == -1){
        perror("open");
        request->status_code = 500;
    }

    int bytes_recv;
    //no body bytes in buf, safe to recv
    if((request->content_length != 0) && (strlen(request->body) == 0)){    
        while(request->content_length > 0){
            if(request->content_length < SIZE){                                 //buf has stuff in it, but not completely full
                bytes_recv = recv(comm_fd, buf, request->content_length, 0);    //recv content_length bytes of body

            }else{                                                              //buf is completely full, can recv SIZE bytes
                bytes_recv = recv(comm_fd, buf, SIZE, 0);                       //recv SIZE bytes of body
            }

            if((request->status_code != 400) || (request->status_code != 500)){ //if bad request, don't write 
                wfile = write(file, buf, bytes_recv);
                if(wfile == -1) request->status_code = 500;
            }

            request->content_length = request->content_length - bytes_recv;     //decrement content_length by # bytes written
        }

        //if not bad req: if file already exists return 200, else return 201
        if((request->status_code != 400) || (request->status_code != 500)){
            if(flag->exists == true) request->status_code = 200;
            else request->status_code = 201;
        }

    //at least part of body was in buf, need to write() before checking whether we need to recv() again
    }else if((request->content_length != 0) && (strlen(request->body) != 0)){            
        if((request->status_code != 400) || (request->status_code != 500)){ //write what was in buf already
            wfile = write(file, request->body, strlen(request->body));
            if(wfile == -1) request->status_code = 500;
        }

        request->content_length = request->content_length - strlen(request->body);

        if((request->collector != NULL) && (strlen(request->collector) != 0)){  //ensure that we write everything from buf
            if((request->status_code != 400) || (request->status_code != 500)){
                wfile = write(file, request->collector, strlen(request->collector));
                if(wfile == -1) request->status_code = 500;
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

                if((request->status_code != 400) || (request->status_code != 500)){
                    wfile = write(file, buf, bytes_recv);                           
                    if(wfile == -1) request->status_code = 500;
                }

                request->content_length = request->content_length - bytes_recv;
            }            
        }

        //if not bad req: if file already exists return 200, else return 201
        if((request->status_code != 400) || (request->status_code != 500)){
            if(flag->exists == true) request->status_code = 200;
            else request->status_code = 201;
        }

    //no content length specified: server copies data until read() reads EOF
    }else{
        //make sure to write already recv-ed body before recving more to write
        if(strlen(request->body) != 0){
            if((request->status_code != 400) || (request->status_code != 500)){
                wfile = write(file, request->body, strlen(request->body));
                if(wfile == -1) request->status_code = 500;
            }
        }

        //to ensure that we have written EVERYTHING previously recv-ed
        if((request->collector != NULL) && (strlen(request->collector) != 0)){
            if((request->status_code != 400) || (request->status_code != 500)){
                wfile = write(file, request->collector, strlen(request->collector));
                if(wfile == -1) request->status_code = 500;

            }
        }

        //continue recv-ing and writing until EOF
        while((bytes_recv = recv(comm_fd, buf, SIZE, 0)) > 0){
            if((request->status_code != 400) || (request->status_code != 500)){
                wfile = write(file, buf, bytes_recv);
                if(wfile == -1) request->status_code = 500;
            }
        }
    }     

    construct_response(comm_fd, request);
    close(file); 
}

void set_first_parse (struct flags* flag, bool value){
    flag->first_parse = value;
}

void executeFunctions (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    int bytes_recv = recv(comm_fd, buf, SIZE, 0); //first recv

    sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

    if(bytes_recv == -1){
        request->status_code = 500;
        //If get req is 500 no need to execute to completion
        if(strcmp(request->type, "GET") == 0){
            construct_response(comm_fd, request);
            return;
        }
    } 

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
        get_request(comm_fd, request, flag, buf);
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
        printf("waiting for a connection...\n");
        fflush(stdout);

        int comm_fd = accept(server_socket, &client_addr, &client_addrlen);
        set_first_parse(&flag, true);
        executeFunctions(comm_fd, &request, buf, &flag);
        memset(buf, 0, SIZE);
        clearStruct(&request);
        clearFlags(&flag);
        close(comm_fd);
    }
    return EXIT_SUCCESS;
}
