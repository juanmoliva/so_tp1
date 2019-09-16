#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>
#include <semaphore.h>
#include <dirent.h>
#define NUM_SLAVES 3
#define MAX_FILES 70
#define INITIAL_FILES_FOR_SLAVE 2

int ftruncate(int fd, off_t length);

/*
    TO DO
    // error handling
*/

//Vector donde guardamos cada uno de los archivos [GLOBAL]
char *files[MAX_FILES];

//Aca vamos marcando por que archivo vamos del vector
int current_file = 0;
int files_tosolve = 0;

char *fifo_path[NUM_SLAVES];

void set_fifo_paths();

int main(int argc, char *argv[])
    {

    // Esto no permite que corra SIN recibir parametros
    if ( argc < 2 ) {
            printf("usage: solve [FILES] \n");
            printf("files must be .cnf\n");
            return 1;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Tomamos EL PATH DE LA CARPETA QUE CONTIENE LOS ARCHIVOS  ////////////////////////////////////////
/////////////////////////////// x argv y guardamos los nombre de archivos en variable: files ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    //Variables para manejo de directorios
    struct dirent * pDirent;
    DIR * pDir;
	

    //Abrimos el directorio 
    pDir = opendir (argv[1]);
	
    //Creamos vector donde van a estar todos los PATHS ABSOLUTOS de los archivos a resolver	
    char * files[MAX_FILES];
    
    //Validamos que no de error
    if (pDir == NULL) {
        printf ("Cannot open directory '%s'\n", argv[1]);
        return 1;
    }

    int i = 0;
    // readdir devuelve el primer archivo del directorio cada vez que se asigna
    // Le asignamos a pDirent cada uno de los directorios en cada iteracion 
    // y le agregamos lo recibido por argv para tener el full path

    while ((pDirent = readdir(pDir)) != NULL) {
	//Asignamos lugar a los punteros pq tienen por default 10 (es poco)
        files[i] = malloc(1024*sizeof(char));
        char *full_path = (char *) calloc(4, 100*sizeof(char));
	
	//Appendeamos el full path + name
        strcat(full_path,argv[1]);
        strcat(full_path,pDirent->d_name);
	
	//Guardamos en files todos los caminos
        if ( strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0 ) {
            strcpy(files[i++],full_path);
        }
        
	//Contamos cuantos archivos tenemos por resolver
        files_tosolve++;
    }


    closedir (pDir);
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// comunicación y sincronización con el proceso vista. ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    char shm_path[32], sem_path[32];
    sprintf(shm_path, "/shm-%d", getpid());
    sprintf(sem_path, "/sem-%d", getpid());

    int fd_shm = shm_open(shm_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
    {
        perror("open shm");
        return 1;
    }

    // aumentar tamaño de shared memory
    int res_truncate = ftruncate(fd_shm, MAX_FILES * 1024);
    if (res_truncate == -1)
    {
        perror("ftruncate");
        return 1;
        }
    	
    void *addr_shm = mmap(NULL, MAX_FILES * 1024, PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_shm == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    char *str_shm = (char *)addr_shm;

    // inicializamos el semaforo.
    sem_t *sem_id = sem_open(sem_path, O_CREAT, 0600, 0);
    if( sem_id == SEM_FAILED ){
        perror("sem_open");
        return 1;
    }

    strcpy(str_shm, "hello");

    // imprimimos pid a stdout para el proceso vista
    printf("%d\n", getpid() );

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Make FIFOS -> Tienen un string que identifica cada pipe
    //Creamos 2 pipes por cada esclavo para conectar padre a hijo y viceversa
    	//Les ponemos estos nombres para despues poder referenciarlos desde cada uno de los slaves

    set_fifo_paths();

    /*for(int i = 0; i < NUM_SLAVES ; i++) {
        char fifo_path_parent[32], fifo_path_slave[32];
        //Le ponemos el nombre y creamos al pipe del parent
        sprintf(fifo_path_parent, "/tmp/fifo-parent-%d", i);
	    //CREA EL PIPE
        mkfifo(fifo_path_parent, 0666);
        //Idem para slave
        sprintf(fifo_path_slave, "/tmp/fifo-slave-%d", i);
        mkfifo(fifo_path_slave, 0666);
    }*/

    for(int i = 0; i < NUM_SLAVES ; i++) {
        remove(fifo_path[i]);
        int res = mkfifo(fifo_path[i], 0666);
        if( res != 0 ) {
            perror("mkfifo");
            return 1;
        }
    }

    // armo los buffers iniciales que se enviaran a cada slave
    char buf[NUM_SLAVES][1024];

    for(int i = 0 ; i < NUM_SLAVES; i++){
        // buf[i] = (char *) malloc(1024*sizeof(char));

        for( int j = 0; j< INITIAL_FILES_FOR_SLAVE ; j++) {
            if(current_file < files_tosolve ){
                strcat(buf[i], files[current_file]);
                strcat(buf[i],";");
                current_file++;
            }    
        }
        // printf("we will send '%s' to slave %d \n", buf[i], i );
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Create NUM_SLAVES slaves /////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pid_t pid;

	//Creamos el primer slave
	pid = fork();
	int j = 0;

	//Creamos el resto de los slaves y guardamos sus respectivos pid's
	for(;pid != 0 && j< NUM_SLAVES - 1 ;j++)
	{ 
		pid = fork();
	 } 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	
    //Un arreglo donde guardamos los file descriptors (un numero x cada archivo) -> el numero es lo q devuelve open, es el numero q te dio el SO
    int fd_fifos[NUM_SLAVES];

	
    if ( pid != 0 ) {
	   //ACA ENTRA SOLO EL PADRE Y LE MANDA LOS PATHS DE LOS ARCHIVOS A CADA SLAVE!!!!!!!!!!
	    
        // write to FIFO's
        // distribute initial files to slaves
        for(int i = 0; i < NUM_SLAVES ; i++) {
		    /*//Armo el path para poder acceder al pipe indicado (creados arriba)
            char fifo_path[32];
            sprintf(fifo_path, "/tmp/fifo-parent-%d", i);*/

		    //Te devuelve el int donde tenes que escribir dps
            fd_fifos[i] = open(fifo_path[i] , O_WRONLY);

	       //Escribis en ese respectivo fd, lo que esta en buf con su respectivo tamaño
            write(fd_fifos[i], buf[i], sizeof(buf[i]));

           //Cierro el archivo.
            close(fd_fifos[i]);
        }
    } else { 
        // SLAVES
	    
	    //CREO UN VECTOR DE CHAR CON SU RESPECTIVO ID x CADA SLAVE
        char j_char[32];
        sprintf(j_char, "%d", j);
        char n_offiles[7];
        sprintf(n_offiles,"%d", INITIAL_FILES_FOR_SLAVE);
	    
	    //Parametros para ejecutar en la consola de comandos
	    //Aca los hijos se transforman en SLAVES y comienzan a resolver los archivos
        char *args_slave[]={ "./slave" , n_offiles , j_char , NULL}; 
	    //1er Parametro: Nombre del archivo "./slave" .     2do Parametro: Array de strings q tenga todos los parametros
        execvp(args_slave[0],args_slave); 
	    //si exec retorna significa que hubo un problema
        printf("problem with exec\n");
        return 2;
    }
	
	/*//Aca se guardan los int que te da el "open" para poder leer.
    int fd_slaves[NUM_SLAVES];
    	
    //Abre todos los canales de LECTURA de los pipes donde le van a escribir los slaves
    for(int i= 0; i<NUM_SLAVES; i++) {
        char fifo_path[32];
        sprintf(fifo_path, "/tmp/fifo-slave-%d", i);
        fd_slaves[i] = open(fifo_path, O_RDONLY);
    }
*/

    //Abre todos los canales de LECTURA de los pipes donde le van a escribir los slaves
    for(int i= 0; i<NUM_SLAVES; i++) {
        fd_fifos[i] = open(fifo_path[i] , O_RDONLY);
        if (fd_fifos[i] == -1) {
            perror("open for reading");
            return -1;
        }
    }

    // use select() to read from multiple fd's without getting blocked.
    	
    int nfds = 0;
    fd_set rfds;
    int retval;

    FD_ZERO(&rfds);
    for(int i = 0 ; i < NUM_SLAVES ; i++) {
        if(fd_fifos[i] > nfds) nfds = fd_fifos[i];
        FD_SET(fd_fifos[i], &rfds);
    }
    
    /*for(int i = 0 ; i < NUM_SLAVES ; i++) {
        if(fd_slaves[i] > nfds) nfds = fd_slaves[i];
        FD_SET(fd_slaves[i], &rfds);
    }
*/    nfds++;
/*
    // wait up to 30 seconds
    tv.tv_sec = 30;
    tv.tv_usec = 0;
*/

    // printf("nfds es %d\n", nfds);
    while( current_file < files_tosolve ){
        printf("current_file es %d\n",current_file );
        retval = select(nfds, &rfds, NULL, NULL, NULL);

        if (retval == -1) {
            perror("select()");
            return 1;
        }
        else if (retval){
            for( int i = 0 ; i < NUM_SLAVES ; i++ ) {
                if(FD_ISSET(fd_fifos[i],&rfds)) {
                    // printf("despues del select, el slave %d esta disponible\n", i);
                    // leer del pipe de esclavo el archivo resuelto
                    char *file = (char*) malloc(2048*sizeof(char));

                    int read_res = read(fd_fifos[i], file, 2048*sizeof(char));
                    if(read_res == -1) {
                        perror("read on select");
                        return 1;
                    }
                    
                    // pasar al proceso vista
                    printf("%s\n", file );
                    strncat(str_shm, file, strlen(file));
                    sem_post(sem_id);


                    close(fd_fifos[i]);
                    fd_fifos[i] = open(fifo_path[i], O_WRONLY);

                    int write_res = write(fd_fifos[i] , files[current_file],strlen(files[current_file]));
                    if(write_res == -1) {
                        perror("write on select");
                        return 1;
                    }

                    current_file++;

                    close(fd_fifos[i]);
                    fd_fifos[i] = open(fifo_path[i], O_RDONLY);

                    FD_ZERO(&rfds);
                    for(int i = 0 ; i < NUM_SLAVES ; i++) {
                        if(fd_fifos[i] > nfds) nfds = fd_fifos[i];
                        FD_SET(fd_fifos[i], &rfds);
                    }

                    

                }
            }

        }
    }

    // terminacion
    for( int i = 0 ; i < NUM_SLAVES ; i++ ) {
        close(fd_fifos[i]);
        fd_fifos[i] = open(fifo_path[i], O_WRONLY);

        char end[5] = "END";
        int write_res = write(fd_fifos[i] , end,strlen(end));
        if(write_res == -1) {
            perror("write on termination");
            return 1;
        }

        close(fd_fifos[i]);
    }
    

    return 0;

}

void set_fifo_paths() {
    char path[32];
    for( int i = 0 ; i < NUM_SLAVES ; i++ ){
        fifo_path[i] = (char *) malloc(32*sizeof(char));
        sprintf(path,"/tmp/fifos-%d", i);
        strcpy(fifo_path[i],path);
    }
}