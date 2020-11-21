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
#include <ctype.h>      //for isDigit()
#include <pthread.h>    //for threads

#include <dirent.h>     //for parsing directory

#include <queue>

#define SIZE 16384       //16 KB
using namespace std;
queue <int> commQ;

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

struct synchronization{
    // Declaration of thread condition variable 
    pthread_cond_t newReq = PTHREAD_COND_INITIALIZER; 
    // declaring mutex 
    pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER; 
    // char* string = "hello";
};

//used to check that all parts of struct are correct
void printStruct(struct httpObject* request){
    printf("type: %s\n", request->type);
    printf("ver: %s\n", request->httpversion);
    printf("file: %s\n", request->filename);
    printf("content length: %ld\n", request->content_length);
    fflush(stdout);
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
bool valid_name (struct httpObject* request, struct flags* flag){
    if(request->filename[0] == '/'){
        memmove(request->filename, request->filename+1, strlen(request->filename));
    }
    
    // COMMENT IN BEFORE SUBMITTING 
    // TOOK OUT FOR EASE OF TESTING
    //check that name is 10 char long
    // if(strlen(request->filename) != 10){
    //     flag->good_name = false;
    //     return false;
    // }

    //check that all chars are ASCII chars
    // for(int i=0; i<10; i++){
    //     if(!isalnum(request->filename[i])){
    //         flag->good_name = false;
    //         return false;
    //     }
    // }

    flag->good_name = true;
    return true;
}


void copyFiles(char* filename, int source){
    char buffer[SIZE];
    
    //Create the path
    char copy1[50] = "./copy1/";
    char copy2[50] = "./copy2/";
    char copy3[50] = "./copy3/";

    //Append file name to path
    strcat(copy1, filename);
    strcat(copy2, filename);
    strcat(copy3, filename);

    //
    // printf("%s\n", copy1);
    // printf("%s\n", copy2);
    // printf("%s\n", copy3);
    // fflush(stdout);
    //
    
    //Create 3 copies of the files
    int des1 = open(copy1, O_CREAT | O_RDWR | O_TRUNC);
    int des2 = open(copy2, O_CREAT | O_RDWR | O_TRUNC);
    int des3 = open(copy3, O_CREAT | O_RDWR | O_TRUNC);
    if(des1==-1 | des2==-1 | des3==-1){
        perror("opening copy folders");
    }

    //Copies content from the current file to all 3 files
    while(read(source, buffer, 1) != 0){
        int write1 = write(des1, buffer, 1);
        int write2 = write(des2, buffer, 1);
        int write3 = write(des3, buffer, 1);
        if(write1==-1 | write2==-1 | write3==-1){
            perror("writing to copy folders");
        }
    }

    //close files
    close(des1);
    close(des2);
    close(des3);
}

bool compareFiles(int file1, int file2){
    char buf1[SIZE];
    char buf2[SIZE];

    //compare a and b
    while(read(file1, buf1, SIZE)>0){ //while file1 != EOF
        if(read(file2, buf2, SIZE) < 1){
            return false;
        }

        int same = memcmp(buf1, buf2, SIZE);
        if(!same){
            return false;
        }
    }
    return true;
}

void get_request (int comm_fd, struct httpObject* request, char* buf, bool rflag){

    // printf("in GET\n");
    // fflush(stdout);
void get_request (int comm_fd, struct httpObject* request, char* buf){
    // printf("in GET\n");
    // fflush(stdout);
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
    memset(buf, 0, SIZE);

    // FAILS WITH MULTITHREADING ONLY FIX IF NEED TO DIFFERENTIATE BETWEEN 200 AND 201
    //check whether file already exists
    // int check;
    // if((check = access(request->filename, F_OK)) != -1){
    //     flag->exists = true;
    // }else{
    //     flag->exists = false;
    // }
    // syscallError(check, request);


    int file = open(request->filename, O_CREAT | O_RDWR | O_TRUNC);
    syscallError(file, request);

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
        //REASON FOR KEEPING IS IN README
        /*bytes_recv = read(comm_fd, buf, SIZE);
        syscallError(bytes_recv, request);
        int wfile = write(file, buf, bytes_recv);
        syscallError(wfile, request);

        while(bytes_recv == SIZE){
            bytes_recv = read(comm_fd, buf, SIZE);
            int wfile2 = write(file, buf, bytes_recv);
            syscallError(wfile2, request);
        }*/

        while((bytes_recv = read(comm_fd, buf, SIZE)) > 0){
            int wfile2 = write(file, buf, bytes_recv);
            syscallError(wfile2, request);
        }

        if(flag->exists == true) request->status_code = 200;
        else request->status_code = 201;
    }       

    construct_response(comm_fd, request);
    close(file); 
}

void set_first_parse (struct flags* flag, bool value){
    flag->first_parse = value;
}

void parse_request (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    // printf("%s\n", buf);
    // fflush(stdout);
    if(flag->first_parse){
        sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

        //check that httpversion is "HTTP/1.1"
        if(strcmp(request->httpversion, "HTTP/1.1") != 0){
            request->status_code = 400;
            construct_response(comm_fd, request);
            // return;
        }

        //check that filename is made of 10 ASCII characters
        else if(!valid_name(request, flag)){
            printf("before status code 400\n");
            fflush(stdout);
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
    // printf("in execFunc()\n");
    // fflush(stdout);

    memset(buf, 0, SIZE);

    int bytes_recv = recv(comm_fd, buf, SIZE, 0);   //recv and parse once 
    // printf("comm_fd in exefunct %d\n", comm_fd);
    // fflush(stdout);
    syscallError(bytes_recv, request);

    parse_request(comm_fd, request, buf, flag);

    printStruct(request);

    if(!valid_name(request, flag)) return;
    if(strcmp(request->httpversion, "HTTP/1.1") != 0) return;

    while(bytes_recv == SIZE){                      //if buf is completely filled
        bytes_recv = recv(comm_fd, buf, SIZE, 0);   //recv again
        syscallError(bytes_recv, request);

        parse_request(comm_fd, request, buf, flag); //parse for content length
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
int getPort (int argc, char *argv[]){
    int port;

    if((optind++) == (argc-1)){             //port number not specified
        port = 80;
        printf("(1) port = %d\n", port);
    }else{                                  //port number specified
        printf("argv[%d] = %s\n", optind, argv[optind]);
        port = atoi(argv[optind]);

        if (port < 1024){
            exit(EXIT_FAILURE);
            printf("(2) port = %d\n", port);
        }
    }

    return port;
}

void* workerThread(void* arg){
    // printf("*comm_fd = %d\n", comm_fd);
    // fflush(stdout);

    // printf("string = %s\n", ((struct synchronization*)arg)->string);

    //might need to be a pointer
    struct synchronization* spoint = (struct synchronization*)arg;
    pthread_cond_t *newReq = &spoint->newReq;
    pthread_mutex_t *queueLock = &spoint->queueLock;
    
    //handled by worker thread
    //worker handles mutex to determine to wait or run
    //create one array for each struct
    struct httpObject request;
    struct flags flag;
    char buf[SIZE];
    int comm_fd;
    while(true){

        printf("Start of workerthread while loop\n");
        fflush(stdout);

        pthread_mutex_lock(queueLock);
        //thread sleeps until an fd is pushed into queue
        while(commQ.empty()){
            pthread_cond_wait(newReq, queueLock);
        }
        
        comm_fd = commQ.front(); //front of queue has oldest fd
        commQ.pop();
        pthread_mutex_unlock(queueLock);

        set_first_parse(&flag, true);
        executeFunctions(comm_fd, &request, buf, &flag);
        memset(buf, 0, SIZE);
        clearStruct(&request);
        // pthread_mutex_unlock(&lock);

        printf("End of Workerthread %d\n", comm_fd);
        fflush(stdout);

        close(comm_fd);
    }
}

void copyFiles(char* filename, int source){
    char buffer[SIZE];
    
    //Create the path
    char copy1[50] = "./copy1/";
    char copy2[50] = "./copy2/";
    char copy3[50] = "./copy3/";

    //Append file name to path
    strcat(copy1, filename);
    strcat(copy2, filename);
    strcat(copy3, filename);

    //
    printf("%s\n", copy1);
    printf("%s\n", copy2);
    printf("%s\n", copy3);
    fflush(stdout);
    //
    
    //Create 3 copies of the files
    int des1 = open(copy1, O_CREAT | O_RDWR | O_TRUNC);
    int des2 = open(copy2, O_CREAT | O_RDWR | O_TRUNC);
    int des3 = open(copy3, O_CREAT | O_RDWR | O_TRUNC);
    if(des1==-1 | des2==-1 | des3==-1){
        perror("opening copy folders");
    }

    //Copies content from the current file to all 3 files
    while(read(source, buffer, 1) != 0){
        int write1 = write(des1, buffer, 1);
        int write2 = write(des2, buffer, 1);
        int write3 = write(des3, buffer, 1);
        if(write1==-1 | write2==-1 | write3==-1){
            perror("writing to copy folders");
        }
    }

    //close files
    close(des1);
    close(des2);
    close(des3);
}

// void* sleepThread(void *sink){
//     struct synchronization *temp = sink;
//     pthread_cond_wait(&temp->cond, &temp->lock);
//     workerThread(comm_fd);
//     close(comm_fd);
// }

int main (int argc, char *argv[]){
    (void)argc; //get rid of unused argc warning
    int option, numworkers = 0;

//----------------for debugging------------------------------
    // printf("-------------------------\n");
    // printf("ALL ARGS:\n");
    // for(int i=0; i<argc; i++){
    //     printf("argv[%d]: %s\n", i, argv[i]);
    //     fflush(stdout);
    // }
    // printf("-------------------------\n\n");
//-----------------------------------------------------------

    //optind:          0            1           2        3  4    5
    //max argc = 6:   ./httpserver  localhost   8080    -N  5   -r
    //if argc > max args, we already know command is wrong
    if(argc > 6){
        printf("too many args!\n");
        exit(EXIT_FAILURE);
    }

    bool rflag = false;
    
    //parse command for -N and -r
    while((option = getopt(argc, argv, "N:r")) != -1){
        if(option == 'r'){
            // printf("has r flag\n");
            rflag = true;

        }else if(option == 'N'){     
            numworkers = atoi(optarg);
            if(numworkers == 0){ //https://piazza.com/class/kfqgk8ox2mi4a1?cid=267
                printf("error: can't have 0 worker threads\n");
                exit(EXIT_FAILURE);
            }   

        }else{
            printf("error: bad flag\n");
            exit(EXIT_FAILURE);
        }
    }

    //if -r is present, make three different copies of all files in the server
    if(rflag == true){
        DIR *d;
        struct dirent *dir;
        d = opendir(".");

        //Loop through all files in the server directory
        if(d){
            while((dir = readdir(d)) != NULL){ 
    //             // printf("%s\n", dir->d_name);
    //             // fflush(stdout);
    //             //check if filename is valid
    //             int source = open(dir->d_name, O_RDONLY); //open source to copy from
    //             if(source == -1){
    //                 perror("can't open source file");
    //             }

    //             //check if file is a directory
    //             struct stat path_stat;
    //             stat(dir->d_name, &path_stat);
    //             int isfile = S_ISREG(path_stat.st_mode);
    //             printf("isfile = %d\n", isfile);

    //             //file is a directory, go to next file
    //             if(isfile==0){
    //                 continue;
    //             }

    //             copyFiles(dir->d_name, source);
    //             close(source);
    //         }  
    //         closedir(d); 
    //     }
    // }
                
                // printf("%s\n", dir->d_name);
                // fflush(stdout);
                //check if filename is valid
                int source = open(dir->d_name, O_RDONLY); //open source to copy from
                if(source == -1){
                    perror("can't open source file");
                }

                //check if file is a directory
                struct stat path_stat;
                stat(dir->d_name, &path_stat);
                int isfile = S_ISREG(path_stat.st_mode);
                printf("isfile = %d\n", isfile);

                //file is a directory, go to next file
                if(isfile==0){
                    continue;
                }

                copyFiles(dir->d_name, source);
                close(source);
            }  
            closedir(d); 
        }
    }

                // printf("%s\n", dir->d_name);
                // fflush(stdout);
                //check if filename is valid
                int source = open(dir->d_name, O_RDONLY); //open source to copy from
                if(source == -1){
                    perror("can't open source file");
                }

                //check if file is a directory
                struct stat path_stat;
                stat(dir->d_name, &path_stat);
                int isfile = S_ISREG(path_stat.st_mode);
                printf("isfile = %d\n", isfile);

                //file is a directory, go to next file
                if(isfile==0){
                    continue;
                }

                copyFiles(dir->d_name, source);
                close(source);
            }  
            closedir(d); 
        }
    }
    //if -N was not present, default is 4
    if(numworkers == 0) numworkers = 4;
    printf("numworkers = %d\n", numworkers);

//----------------for debugging------------------------------
    // printf("\noptind = %d\nargc = %d\n\n", optind, argc);
    // printf("-------------------------\n");
    // printf("REMAINING ARGS TO PARSE:\n");
    // for(int i=optind; i<argc; i++){
    //     printf("argv[%d]: %s\n", i, argv[i]);
    //     fflush(stdout);
    // }
    // printf("-------------------------\n\n");
//-----------------------------------------------------------

    //get host address (e.g. localhost)
    char* hostaddr = argv[optind];
    printf("hostaddr = %s\n", hostaddr);

    //get port number
    int port = getPort(argc, argv);
    printf("port = %d\n", port);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
        
    server_addr.sin_family = AF_INET;   
    inet_aton(hostaddr, &server_addr.sin_addr); //change this; cant use argv[1] anymore
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
        // perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //bind server address to open socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, addrlen) < 0){
        // perror("bind");
        exit(EXIT_FAILURE);
    }

    //listen for incoming connections
    if (listen(server_socket, 500) < 0){
        // perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr client_addr;
    socklen_t client_addrlen;

    // Create numworkers threads from pthread_t []
    pthread_t tid[numworkers];
    struct synchronization sink;

    for(int i=0; i<numworkers; i++){
        int tcreateerror = pthread_create(&(tid[i]), NULL, workerThread, (void*)(&sink));
        if(tcreateerror != 0){
            printf("\nThread %d cannot be created: [%s]", i, strerror(tcreateerror));
            fflush(stdout);
        }
    }
    //at end of loop, we will have N worker threads that are sleeping
    
    //dispatch thread
    while(true){
        printf("waiting for a connection\n");
        fflush(stdout);
        //accept incoming connection
        int comm_fd = accept(server_socket, &client_addr, &client_addrlen); //static
        printf("comm_fd in main: %d\n", comm_fd);
        fflush(stdout);
        
        //lock queue and push comm_fd into queue
        pthread_mutex_lock(&sink.queueLock);
        commQ.push(comm_fd);
        //  printf("inside lock queue top is: %d\n", commQ.front());
        pthread_mutex_unlock(&sink.queueLock);

        //alert one thread in pool to handle connection here
        printf("\nAbout to signal %d\n", comm_fd);
        fflush(stdout);
        pthread_cond_signal(&sink.newReq);
        printf("\nAfter signal %d\n", comm_fd);
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}
