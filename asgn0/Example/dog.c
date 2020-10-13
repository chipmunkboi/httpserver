//Michelle Yeung
//myyeung
//asgn0
//dog.c

#include <string.h>	//strcmp()
#include <fcntl.h>	//open(2)
#include <unistd.h>	//read(2) and write(2)
#include <errno.h>	//warn(3)
#include <err.h>	//warn(3)
#include <stdlib.h> //exit(3)

int main(int argc, char *argv[]){
	int no_args = argc; //argc holds number of arguments
	int error_flag = 0;

	if(no_args==1){	//"cat"
		char buf[1];

		while((read(STDIN_FILENO, buf, 1)) != 0){ //0 = EOF
			int write_check = write(1, buf, 1);
			if(write_check==-1){
				warn("%s", argv[0]);
				error_flag = 1;
			}
		}

	}else{ 
		for(int i=1; i<no_args; i++){ //process args in order		
			if(strcmp(argv[i], "-")==0){ //"cat -" 
				char buf[1];

				while((read(STDIN_FILENO, buf, 1)) != 0){
					int write_check = write(1, buf, 1);
					if(write_check==-1){
						warn("%s", argv[i]);
						error_flag = 1;
					}
				}
			
			}else{ //"cat <filename>"
				char buf[1];
				int fd = open(argv[i], O_RDONLY);

				if(fd==-1){
					warn("%s", argv[i]);
					error_flag = 1;
				}

				while((read(fd, buf, 1)) > 0){
					int write_check = write(1, buf, 1);
					if(write_check==-1){
						warn("%s", argv[i]);
						error_flag = 1;
					}
				}			
			}
		}
	}
	
	return error_flag;
}
