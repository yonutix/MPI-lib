build:libsompi.so
	gcc -Wall -I. -lrt -o mpirun mpirun.c -lsompi -L.

mpi.o:mpi.c
	gcc -fPIC -lrt -c -o mpi.o mpi.c

libsompi.so:mpi.o
	gcc -shared mpi.o -lrt -o libsompi.so

run:mpirun
	./mpirun -np 6 test

test:
	gcc -Wall -I. -lrt -o test test.c -lsompi -L.
