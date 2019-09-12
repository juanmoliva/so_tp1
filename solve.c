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



    // make fifos
    for(int i = 0; i < NUM_SLAVES ; i++) {
        char fifo_path[32];
        sprintf(fifo_path, "/tmp/fifo-%d", i);
        mkfifo(fifo_path, 0666);
    }


	// create NUM_SLAVES slaves
	pid_t pid;
	pid_t slaves_pids[5];
	pid = fork();
    int j = 0;

	for(;pid != 0 && j< NUM_SLAVES - 1 ;j++)
    { 
    	slaves_pids[j] = pid;
        pid = fork();
    } 

    int fd_fifos[NUM_SLAVES];

    if ( pid != 0 ) {
    	// padre
        slaves_pids[NUM_SLAVES-1] = pid; 

        // write to FIFO's
        for(int i = 0; i < NUM_SLAVES ; i++) {
            char buff[32];
            sprintf(buff, "messsage-%d", i);
            char fifo_path[32];
            sprintf(fifo_path, "/tmp/fifo-%d", i);
            fd_fifos[i] = open(fifo_path, O_WRONLY);
            write(fd_fifos[i], buff, sizeof(buff));
            close(fd_fifos[i]);
        }
    } else {
        // a slave
        char j_char[32];
        sprintf(j_char, "%d", j);
        char *args_slave[]={ "./slave" , INITIAL_FILES_FOR_SLAVE , j_char , NULL}; 
        printf("exec executed.\n");
        execvp(args_slave[0],args_slave); 
        printf("problem with exec\n");
        return 2;
    }

	// distribute initial files to slaves



    return 0;

}