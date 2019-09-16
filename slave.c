#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>


void set_fifo_path(int identifier);

/////////////////////////////// Puntero a los archivos a resolver Y un contador  ////////////////////////////////////////
char *files_tosolve[100];
int current_files = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char fifo_path[32];

FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Resuelve el archivo - Le pasamos un puntero al archivo y un puntero para que deje la rta  //////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int solveFile(char *file, char *solved ) {
    char pid[50], file_str[100];
    sprintf(pid, "pid of the slave is %d\n", getpid());
    sprintf(file_str, "the file '%s' was solved\n", file);
    char command[200];
    sprintf(command, "minisat %s | grep -e  'Number of variables' -e 'Number of clauses' -e 'CPU time' -e 'SAT' ", file);
    FILE* fp;
    char line[128];
    unsigned int size=0;
    printf("about to execute in %d : '%s'\n", getpid() ,command );
    fp=popen(command,"r");

    strcat(solved, file_str);
    strcat(solved,pid);
    
    while (fgets(line,sizeof(line),fp))
        {
        size+=strlen(line);
        strcat(solved,line);
        }

    fclose(fp);
    
    return 0;
}




int main(int argc, char *argv[])
{
    // argv[1] is the initial number of files the slave will receive
    // argv[2] is the slave's identifier
    int initial_files = atoi(argv[1]);
    int identifier = atoi(argv[2]);

    set_fifo_path(identifier);

    // printf("hello from slave , %d files to receive. My identifier is %d \n", initial_files, identifier );

    /*// Asignamos respectivo id a los nombres del pipe
    char fifo_parent_path[32], fifo_slave_path[32];
    sprintf(fifo_parent_path, "/tmp/fifo-parent-%d", identifier);
    sprintf(fifo_slave_path, "/tmp/fifo-slave-%d", identifier);*/
     
    /*// Abrimos pipe de lectura del padre y guardamos el int q devuelve
    int fd  = open(fifo_parent_path, O_RDONLY);*/

    // abrimos fifo para lectura
    int fd = open(fifo_path, O_RDONLY);
    if ( fd == -1 ){
        perror( "open fifo in slave");
        return 1;
    }
    
    // Creamos un buffer
    char buf[1024];
     
    // Guarda en buf lo que le escribio el padre
    read(fd, buf, sizeof(buf));


    // printf("Im slave %d, I received '%s'", identifier, buf);

    close(fd);

    /*// open fifo for writing
    int send_fd = open(fifo_slave_path, O_WRONLY);*/

    fd = open(fifo_path, O_WRONLY);

    char *file = strtok (buf,";");
    while (file!= NULL){
        files_tosolve[current_files] = file;
        // printf("slave, %d, file %d, %s\n", identifier, current_files, files_tosolve[current_files] );
        
        current_files++;
        file = strtok (NULL, ";");
    }


    char *solved = (char *)malloc(8096*sizeof(char));

    // solve the initial files.

    while(current_files > 0) {
        current_files--;
        char *this_file = (char *)malloc(4048*sizeof(char));
        printf("slave %d, solving %s \n", identifier, files_tosolve[current_files]);
        solveFile(files_tosolve[current_files], this_file);
        strcat(solved, this_file);
        strcat(solved, "\n");
    }

    int res = write(fd, solved, strlen(solved));
    if ( res == -1 ){
        perror("write in slave");
        return 1;
    }

    close(fd);

    char file_loop[512], solved_loop[512];
    int end = 0;
    while(!end) {
        // a partir de acá el slave recibe los archivos de a uno
        fd = open(fifo_path, O_RDONLY);
        if (fd == -1) {
            perror("open in slave loop");
            return -1;
        }

        int read_res = read(fd, file_loop, 512);

        if (read_res == -1) {
            perror("read on slave in loop");
            return 1;
        } 

        if ( strcmp(file_loop,"END") == 0 ) {
            printf("in slave %d we reached termination\n", identifier);
            close(fd);
            break;
        }
        close(fd);

        // solve file read on fifo and send in on solved.
        solveFile(file_loop, solved_loop);

        fd = open(fifo_path, O_WRONLY);
        if (fd == -1) {
            perror("open in slave loop");
            return 1;
        }

        int write_res = write(fd, solved_loop, strlen(solved_loop));
        if(write_res == -1) {
            perror("write on slave in loop");
            return 1;
        }

        close(fd);

        /*// write(send_fd, solved, 8096*sizeof(char));
        write(send_fd, test_buf, 50);
        int nbytes = read(fd, new_file, 2048*sizeof(char));
        if (nbytes == -1) {
            perror("read file on slave");
            return 1;
        } else if ( nbytes != 0 ){
            solveFile(new_file, solved);
        }*/
        /*
        else {
            end = 1;
        }*/
    }


    close(fd);

    return 0;
}

void set_fifo_path( int identifier ) {
    sprintf( fifo_path, "/tmp/fifos-%d", identifier );
    return;
}
