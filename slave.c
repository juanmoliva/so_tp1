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

char fifo_write_path[32];
char fifo_read_path[32];

FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// SolveFile. Resuelve el archivo - Le pasamos un puntero al archivo y un puntero para que deje la rta  //////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int solveFile(char *file, char *solved ) {
    char pid[50], file_str[100];
    sprintf(pid, "pid of the slave is %d\n", getpid());
    sprintf(file_str, "the file '%s' was solved\n", file);
    char command[200];
    sprintf(command, "minisat %s | grep -e  'Number of variables' -e 'Number of clauses' -e 'CPU time' -e 'SAT' &>/dev/null", file);
    FILE* fp;
    char line[128];
    unsigned int size=0;
    fp=popen(command,"r");

    strcat(solved, file_str);
    strcat(solved,pid);
    while (fgets(line,sizeof(line),fp)) {
    size+=strlen(line);
    strcat(solved,line);
    }

    fclose(fp);
    
    return 0;
}

int main(int argc, char *argv[])
{
    // argv[1] is the slave's identifier
    int identifier = atoi(argv[1]);

    set_fifo_path(identifier);

    // abrimos fifo para lectura
    int fd_read = open(fifo_read_path, O_RDONLY);
    if ( fd_read == -1 ){
        perror( "open fifo in slave");
        return 1;
    }
    
    // Creamos un buffer
    char buf[1024];
     
    // Guarda en buf lo que le escribio el padre
    read(fd_read, buf, sizeof(buf));

    int fd_write = open(fifo_write_path, O_WRONLY);

    // los archivos inciales se reciben en un unico buffer separados por puntos y coma ( ; )
    // ahora se separan y se van guardando en el array files_tosolve[].
    char *file = strtok (buf,";");
    while (file!= NULL){
        files_tosolve[current_files] = file;
        current_files++;
        file = strtok (NULL, ";");
    }

    char *solved = (char *)calloc( 4,1000*sizeof(char));

    // resolvemos los archivos iniciales.

    while(current_files > 0) {
        current_files--;
        char *this_file = (char *)malloc(2048*sizeof(char));
        memset(this_file,0,2048*sizeof(char));
        solveFile(files_tosolve[current_files], this_file);
        strcat(solved, this_file);
        strcat(solved, "\n");
        free(this_file);
    }

    // mandamos al proceso solve los archivos iniciales resueltos.
    int res_write = write(fd_write, solved, strlen(solved));
    if ( res_write == -1 ){
        perror("write in slave");
        return 1;
    }
    
    free(solved);

    while(1) {
        char *file_loop = (char*) malloc(1024*sizeof(char));
        memset(file_loop,0,1024*sizeof(char));
        char *solved_loop = (char*) malloc(1024*sizeof(char));
        memset(solved_loop,0,1024*sizeof(char));

        // a partir de acá el slave recibe los archivos de a uno

        int read_res = read(fd_read , file_loop, 1024*sizeof(char));
        if (read_res == -1) {
            perror("read on slave in loop");
            return 1;
        } 

        if ( strncmp(file_loop,"END",3) == 0 ) {
            close(fd_read);
            close(fd_write);
            break;
        }

        // solve file read on fifo and send in on solved.
        solveFile(file_loop, solved_loop);

        int write_res = write(fd_write, solved_loop, strlen(solved_loop));
        if(write_res == -1) {
            perror("write on slave in loop");
            return 1;
        }

        free(file_loop);
        free(solved_loop);
    }


    close(fd_write);
    close(fd_read);

    return 0;
}


 
void set_fifo_path( int identifier ) {
    sprintf( fifo_write_path, "/tmp/fifos-slave-%d", identifier );
    sprintf( fifo_read_path, "/tmp/fifos-parent-%d", identifier );
    return;
}
