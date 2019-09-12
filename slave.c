#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	// argv[1] is the initial number of files the slave will receive, argv[2] is the slave's identifier
    int initial_files = atoi(argv[1]);
    int identifier = atoi(argv[2]);

    printf("hello from slave , %d files to receive. My identifier is %d \n", initial_files, identifier );

    char fifo_path[32];
    sprintf(fifo_path, "/tmp/fifo-%d", identifier);

    int fd  = open(fifo_path, O_RDONLY);
    char buf[1024];

    int n = read(fd, buf, sizeof(buf));

    printf("message received from pid %d : %s\n", getpid(), buf );
    close(fd);

    return 0;
    
}
