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
#include <ctype.h>      //isdigit
#include <pthread.h>    //threads
#include <dirent.h>     //for parsing directory

#include <queue>         //queue
#include <unordered_map> // C++ library for hashmap 

#define SIZE 4096       //4 KB buffer restriction

using namespace std;
// create queue for requests 
queue <int> commQ;

//create global lock for new files
pthread_mutex_t newFileLock = PTHREAD_MUTEX_INITIALIZER;

// create hashmap for file locks
unordered_map<string, pthread_mutex_t> fileLock;

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
};

struct requestLock{
    pthread_cond_t *newReq;     //cond var for threads getting new request
    pthread_mutex_t *queueLock; //mutex for queue
    bool rflag;
};

//used to check that all parts of struct are correct 
void printStruct(struct httpObject* request){
    printf("type: %s=\n", request->type);
    printf("ver: %s=\n", request->httpversion);
    printf("file: %s=\n", request->filename);
    printf("content length: %ld=\n", request->content_length);
    fflush(stdout);
}

void clearStruct(struct httpObject* request){
    memset(request->type, 0, 4);
    memset(request->httpversion, 0, 9);
    memset(request->filename, 0, 50);
    memset(request->body, 0, SIZE);
    request->collector = NULL;
    request->status_code = 0;
    //why -1 not 0 !!CHECK 
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

void syscallError(int comm_fd, int file, struct httpObject* request){
    if(file == -1){
        printf("syscallError: %d = %s\n", errno, strerror(errno));
        request->status_code = 500;
        construct_response(comm_fd, request);
    }
}

//returns TRUE if name is valid and FALSE if name is invalid
bool valid_name (struct flags* flag, char* tempname){
    //remove "/" from front of file name
    if(tempname[0] == '/'){
        memmove(tempname, tempname+1, strlen(tempname));
    }

    // check that name is 10 char long
    if(strlen(tempname) != 10){
        flag->good_name = false;
        return false;
    }

    // check that all chars are ASCII chars
    for(int i=0; i<10; i++){
        if(!isalnum(tempname[i])){
            flag->good_name = false;
            return false;
        }
    }

    flag->good_name = true;
    return true;
}

void pathName(struct httpObject* request, bool rflag, char* path){
    if(rflag){
        strcpy(path, "./copy1/");
        // printf("path in pathName BEFORE COPY: %s\n", path);
        // fflush(stdout); 
        strcat(path, request->filename);

    }
    else strcpy(path, request->filename);
}

void copyFiles(char* filename, int source, bool isMain = false){
    char buffer[SIZE];

    //create path "./copy#/filename" to navigate from httpserver directory
    char copy1[20] = "./copy1/";
    char copy2[20] = "./copy2/";
    char copy3[20] = "./copy3/";
    strcat(copy1, filename);
    strcat(copy2, filename);
    strcat(copy3, filename);
    
    //Create 3 copies of the files
    int des1;
    if(isMain){
        des1 = open(copy1, O_CREAT | O_RDWR | O_TRUNC);
        // if (des1==-1){
        //     if(errno == ENOENT){
        //     request->status_code = 404;
        //     }else if(errno == EACCES){
        //         request->status_code = 403;
        //     }else{
        //         request->status_code = 400;
        //     }
        // }
        perror("opening copy1 folder"); //
    }

    int des2 = open(copy2, O_CREAT | O_RDWR | O_TRUNC);
    int des3 = open(copy3, O_CREAT | O_RDWR | O_TRUNC);
    if((des2==-1) || (des3==-1)){
        perror("opening copy2/copy3 folder");
        // if(errno == ENOENT){
        //     request->status_code = 404;
        // }else if(errno == EACCES){
        //     request->status_code = 403;
        // }else{
        //     request->status_code = 400;
        // }
    }

    //Copies content from the current file to all files
    while(read(source, buffer, 1) != 0){
        if(isMain){
            int write1 = write(des1, buffer, 1);
            if(write1==-1) perror("writing to copy1 folders");
        }
        int write2 = write(des2, buffer, 1);
        int write3 = write(des3, buffer, 1);
        if((write2==-1) | (write3==-1)){
            perror("writing to copy 2/3 folders");
        }
    }

    //close files
    if(isMain) close(des1);
    close(des2);
    close(des3);
}

//test to see if work with path/filename
bool compareFiles(const char* fileOne, const char* fileTwo){
    char buf1[SIZE];
    char buf2[SIZE];
    memset(buf1, 0, SIZE);
    memset(buf2, 0, SIZE);
    int file1 = open(fileOne, O_RDONLY);
    int file2 = open(fileTwo, O_RDONLY);
    //compare a and b
    while(read(file1, buf1, SIZE) > 0){ //while file1 != EOF
        //If file2 is empty
        if(read(file2, buf2, SIZE) == 0){
            close(file1);
            close(file2);
            return false;
        }
        int same = memcmp(buf1, buf2, SIZE);
        //If contents of buffer is different
        if(same != 0){
            close(file1);
            close(file2);
            return false;
        }
        //clear buffer before next read
        memset(buf1, 0, SIZE);
        memset(buf2, 0, SIZE);
    }
    // If file2 still have things to read
    if(read(file2, buf2, SIZE) > 0){
        close(file1);
        close(file2);
        return false;
    }
    //otherwise true
    close(file1);
    close(file2);
    return true;
}

void parse_request (int comm_fd, struct httpObject* request, char* buf, struct flags* flag){
    printf("[%d] in parse req\n", comm_fd);
    if(flag->first_parse){
        sscanf(buf, "%s %s %s", request->type, request->filename, request->httpversion);

        //check that httpversion is "HTTP/1.1"
        if(strcmp(request->httpversion, "HTTP/1.1") != 0){
            request->status_code = 400;
            construct_response(comm_fd, request);
        }

        //check that filename is made of 10 ASCII characters
        else if(!valid_name(flag, request->filename)){
            printf("before status code 400\n");
            fflush(stdout);
            request->status_code = 400;
            construct_response(comm_fd, request);
        }

        flag->first_parse = false;
    }
    //In executefunctions put line by line into buf by scanning for \r\n instead of scanning it in buf
    char* token = strtok(buf, "\r\n");
    while(token){
        if(strncmp(token, "Content-Length:", 15) == 0){
            printf("[%d] found content length\n", comm_fd);
            sscanf(token, "%*s %ld", &request->content_length);
        }else if(strncmp(token, "\r\n", 2)==0){
            printf("[%d] should break here\n", comm_fd); //doesn't even go into here
            fflush(stdout);
            break;
        }
        token = strtok(NULL, "\r\n");
    }
}

void get_request (int comm_fd, struct httpObject* request, char* buf, bool rflag){
    memset(buf, 0, SIZE);

    //check
    char path[50];
    pathName(request, rflag, path);
    int file = open(path, O_RDONLY);
    int sendfile;
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

        //get content length
        struct stat size;
        fstat(file, &size);
        request->content_length = size.st_size;

        if(rflag){
            //Create the path
            char copy2[50] = "./copy2/";
            char copy3[50] = "./copy3/";
            //path is ./copy1/filename
            strcat(copy2, request->filename);
            strcat(copy3, request->filename);
            //because compareFiles will open again
            close(file);
            if(compareFiles(path, copy2)){
                sendfile = open(path, O_RDONLY);
            }
            else if(compareFiles(path, copy3)){
                 sendfile = open(path, O_RDONLY);
            }
            else if(compareFiles(copy2, copy3)){
                 sendfile = open(copy2, O_RDONLY);
    
            }
            // int file2 = open(copy2, O_RDONLY);
            // int file3 = open(copy3, O_RDONLY);
            
            // if(file!=-1 && file2!=-1 && compareFiles(file, file2)){
            //     close(file);
            //     close(file2);
            //     close(file3);
            //     sendfile = open(copy2, O_RDONLY); 

            // }else if(file != -1 && file3 != -1 && compareFiles(file, file3)){
            //     close(file);
            //     close(file2);
            //     close(file3);
            //     sendfile = open(path, O_RDONLY); 

            // }else if(file2 != -1 && file3 != -1 && compareFiles(file2, file3)){
            //     close(file);
            //     close(file2);
            //     close(file3);
            //     sendfile = open(copy2, O_RDONLY);
           
            else {
            //If files fail the checks
            printf("in get_req else, return 500\n");
            fflush(stdout);
            request->status_code = 500;
            }

        }else{
            sendfile = file;
        }

        construct_response(comm_fd, request);

        int check;
        while((check = read(sendfile, buf, 1)) != 0){
            send(comm_fd, buf, 1, 0); 
        } 
    }
    close(sendfile);
}

