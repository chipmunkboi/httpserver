#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
    char buff[100]; // declare a 100 byte buffer to be used per file

    for(int i=1; i<argc; i++){ // loops through argc files and prints each file
        memset(buff, 0, sizeof(buff)); //clears buffer before reading new file
        
        int fone = open(argv[i], O_RDONLY);
        int rone = read(fone, buff, sizeof(buff));
        close(fone);

        fprintf(stdout, "%s", buff);

    }
}