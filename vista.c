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
	long long bytes_read = 0;
	char pid[6];
	read(STDIN_FILENO, pid, 6);

	printf("pid leido: %s \n", pid);

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

    sem_wait(sem_id);

    int loop = 0;
    while( 1 ) {
    	printf("NEW LOOP !!!! \n");
		char *current = calloc(1,2048*sizeof(char *));
		char *token = calloc(1,1024*sizeof(char *));
		strcpy(current, vista_char_addr);
		token = strtok(current,"&&&");
    	for (int i= 0 ; i< loop; i++) { token = strtok(NULL,"&&&");}
    	printf("loop es: %d\n", loop);
    	printf("token quedo: '%s'\n", token);
    	if (strcmp(token, "ENDSHM") == 0){
    		printf("finalizando memoria\n");
    		break;
    	}
    	loop++;
    	sem_wait(sem_id);
    	

	}

	return 0;
    /*int loop = 0;
    
    while( loop <= 7) {
    	printf("NEW LOOP !!!! \n");
    	char* outputs[MAX_FILES];
    	int i = 0;
	    outputs[i] = strtok (vista_char_addr,"&&&");
	    while ( outputs[i] != NULL){
	    	printf("%s\n", outputs[i]);
	        i++;
	        outputs[i] = strtok (NULL, ";");
	    }
	    sem_wait(sem_id);
    	loop++;
    	
    	int count=1;
    	char *str = (char *) calloc(4, 512*sizeof(char*));
    	str = strtok(vista_char_addr,"&&&");
    	for (int i= 0 ; i< loop; i++) { str = strtok(NULL,"&&&"); count++;}
    	printf("count es: %d\n", count);
    	printf("str quedo: '%s'\n", str );
    	
    }*/
    

    /*char *file = strtok (vista_char_addr,"&&&");
    printf("%s\n", file);

    int running = 1;
	while(running) {
		sem_wait(sem_id);
		char *file = strtok (NULL,"&&&");
		if( strcmp(file,"ENDSHM")==0){ running = 0; }
		else { printf("%s\n\n", file); }
		//strcpy(new_file, vista_char_addr + bytes_read);
		//bytes_read += strlen(new_file);
		
		
	};*/


	

}