void put_request(int comm_fd, struct httpObject* request, char* buf, struct flags* flag, bool rflag){
    int wfile;      //to check if write() is successful/how many bytes got written
    int check;
    char path[50];  //to store file path
    pathName(request, rflag, path);

    //check whether or not file already exists
    if((check = access(path, F_OK)) != -1){
        flag->exists = true;
    }else{
        flag->exists = false;
    }

    int file;
    // int testWHYNOTWORKS;
    if(request->status_code != 400){                                            //if bad request, don't create the file
        file = open(path, O_CREAT | O_RDWR | O_TRUNC);
    }
    if(file == -1){perror("open");} //CHECK: do we need to implement status codes here??
    syscallError(comm_fd, file, request);
    
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
                syscallError(comm_fd, wfile, request);
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
            syscallError(comm_fd, wfile, request);
        }

        request->content_length = request->content_length - strlen(request->body);

        if((request->collector != NULL) && (strlen(request->collector) != 0)){  //ensure that we write everything from buf
            if(request->status_code != 400){
                wfile = write(file, request->collector, strlen(request->collector));
                syscallError(comm_fd, wfile, request);
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
                    syscallError(comm_fd, wfile, request);
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

    //if redundancy
    if(rflag && request->status_code!=400){ //if bad request, don't need to copy
        close(file);
        file = open(path, O_RDONLY);
        copyFiles(request->filename, file);
    }

    construct_response(comm_fd, request); //construct respond & send

    memset(path, 0, 50); //probably not needed
    close(file); 
}

void executeFunctions (int comm_fd, struct httpObject* request, char* buf, struct flags* flag, bool rflag){
    int bytes_recv = recv(comm_fd, buf, SIZE, 0); //first recv
    syscallError(comm_fd, bytes_recv, request);

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
    else if(!valid_name(flag, request->filename)){
        request->status_code = 400;
        //If get req is 400 no need to execute to completion
        if(strcmp(request->type, "GET") == 0){
            construct_response(comm_fd, request); //commented to help fix invalid name problem
            return;
        }
    }
    char temp[SIZE];
    strncpy(temp, buf, SIZE); //protects buf from being 
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
        printf("IN EXP\n");
        if(copyBytes == (strlen(request->body)+1)){     //if body is missing one byte, it's probably due to an \n being cut
        strcat(request->body, "\n");
        
        }else{                                          //if # of body bytes != # body bytes we have...
            request->collector = &buf[strlen(request->body)+token_counter]; //...retrieve the rest from the original buffer buf
        }
    }

    pthread_mutex_lock(&newFileLock); //global lock
    char path[50];
    pathName(request, rflag, path);
    if(fileLock.find(path) == fileLock.end()){
        pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
        fileLock[path] = fileMutex;
    }
    pthread_mutex_unlock(&newFileLock); //global unlock

    pthread_mutex_lock(&fileLock.at(path));
    if(strcmp(request->type, "PUT") == 0){ //body is present, read message body
        put_request(comm_fd, request, buf, flag, rflag);
    }else if(strcmp(request->type, "GET") == 0){
        get_request(comm_fd, request, buf, rflag);
    }
    else{
        request->status_code = 500;
        construct_response(comm_fd, request);
    }
    pthread_mutex_unlock(&fileLock.at(path));

}

