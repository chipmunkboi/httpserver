#include <stdio.h>
#include <unistd.h>

void main (int argc, char *argv[]){
    int opt;
    while((opt = getopt(argc, argv, "N:r")) != -1){
        if(opt == 'r'){
            printf("has r flag\n");
        }else if(opt == 'N'){
            printf("has %s arg\n", optarg);
        }else{
            printf("error: bad flag\n");
        }
    }

    printf("optind = %d, argc = %d\n", optind, argc);

    if(optind != argc && optind != (argc-1)){
        printf("too many args\n");
    }else{
        printf("extra arg: %s\n", argv[optind]);
    }

}