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
	int bytes_read = 0;
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
	void *vista_addr = mmap(NULL, 50*1024, PROT_WRITE, MAP_SHARED, fd, 0);
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

    printf("%s\n", vista_char_addr);

	while(1) {
		char new_file[1024];
		strcpy(new_file, vista_char_addr + bytes_read);
		bytes_read += strlen(new_file);
		printf("%s\n", new_file) ;
		sem_wait(sem_id);
	};

}
