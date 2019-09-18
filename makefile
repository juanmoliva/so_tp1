CC=gcc
default:
	$(CC) -Wall -g -std=c99 -pthread -o solve solve.c -lrt 
	$(CC) -Wall -g -std=c99 -pthread -o slave slave.c -lrt 
	$(CC) -Wall -g -std=c99 -pthread -o vista vista.c -lrt 
exec:
	./solve $(PWD)/files_bart/ | ./vista
mixed:
	./solve $(PWD)/files_mixed/ | ./vista
