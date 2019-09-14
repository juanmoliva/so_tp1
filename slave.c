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

    char fifo_parent_path[32], fifo_slave_path[32];
    sprintf(fifo_parent_path, "/tmp/fifo-parent-%d", identifier);
    sprintf(fifo_slave_path, "/tmp/fifo-slave-%d", identifier);

    int fd  = open(fifo_parent_path, O_RDONLY);
    char buf[1024];

    read(fd, buf, sizeof(buf));

    printf("message received from pid %d : %s\n", getpid(), buf );

    close(fd);

    int send_fd = open(fifo_slave_path, O_WRONLY);

    close(send_fd);
    
    return 0;

}
