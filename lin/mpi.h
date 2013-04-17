#ifndef MPI_H_
#define MPI_H_

#include "mpi_err.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>       /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/mman.h>
#include <mqueue.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

#if defined(__linux__)

#define PID_FD_NAME "pid_name"
#define SEM_MEM_NAME "sem_mem"
#define SET_ID "sem_set_id"
#define RECV_SEND_MEM "recv_mem_send"
#define BIG_MEM "big_mem"

#define DECLSPEC

#elif defined(_WIN32)

#ifdef EXPORTS
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif

#else
#error "Unknown platform"
#endif

struct mpi_comm{
	int id;
	unsigned int proc_nr;
	int pid[50];
	int shm_fd;
};
typedef struct mpi_comm *MPI_Comm;

extern DECLSPEC struct mpi_comm *mpi_comm_world;
#define MPI_COMM_WORLD (mpi_comm_world)

typedef unsigned char MPI_Datatype;

#define MPI_CHAR	0
#define MPI_INT		1
#define MPI_DOUBLE	2

struct mpi_status {
	int MPI_SOURCE;
	int MPI_TAG;
	int _size;
};

typedef struct mpi_status MPI_Status;

#define MPI_ANY_SOURCE	(0xffffeeee)
#define MPI_ANY_TAG	(0xaaaabbbb)
#define MPI_STATUS_IGNORE ((MPI_Status *)0xabcd1234)

int DECLSPEC MPI_Init(int *argc, char ***argv);
int DECLSPEC MPI_Initialized(int *flag);
int DECLSPEC MPI_Comm_size(MPI_Comm comm, int *size);
int DECLSPEC MPI_Comm_rank(MPI_Comm comm, int *rank);
int DECLSPEC MPI_Finalize();
int DECLSPEC MPI_Finalized(int *flag);

int DECLSPEC MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest,
		      int tag, MPI_Comm comm);

int DECLSPEC MPI_Recv(void *buf, int count, MPI_Datatype datatype,
		      int source, int tag, MPI_Comm comm, MPI_Status *status);


int DECLSPEC MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count);

MPI_Comm info = NULL;
sem_t *set_id;
sem_t *mem_access;
struct mpi_comm *mpi_comm_world;
void* send_recv_mem;
int send_recv_fd;
sem_t **big_mem_access;
#endif
