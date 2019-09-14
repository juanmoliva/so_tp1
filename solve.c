#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#define NUM_SLAVES 3
#define MAX_FILES 200
#define INITIAL_FILES_FOR_SLAVE "2"

int main(int argc, char *argv[])
{
    // falta aceptar los archivos por linea de comandos.
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

    char *files[12] = {"./files/bart10.shuffled.cnf","./files/bart11.shuffled.cnf","./files/bart12.shuffled.cnf","./files/bart13.shuffled.cnf","./files/bart14.shuffled.cnf","./files/bart15.shuffled.cnf","./files/bart16.shuffled.cnf","./files/bart17.shuffled.cnf","./files/bart18.shuffled.cnf","./files/bart19.shuffled.cnf","./files/bart20.shuffled.cnf","./files/bart21.shuffled.cnf"};

    // make fifos
    for(int i = 0; i < NUM_SLAVES ; i++) {
        char fifo_path_parent[32], fifo_path_slave[32];
        sprintf(fifo_path_parent, "/tmp/fifo-parent-%d", i);
        mkfifo(fifo_path_parent, 0666);
        sprintf(fifo_path_slave, "/tmp/fifo-slave-%d", i);
        mkfifo(fifo_path_slave, 0666);
    }


	// create NUM_SLAVES slaves
	pid_t pid;
	pid_t slaves_pids[NUM_SLAVES];
	pid = fork();
    int j = 0;

	for(;pid != 0 && j< NUM_SLAVES - 1 ;j++)
    { 
    	slaves_pids[j] = pid;
        pid = fork();
    } 

    int fd_fifos[NUM_SLAVES];

    if ( pid != 0 ) {
    	// parent
        slaves_pids[NUM_SLAVES-1] = pid; 

        // write to FIFO's
        // distribute initial files to slaves
        for(int i = 0; i < NUM_SLAVES ; i++) {
            char buff[32];
            sprintf(buff, "messsage-%d", i);
            char fifo_path[32];
            sprintf(fifo_path, "/tmp/fifo-parent-%d", i);
            fd_fifos[i] = open(fifo_path, O_WRONLY);
            write(fd_fifos[i], buff, sizeof(buff));
            close(fd_fifos[i]);
        }
    } else {        // a slave
        char j_char[32];
        sprintf(j_char, "%d", j);
        char *args_slave[]={ "./slave" , INITIAL_FILES_FOR_SLAVE , j_char , NULL}; 
        printf("exec executed.\n");
        execvp(args_slave[0],args_slave); 
        printf("problem with exec\n");
        return 2;
    }


    // open slave fifos for reading.
	printf("Opening slave fifos\n");
    int fd_slaves[NUM_SLAVES];

    for(int i= 0; i<NUM_SLAVES; i++) {
        char fifo_path[32];
        sprintf(fifo_path, "/tmp/fifo-slave-%d", i);
        fd_slaves[i] = open(fifo_path, O_RDONLY);
    }

    // use select() to read from multiple fd's without getting blocked.
    int nfds = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    for(int i = 0 ; i < NUM_SLAVES ; i++) {
        if(fd_slaves[i] > nfds) nfds = fd_slaves[i];
        FD_SET(fd_slaves[i], &rfds);
    }
    nfds++;

    // wait up to 5 seconds
    tv.tv_sec = 5;
    tv.tv_usec = 0;


    retval = select(1, &rfds, NULL, NULL, &tv);

    if (retval == -1)
        perror("select()");
    else if (retval)
        printf("Data is available now.\n");
        /* FD_ISSET(0, &rfds) will be true. */
    else
        printf("No data within five seconds.\n");
    




    return 0;

}