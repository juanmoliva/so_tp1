#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#define NUM_SLAVES 5
#define MAX_FILES 200
#define INITIAL_FILES_FOR_SLAVE "2"

int main(int argc, char *argv[])
{
    // accept dir/* , dir/file.cnf , file.cnf as parameters

    /*if ( argc < 2 ) {
        printf("usage: solve [FILES] \n");
        printf("files must be .cnf\n");
        return 1;
    }*/

    /*
    char *to_solve[MAX_FILES];
    for( int i = 1 ; i < argc ; i++ ) {
        char *to_solve[i-1] = argv[i];
    }
    */

	// create NUM_SLAVES slaves
	pid_t pid;
	pid_t slaves_pids[5];
	pid = fork();

	for(int i=0;pid != 0 && i< NUM_SLAVES - 1 ;i++)
    { 
    	slaves_pids[i] = pid;
        pid = fork();
    } 

    int fd_fifos[NUM_SLAVES];

    if ( pid != 0 ) {
    	// padre
        slaves_pids[NUM_SLAVES-1] = pid; 

        // make fifos
        for(int i = 0; i < NUM_SLAVES ; i++) {
            char* buffs_1 = "messsage_1";
            char fifo_path[32];
            sprintf(fifo_path, "/tmp/fifo-%d", slaves_pids[i]);
            mkfifo(fifo_path, 0666);
            fd_fifos[i] = open(fifo_path, O_WRONLY);
            write(fd_fifos[i], buffs_1, sizeof(buffs_1));
            close(fd_fifos[i]);
        }

    }

    if ( pid == 0 ){
        // a slave
        char *args_slave[]={"./slave",INITIAL_FILES_FOR_SLAVE, NULL}; 
        printf("exec executed.\n");
        execvp(args_slave[0],args_slave); 
        printf("problem with exec\n");
        return 2;
    }

    char* buffs_1 = "messsage 1";
    char* buffs_2 = "messsage 2";

	// distribute initial files to slaves



    return 0;

}