void* workerThread(void* arg){
    //created pointer to the locks/conditional variables
    pthread_cond_t *newReq = ((struct requestLock*)arg)->newReq;
    pthread_mutex_t *queueLock = ((struct requestLock*)arg)->queueLock;

    bool rflag = (((struct requestLock*)arg)->rflag);

    struct httpObject request;
    struct flags flag;
    char buf[SIZE];
    int comm_fd;
    while(true){
        //thread sleeps until an fd is pushed into queue
        if(commQ.empty()){
            pthread_cond_wait(newReq, queueLock);
        }
        else pthread_mutex_lock(queueLock);
        
        comm_fd = commQ.front(); //front of queue has oldest fd
        commQ.pop();
        pthread_mutex_unlock(queueLock);

        flag.first_parse = true;
        executeFunctions(comm_fd, &request, buf, &flag, rflag);
        memset(buf, 0, SIZE);
        clearStruct(&request);

        close(comm_fd);
    }
}

//port is set to user-specified number or 80 by default
int getPort (int argc, char *argv[]){
    int port;

    if((optind++) == (argc-1)){     //port number not specified
        port = 80;
    }else{                          //port number specified
        port = atoi(argv[optind]);

        if (port < 1024){           //invalid port number
            exit(EXIT_FAILURE);
        }
    }

    return port;
}

int main (int argc, char *argv[]){
    (void)argc; //get rid of unused argc warning
    int option, numworkers = 0;

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

    // // UNCOMMENT BEFORE SUBMIT
    //if -r is present, make three different copies of all files in the server
    // if(rflag == true){
    //     DIR *d;
    //     struct dirent *dir;
    //     d = opendir(".");

    //     //Loop through all files in the server directory
    //     if(d){
    //         while((dir = readdir(d)) != NULL){ 
                
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

    //             copyFiles(dir->d_name, source, true);
    //             close(source);
    //         }  
    //         closedir(d); 
    //     }
    // }

    //if -N was not present, default is 4
    if(numworkers == 0) numworkers = 4;

    //get host address (e.g. localhost)
    char* hostaddr = argv[optind];

    //get port number
    int port = getPort(argc, argv);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
        
    server_addr.sin_family = AF_INET;   
    inet_aton(hostaddr, &server_addr.sin_addr); //change this; cant use argv[1] anymore
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

    //create queue lock and new request signal
    pthread_cond_t request = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;

    struct requestLock sink;

    sink.newReq = &request;
    sink.queueLock = &queueLock;
    sink.rflag = rflag;

    // Create numworkers threads from pthread_t []
    pthread_t *tid = new pthread_t[numworkers]; //NEW: check this - might need to delete[] tid somewhere
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
        
        //lock queue and push comm_fd into queue
        pthread_mutex_lock(&queueLock);
        commQ.push(comm_fd);
        pthread_mutex_unlock(&queueLock);

        //alert one thread in pool to handle connection here
        pthread_cond_signal(&request);
    }

    return EXIT_SUCCESS;
}
