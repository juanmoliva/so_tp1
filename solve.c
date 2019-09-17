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
#define NUM_SLAVES 2
#define MAX_FILES 70
#define INITIAL_FILES_FOR_SLAVE 5

int ftruncate(int fd, off_t length);

//Vector donde guardamos cada uno de los archivos [GLOBAL]
char *files[ MAX_FILES + NUM_SLAVES];

//Aca vamos marcando por que archivo vamos del vector
int current_file = 0;
int files_tosolve = 0;

char *fifo_read_path[NUM_SLAVES];
char *fifo_write_path[NUM_SLAVES];

void set_fifo_paths();

int main(int argc, char *argv[])
    {



    // Esto no permite que corra SIN recibir parametros
    if ( argc < 2 ) {
            printf("usage: solve [FULL PATH OF FILES FOLDER] \n");
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
    char *dir_param = (char*)calloc(1, 512*sizeof(char));
    strncpy( dir_param ,argv[1], strlen(argv[1]));
	
    //Abrimos el directorio 
    pDir = opendir (dir_param);
	
    //Creamos vector donde van a estar todos los PATHS ABSOLUTOS de los archivos a resolver	
    char * files[MAX_FILES];
    
    //Validamos que no de error
    if (pDir == NULL) {
        printf ("Cannot open directory '%s'\n",dir_param);
        return 1;
    }
    
    int i = 0;
    // readdir devuelve el primer archivo del directorio cada vez que se asigna
    // Le asignamos a pDirent cada uno de los directorios en cada iteracion 
    // y le agregamos lo recibido por argv para tener el full path
   
    while ((pDirent = readdir(pDir)) != NULL) {
        if( strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0 ){
            //Asignamos lugar a los punteros pq tienen por default 10 (es poco)
        char *full_path = (char *)calloc(1 ,512 *sizeof(char));
        //Appendeamos el full path + name
            strcat(full_path,dir_param);
            strcat(full_path,pDirent->d_name);
        
        //Guardamos en files todos los caminos
            files[i] = (char *) calloc(1, 1024*sizeof(char));
            strcpy(files[i],full_path);
            i++;

            free(full_path);
        //Contamos cuantos archivos tenemos por resolver
            files_tosolve++;
        }
        
    }


    
    // last "files" are "END" string literals.
    char end[4] = "END";
    for( int k = 0 ; k<NUM_SLAVES ; k++) {
        files[i] = (char *) calloc(1, 32*sizeof(char));
        strncpy(files[i], end,4);
        files_tosolve++;
        i++;
    } 
    
    free(dir_param);
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

    // imprimimos pid a stdout para el proceso vista
    printf("%d\n", getpid() );

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Make FIFOS -> Tienen un string que identifica cada pipe
    //Creamos 2 pipes por cada esclavo para conectar padre a hijo y viceversa
    //Les ponemos estos nombres para despues poder referenciarlos desde cada uno de los slaves

    set_fifo_paths();

    for(int i = 0; i < NUM_SLAVES ; i++) {
        remove(fifo_write_path[i]);
        remove(fifo_read_path[i]);
        int res = mkfifo(fifo_write_path[i], 0666);
        if( res != 0 ) {
            perror("mkfifo");
            return 1;
        }
        int res_read = mkfifo(fifo_read_path[i], 0666);
        if( res_read != 0 ) {
            perror("mkfifo");
            return 1;
        }
    }

   /* // armamos los buffers iniciales que se enviaran a cada slave
    char buf[NUM_SLAVES][1024];

    for(int i = 0 ; i < NUM_SLAVES; i++){
        for( int j = 0; j< INITIAL_FILES_FOR_SLAVE ; j++) {
            if(current_file < files_tosolve ){
                printf("copying '%s' to buf[%d]\n",files[current_file],i );
                strcat(buf[i], files[current_file]);
                strcat(buf[i],";");
                current_file++;
            }    
        }
    }*/

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
    	
    //arreglos donde guardamos los file descriptors.
    int fd_write_fifos[NUM_SLAVES];
    int fd_read_fifos[NUM_SLAVES];

	
    if ( pid != 0 ) {
	   // padre
	    
        // write to FIFO's
        // distribute initial files to slaves
        for(int i = 0; i < NUM_SLAVES ; i++) {
            char* buf = (char *)calloc(INITIAL_FILES_FOR_SLAVE, 512*sizeof(char *));
            for( int j = 0; j< INITIAL_FILES_FOR_SLAVE ; j++) {
                if(current_file < (files_tosolve- NUM_SLAVES) ){
                    printf("current_file %d y (files_tosolve-NUM_SLAVES) %d\n", current_file, (files_tosolve- NUM_SLAVES));
                    strncat(buf, files[current_file], strlen(files[current_file]));
                    strcat(buf,";");
                    current_file++;
                }    
            }

            fd_write_fifos[i] = open(fifo_write_path[i] , O_WRONLY);
            write(fd_write_fifos[i], buf, strlen(buf));
            free(buf);
        }
    } else { 
        // slaves
	    
	    //CREO UN VECTOR DE CHAR CON SU RESPECTIVO ID x CADA SLAVE
        char j_char[32];
        sprintf(j_char, "%d", j);
	    
	    //Parametros para ejecutar en la consola de comandos
	    //Aca los hijos se transforman en SLAVES y comienzan a resolver los archivos
        char *args_slave[]={ "./slave" , j_char , NULL}; 
	    //1er Parametro: Nombre del archivo "./slave" .     2do Parametro: Array de strings q tenga todos los parametros
        execvp(args_slave[0],args_slave); 
	    //si exec retorna significa que hubo un problema
        printf("problem with exec\n");
        return 2;
    }
	

    //Abre todos los canales de LECTURA de los pipes donde le van a escribir los slaves
    for(int i= 0; i<NUM_SLAVES; i++) {
        fd_read_fifos[i] = open(fifo_read_path[i] , O_RDONLY);
        if (fd_read_fifos[i] == -1) {
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
        if(fd_read_fifos[i] > nfds) nfds = fd_read_fifos[i];
        FD_SET(fd_read_fifos[i], &rfds);
    }
    nfds++;


    int loop_test = 1;
    // loop principal, comunicacion entre proceso principal y slaves.
    
    int slaves_working[NUM_SLAVES];
    for (int i=0; i< NUM_SLAVES; i++) {
        slaves_working[i] = 1;
    }

    while( current_file < files_tosolve ){
        retval = select(nfds, &rfds, NULL, NULL, NULL);
        printf("loop_test es %d, retval es %d\n current_file es %d y files_tosolve %d.\n", loop_test,retval,current_file,files_tosolve );
        

        if (retval == -1) {
            perror("select()");
            return 1;
        }
        else {
            for( int i = 0 ; i < NUM_SLAVES ; i++ ) {
                if(slaves_working[i] && FD_ISSET(fd_read_fifos[i],&rfds)) {
                    // printf("despues del select, el slave %d esta disponible\n", i);
                    // leer del pipe de esclavo el archivo resuelto
                    printf("en loop %d,slave %d tiene algo para leer\n",loop_test,i);
                    char *file = (char*) malloc(1024*INITIAL_FILES_FOR_SLAVE*sizeof(char));
                    memset(file, 0, 1024*INITIAL_FILES_FOR_SLAVE*sizeof(char));

                    int read_res = read(fd_read_fifos[i], file, 1024*INITIAL_FILES_FOR_SLAVE*sizeof(char));
                    if(read_res == -1) {
                        perror("read on select");
                        return 1;
                    }
                    
                    // pasar al proceso vista
                    printf("read after select:\n %s\n", file );
                    strncat(str_shm, file, strlen(file));
                    sem_post(sem_id);

                    int write_res = write(fd_write_fifos[i] , files[current_file],strlen(files[current_file]));
                    if(write_res == -1) {
                        perror("write on select");
                        return 1;
                    }
                    printf("al slave %d le mandamos %s\n",i,files[current_file]);

                    // si mandamos el string "END" a un slave, no queremos que ese slave esté en el proximo select().
                    if (strcmp(files[current_file],"END") == 0) { slaves_working[i] = 0; }

                    current_file++;

                    

                    
                    free(file);
                }

                FD_ZERO(&rfds);
                for(int i = 0 ; i < NUM_SLAVES ; i++) {
                    if( slaves_working[i]) {
                        if(fd_read_fifos[i] > nfds) nfds = fd_read_fifos[i];
                        FD_SET(fd_read_fifos[i], &rfds);
                    }
                    
                }
            }

        }
        loop_test++;

    }

    /*// receive last files sended char 
    char *last_file = (char*) malloc(1024*sizeof(char));
    memset(last_file, 0, 1024*sizeof(char));

    int read_res = read(fd_read_fifos[i], last_file, 1024*sizeof(char));
    if(read_res == -1) {
            perror("read on select");
            return 1;
    }

    free(last_file);
*/

    // terminacion
    for( int i = 0 ; i < NUM_SLAVES ; i++ ) {
        printf("enter termination on solve\n");
        close(fd_read_fifos[i]);
/*
        char end[5] = "END";
        int write_res = write(fd_write_fifos[i] , end,strlen(end));
        if(write_res == -1) {
            perror("write on termination");
            return 1;
        }*/

        close(fd_write_fifos[i]);
    }
    

    return 0;

}

void set_fifo_paths() {
    char path[32];
    for( int i = 0 ; i < NUM_SLAVES ; i++ ){
        fifo_write_path[i] = (char *) malloc(32*sizeof(char));
        fifo_read_path[i] = (char *) malloc(32*sizeof(char));
        sprintf(path,"/tmp/fifos-parent-%d", i);
        strcpy(fifo_write_path[i],path);
        sprintf(path,"/tmp/fifos-slave-%d", i);
        strcpy(fifo_read_path[i],path);
    }
}