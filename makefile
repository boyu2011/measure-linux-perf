
program:program.o
	gcc -o program program.o -D_REENTRANT -lpthread
program.o:program.c
	gcc -c -g program.c
clean:
	rm -f program.o program tmp
