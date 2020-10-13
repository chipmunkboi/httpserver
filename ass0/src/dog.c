#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>

// reads from stdin then writes to stdout
void userinput(char argzero[]){
    char buf[1];
    while((read(STDIN_FILENO, buf, 1)) != 0){ 
        int write_check = write(STDOUT_FILENO, buf, 1);
        if (write_check == -1){
            warn("%s", argzero);
        }
    }
}

int main(int argc, char *argv[]){
    // declare a 100 byte buffer to be used per file
    char buff[100]; 

    // if only arg is "dog"
    if (argc == 1){ 
        userinput(argv[0]);
    }

    // loops through argc files and prints each file
    for (int i=1; i<argc; i++){ 

        // clears buffer before reading new file
        memset(buff, 0, sizeof(buff)); 
        // flushes buffer to prevent out of order execution
        //fflush(stdout);
        // check if "-" is present for user input
        //printf("%s", argv[i]);
        if (strcmp(argv[i], "-") == 0){
            userinput(argv[i]);
            continue;
        }

        int fone = open(argv[i], O_RDONLY);
        
        // check if file is valid
        if (fone == -1){
            // file is not valid else continue
            warn("%s", argv[i]);
            close(fone);
            continue;
        }
        //int rone = read(fone, buff, sizeof(buff));
        while(read(fone, buff, 1) != 0){
            int write_check = write(STDOUT_FILENO, buff, 1);
            if (write_check == -1){
                warn("%s", argv[i]);
            }
        }
        close(fone);

        //fprintf(stdout, "%s", buff);

    }
}

