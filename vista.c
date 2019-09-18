 #include <sys/types.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#define MAX_FILES 50

int fileno(FILE *stream);

int main(void)
{
	char pid[6];
	read(STDIN_FILENO, pid, 6);

	// acceso a memoria compartida con el proceso solve
	char shm_path[32], sem_path[32];
	sprintf(shm_path, "/shm-%d", atoi(pid));
	sprintf(sem_path, "/sem-%d", atoi(pid));

	int fd = shm_open(shm_path, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		perror("open in vista");
		return 1;
	}
	// mapeo de memoria.
	void *vista_addr = mmap(NULL, MAX_FILES*2048, PROT_WRITE, MAP_SHARED, fd, 0);
	if (vista_addr == MAP_FAILED)
	{
		perror("mmap in vista");
		return 1;
	}
	char *vista_char_addr = (char *)vista_addr;

	// accedemos al semaforo.
    sem_t *sem_id = sem_open(sem_path, O_RDWR, 0);
    if( sem_id == SEM_FAILED ){
        perror("sem_open in vista");
        return 1;
    }


    // la lectura sobre la shared memory se realiza mediante tokens extraidos del string vista_char_addr en cada iteración, el semaforo 
    // es utilizado para sincronizar los procesos vista y solve.

    sem_wait(sem_id);

    int loop = 0;
    while( 1 ) {
		
		char *current = calloc(1,2048*sizeof(char *));
		char *token = calloc(1,1024*sizeof(char *));
		strcpy(current, vista_char_addr);
		
		token = strtok(current,"&&&");
    	for (int i= 0 ; i< loop; i++) { token = strtok(NULL,"&&&");}
    	
    	if (strcmp(token, "ENDSHM") == 0){
    		break;
    	}
    	printf("%s\n", token);
    	loop++;
    	sem_wait(sem_id);

    	free(current);
	}

	return 0;

}
