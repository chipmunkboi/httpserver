#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
    char buff[100]; // declare a 100 byte buffer to be used per file

    if(argc==1){ //
        userinput();
    }

    for(int i=1; i<argc; i++){ // loops through argc files and prints each file
        memset(buff, 0, sizeof(buff)); //clears buffer before reading new file
        // check if file is valid
        

        int fone = open(argv[i], O_RDONLY);
        int rone = read(fone, buff, sizeof(buff));
        close(fone);

        fprintf(stdout, "%s", buff);

    }
}

int userinput(){
    char buf[1];
    
    while((read(STDIN_FILENO, buf, 1)) != 0){ //0 = EOF
        int write_check = write(1, buf, 1);
        if(write_check==-1){
            warn("%s", argv[0]);
            error_flag = 1;
        }
    }
    return error_flag;
}