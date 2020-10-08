#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    char buff[100];
    char buff2[100];

    int fone = open("abc.txt", O_RDONLY);
    int ftwo = open("def.txt", O_RDONLY);

    int rone = read(fone, buff, sizeof(buff));
    int rtwo = read(ftwo, buff2, sizeof(buff2));

    //printf("%d\n", rone);

    close(fone);
    close(ftwo);

    fprintf(stdout, "%s", buff);
    fprintf(stdout, "%s", buff2);
}