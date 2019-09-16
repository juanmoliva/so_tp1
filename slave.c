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

void parse_output( char *solved);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Resuelve el archivo - Le pasamos un puntero al archivo y un puntero para que deje la rta  //////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int solveFile(char *file, char *solved ) {
     //////////////////////////////////////////// Preparamos el canal de escritura y lectura del pipe /////////////////////////////////
     int link[2];
     /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     
     ///////////////////////// En local_solved ESTÁ EL OUTPUT DE MINISAT y pid va a ser para el fork /////////////////////////////////
     pid_t pid;
     char *local_solved= (char *)malloc( 8096 *sizeof(char) );
     /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     
     ////////////////////////////////////////// Crea el pipe y se fija si da error ////////////////////////////////////////////////////////////////
     if (pipe(link)==-1) {
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

      ////////////////////////////////////////// Entra si es el PADRE ////////////////////////////////////////////////////////////////
     else {
         //Feedback de que funciona
         //printf("exec will call minisat %s \n", file );
         
         //El padre no escribe entonces lo cerramos
         close(link[1]);
         
         //Lee del 1er Parametro la cantidad del 3er Parametro y lo guarda en el parametro del medio (tiene que ser un puntero)
         int nbytes = read(link[0], local_solved, 2048 *sizeof(char));
    // /*//////
    // ACA EN local_solved ESTÁ EL OUTPUT DE MINISAT
    // habría que llamar una funcion que sea algo así:
    // parse_output(local_solved);
    // y que deje la variable con esta info:
    //     Nombre de archivo.
    //     Cantidad de cláusulas
    //     Cantidad de variables
    //     Resultado (SAT | UNSAT)
    //     Tiempo de procesamiento
    //     ID del esclavo que lo procesó.
    // //////*/
         parse_output(local_solved);
         strcat(local_solved, file);

         
         strcpy(solved, local_solved);

         wait(NULL);
         return nbytes;
     }
     return -1;
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
        char *this_file = (char *)malloc(2048*sizeof(char));
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
        printf("recibido en slave '%s'\n", file_loop);
        if (read_res == -1) {
            perror("read on slave in loop");
            return 1;
        }


        if ( strcmp(file_loop,"END") ) {
            printf("in slave %d we reached termination\n", identifier);
            close(fd);
            break;
        }
        close(fd);

        // solve file read on fifo and send in on solved.
        solveFile(file_loop, solved_loop);

        strcpy(solved_loop, "mensaje mandado de slave!");
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
    sprintf( fifo_path, "/tmp/fifo-%d", identifier );
}

void parse_output(char* solved) {
    // popen = corre el primer parametro y crea un pipe entre el fork que hace para correrlo y el programa actual (aca), el segundo parametro
    // es "r" que indica que voy a leer lo que me mande el fork (salida del minisat con egrep)
    // devuelve un tipo FILE que tengo que pasar a *char para devolverlo en solved 
    FILE *ff = popen("grep -e  'Number of variables' -e 'Number of clauses' -e 'CPU time' -e 'SAT'  local_solved", "r"); // me quedo con lo que me interesa, 
    if (ff == NULL){ perror("popen"); return;}
        FILE *faux = ff = fopen(ff, "r");  // con fopen abro el FILE que devuelve popen
         while (fgets(solved, 8192 , faux) != EOF){ // con esto copio line por line de lo que tiene faux a solved
             fclose(ff);
         }

        char pid[10];
        sprintf(pid, "%d", getpid());
        strcat(solved, pid); //copio el pid 
        int status = pclose(ff); // cierro popen
        if (status == -1) {     // manejo de error popen
            perror("pclose");
            return ;
        }
}