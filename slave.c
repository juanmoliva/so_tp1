#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	// argv[1] should be an int
    int initial_files = atoi(argv[1]);

    printf("hello from slave , %d files to receive.\n", initial_files );

    char fifo_path[32];
    sprintf(fifo_path, "/tmp/fifo-%d", getpid());

    int fd  = open(fifo_path, O_RDONLY);
    char buf[1024];

    int n = read(fd, buf, sizeof(buf));

    printf("message received from pid %d : %s\n", getpid(), buf );
    close(fd);

    return 0;
}
