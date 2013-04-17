#include <stdio.h>
#include "mpi.h"



int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);
	printf("Start\n");
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(rank == 0){
		printf("Process 0 started\n");
		int a = 5;
		MPI_Send(&a, 1, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD);
	}
	if(rank == 1){
		printf("Process 1 started\n");
		int b;
		MPI_Status status;
		MPI_Recv(&b, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("%d\n", b);
	}
	MPI_Finalize();
	return 0;
}