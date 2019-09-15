#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>


/////////////////////////////// Puntero a los archivos a resolver Y un contador  ////////////////////////////////////////

char *files_tosolve[100];
int current_files = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Resuelve el archivo - Le pasamos un puntero al archivo y un puntero para que deje la rta  //////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int solveFile(char *file, char *solved ) {

    //////////////////////////////////////////// Preparamos el canal de escritura y lectura del pipe /////////////////////////////////
    int link[2];
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    ///////////////////////////////////////////////////// ???????? ///////////////////////////////////////////////////////////
    pid_t pid;
    char local_solved[4096];
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





    ////////////////////////////////////////// Crea el pipe y se fija si da error ////////////////////////////////////////////////////////////////
    if (pipe(link) == -1) {
        perror("pipe");
        return -1;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





    ////////////////////////////////////////// Hace el FORK y se fija si da error ////////////////////////////////////////////////////////////////
    if ((pid = fork()) == -1){
        perror("fork");
        return -1;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





    ////////////////////////////////////////// Entra si es el HIJO ////////////////////////////////////////////////////////////////

    if(pid == 0) {

        /* Seteamos para que el SLAVE devuelva lo mismo   
         que devolvia en STDOUT a devolverlo en link[1] */
        dup2 (link[1], STDOUT_FILENO);
        

        // Cerramos ambos links 
        close(link[0]);
        close(link[1]);
        

        // Armamos el parametro para pasarle a "execvp"
        char *args_slave[]={ "minisat" , file , NULL};
        execvp(args_slave[0],args_slave); 

        // Si el exec retorna significa que hubo un error 
        perror("exec");
        return -1;

    } 
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    else {
        printf("exec will call minisat %s \n", file );
        close(link[1]);
        int nbytes = read(link[0], local_solved, sizeof(local_solved));
    /*//////
    ACA EN local_solved ESTÁ EL OUTPUT DE MINISAT
    habría que llamar una funcion que sea algo así:
    parse_output(local_solved);
    y que deje la variable con esta info:
        Nombre de archivo.
        Cantidad de cláusulas
        Cantidad de variables
        Resultado (SAT | UNSAT)
        Tiempo de procesamiento
        ID del esclavo que lo procesó.
    //////*/
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
    printf("Im slave %d, I received '%s'", identifier, buf);

    

    // open fifo for writing
    int send_fd = open(fifo_slave_path, O_WRONLY);

    char *file = strtok (buf,";");
    while (file!= NULL){
        files_tosolve[current_files] = file;
        printf("slave, %d, file %d, %s\n", identifier, current_files, files_tosolve[current_files] );
        
        current_files++;
        file = strtok (NULL, ";");
    }


    char solved[8192];

    printf("current_files %d\n",current_files );
    // solve the initial files.

    while(current_files > 0) {
        current_files--;
        char this_file[4096];
        printf("slave %d, solving %s \n", identifier, files_tosolve[current_files]);
        solveFile(files_tosolve[current_files], this_file);
        strcat(solved, this_file);
        strcat(solved, "\n");
    }

    int end = 0;
    while(!end) {
        // a partir de acá el slave recibe los archivos de a uno
        char new_file[2048];
        write(send_fd, solved, sizeof(solved));
        int nbytes = read(fd, new_file, sizeof(new_file));
        if (nbytes == -1) {
            perror("read file on slave");
            return 1;
        } else if ( nbytes != 0 ){
            solveFile(new_file, solved);
        }
        else {
            end = 1;
        }
    }

    close(send_fd);
    close(fd);

    return 0;
}
