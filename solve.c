 #include <sys/types.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
#define NUM_SLAVES 5
#define MAX_FILES 200

int main(int argc, char *argv[])
{
    // accept dir/* , dir/file.cnf , file.cnf as parameters

    if ( argc < 2 ) {
        printf("usage: solve [FILES] \n");
        printf("files must be .cnf\n");
        return;
    }

    char *to_solve[MAX_FILES];
    for( int i = 1 ; i < argc ; i++ ) {
        char *to_solve[i-1] = argv[i];
    }


	// create NUM_SLAVES slaves
	pid_t pid;
	pid_t slaves_pids[5];
	pid = fork();

	for(int i=0;pid != 0 && i< NUM_SLAVES - 1 ;i++)
    { 
    	slaves_pids[i] = pid;
        pid = fork();
    } 

    if ( pid != 0 ) {
    	// padre
        slaves_pids[NUM_SLAVES-1] = pid; 
    }

	// distribute initial files to slaves

}