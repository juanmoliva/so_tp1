#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#define NUM_SLAVES 1
#define MAX_FILES 200
#define INITIAL_FILES_FOR_SLAVE "2"


/*
    TO DO
    // error handling
    // falta aceptar los archivos por linea de comandos.
*/    // accept dir/* , dir/file.cnf , file.cnf as parameters

//Vector donde guardamos cada uno de los archivos [GLOBAL]
//FALTA RECIBIRLOS POR LINEA DE COMANDO
char *files[12] = {"./files/bart10.shuffled.cnf","./files/bart11.shuffled.cnf","./files/bart12.shuffled.cnf","./files/bart13.shuffled.cnf","./files/bart14.shuffled.cnf","./files/bart15.shuffled.cnf","./files/bart16.shuffled.cnf","./files/bart17.shuffled.cnf","./files/bart18.shuffled.cnf","./files/bart19.shuffled.cnf","./files/bart20.shuffled.cnf","./files/bart21.shuffled.cnf"};

//Aca vamos marcando por que archivo vamos del vector
int current_file = 0;

int main(int argc, char *argv[])
{

//Esto no permite que corra SIN recibir parametros
    /*if ( argc < 2 ) {
        printf("usage: solve [FILES] \n");
        printf("files must be .cnf\n");
        return 1;
    }*/
	
   
//Make FIFOS -> Tienen un string que identifica cada pipe (esa es la diferencia con los pipes normales)
//Creamos 2 pipes por cada esclavo para conectar padre a hijo y viceversa
	//Les ponemos estos nombres para despues poder referenciarlos desde cada uno de los slaves
    for(int i = 0; i < NUM_SLAVES ; i++) {
        char fifo_path_parent[32], fifo_path_slave[32];
	    //Le ponemos el nombre y creamos al pipe del parent
        sprintf(fifo_path_parent, "/tmp/fifo-parent-%d", i);
	    //CREA EL PIPE
        mkfifo(fifo_path_parent, 0666);
	    //Idem para slave
        sprintf(fifo_path_slave, "/tmp/fifo-slave-%d", i);
        mkfifo(fifo_path_slave, 0666);
    }


	// Create NUM_SLAVES slaves
	pid_t pid;
	pid_t slaves_pids[NUM_SLAVES];
	//Creamos el primer slave
	pid = fork();
	int j = 0;

	//Creamos el resto de los slaves y guardamos sus respectivos pid's
	for(;pid != 0 && j< NUM_SLAVES - 1 ;j++)
	{ 
		slaves_pids[j] = pid;
		pid = fork();
	 } 
	
//Un arreglo donde guardamos los file descriptors (un numero x cada archivo) -> el numero es lo q devuelve open, es el numero q te dio el SO
    int fd_fifos[NUM_SLAVES];

	
    if ( pid != 0 ) {
	//ACA ENTRA SOLO EL PADRE Y LE MANDA LOS PATHS DE LOS ARCHIVOS A CADA SLAVE!!!!!!!!!!
	    //Aca guardamos el ultimo pid q no se guardo en el ciclo for
        slaves_pids[NUM_SLAVES-1] = pid; 

        // write to FIFO's
        // distribute initial files to slaves
        for(int i = 0; i < NUM_SLAVES ; i++) {
		//Armo el path para poder acceder al pipe indicado (creados arriba)
            char fifo_path[32];
            sprintf(fifo_path, "/tmp/fifo-parent-%d", i);
		//Te devuelve el int donde tenes que escribir dps
            fd_fifos[i] = open(fifo_path, O_WRONLY);
		
												//linea de prueba
            char buf[] =  "./files/bart10.shuffled.cnf;./files/bart11.shuffled.cnf";
		
	   //Escribis en ese respectivo fd, lo que esta en buf con su respectivo tamaño
            write(fd_fifos[i], buf, sizeof(buf));
           //Cierro el archivo.
            close(fd_fifos[i]);
        }
    } else {        // SLAVES
	    
	    //CREO UN VECTOR DE CHAR CON SU RESPECTIVO ID x CADA SLAVE
        char j_char[32];
        sprintf(j_char, "%d", j);
	    
	    //Parametros para ejecutar en la consola de comandos
	    //Aca los hijos se transforman en SLAVES y comienzan a resolver los archivos
        char *args_slave[]={ "./slave" , INITIAL_FILES_FOR_SLAVE , j_char , NULL}; 
	    //1er Parametro: Nombre del archivo "./slave" .     2do Parametro: Array de strings q tenga todos los parametros
        execvp(args_slave[0],args_slave); 
	    //si exec retorna significa que hubo un problema
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


    retval = select(nfds, &rfds, NULL, NULL, &tv);

    if (retval == -1)
        perror("select()");
    else if (retval)
        printf("Data is available now.\n");
        /* FD_ISSET(0, &rfds) will be true. */
    else
        printf("No data within five seconds.\n");
    




    return 0;

}
