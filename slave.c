#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

char *files_tosolve[100];
int current_files = 0;

int solveFile(char *file, char *solved ) {
    int link[2];
    pid_t pid;
    char local_solved[4096];

    if (pipe(link)==-1) {
        perror("pipe");
        return -1;
    }

    if ((pid = fork()) == -1){
        perror("fork");
        return -1;
    }

    if(pid == 0) {
        dup2 (link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        char *args_slave[]={ "minisat" , "./files/bart10.shuffled.cnf" , NULL};
        execvp(args_slave[0],args_slave); 
        perror("exec");
        return -1;

    } else {
        close(link[1]);
        int nbytes = read(link[0], local_solved, sizeof(local_solved));
        strncpy(solved, local_solved, sizeof(local_solved));
        wait(NULL);
        return nbytes;
    }
    return -1;
}

int main(int argc, char *argv[])
{
	// argv[1] is the initial number of files the slave will receive, argv[2] is the slave's identifier
    int initial_files = atoi(argv[1]);
    int identifier = atoi(argv[2]);

    printf("hello from slave , %d files to receive. My identifier is %d \n", initial_files, identifier );

    char fifo_parent_path[32], fifo_slave_path[32];
    sprintf(fifo_parent_path, "/tmp/fifo-parent-%d", identifier);
    sprintf(fifo_slave_path, "/tmp/fifo-slave-%d", identifier);

    int fd  = open(fifo_parent_path, O_RDONLY);
    char buf[1024];

    read(fd, buf, sizeof(buf));

    close(fd);

    // open fifo for writing
    int send_fd = open(fifo_slave_path, O_WRONLY);

    char *file = strtok (buf,";");
    while (file!= NULL){
        files_tosolve[current_files] = file;
        current_files++;
        file = strtok (NULL, ";");
    }


    char solved[8192];

    // solve the initial files.
    for(; current_files > 0 ; current_files-- ){
        char this_file[4096];
        int nbytes = solveFile(files_tosolve[current_files], this_file);
        strcat(solved, this_file);
        strcat(solved, "\n");
    }

    printf("%s\n", solved);

    while(1) {
        // a partir de acá el slave recibe los archivos de a uno
        char new_file[2048];
        write(send_fd, solved, sizeof(solved));
        read(fd, new_file, sizeof(new_file));
        // si se lee un mensaje especial hay que terminar el slave.
        solveFile(new_file, solved);
    }

    close(send_fd);

    return 0;
}
