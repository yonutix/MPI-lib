#include "mpi.h"

int MPI_Init(int *argc, char ***argv)
{
	int i;
	if(info != NULL || info == (MPI_Comm)0x1){
		//printf("prea intializat\n");
		return MPI_ERR_OTHER;
	}
	else{
		mem_access = sem_open(SEM_MEM_NAME, 0); 
		DIE(mem_access == SEM_FAILED, "sem_open failed");


		info = malloc(sizeof(*info));
		info->shm_fd = shm_open(PID_FD_NAME, O_RDWR, 0644);
		void* mem = mmap(0, getpagesize(), PROT_WRITE | PROT_READ, MAP_SHARED, info->shm_fd, 0);

		DIE(mem == MAP_FAILED, "mmap");

		free(info);
		info = mem;
		//printf("ss %d\n", info->proc_nr);


		set_id = sem_open(SET_ID, 0); 
		DIE(set_id == SEM_FAILED, "sem_open failed");

		int pvalue;
		sem_getvalue(set_id, &pvalue);
		sem_wait(set_id);
		for(i = 0; i < info->proc_nr; ++i){
			if(info->pid[i] == 0){
				info->pid[i] = getpid();
				sem_getvalue(set_id, &pvalue);
				info->id = i;
				sem_post(set_id);
				sem_getvalue(set_id, &pvalue);
				break;
			}
		}		
	}

	send_recv_fd = shm_open(RECV_SEND_MEM, O_RDWR, 0644);
	send_recv_mem = mmap(0, 2 * info->proc_nr * getpagesize(), PROT_WRITE | PROT_READ, MAP_SHARED, send_recv_fd, 0);

	big_mem_access = malloc(sizeof(sem_t*) * info->proc_nr);
	for(i = 0; i < info->proc_nr; ++i){
 		char name[40];
 		sprintf(name, "sem_name_%d", i);
	 	big_mem_access[i] = sem_open(name, 0); 
		DIE(big_mem_access[i] == SEM_FAILED, "sem_open failed");
	}


	return MPI_SUCCESS;
}

int MPI_Initialized(int *flag)
{
	if(!info){
		*flag = 0;
	}
	else{
		*flag = 1;
	}
	return MPI_SUCCESS;
}

int MPI_Finalize()
{
	int rc;
	if(info != NULL && info != (MPI_Comm)0x1){
		//free(info);
		info = (MPI_Comm)0x1;
		rc = sem_close(mem_access);
		DIE(rc == -1, "sem_close");
		free(big_mem_access);
		//rc = sem_close(set_id);
		//DIE(rc == -1, "sem_close");
		
		return MPI_SUCCESS;
	}
	else{
		return MPI_ERR_OTHER;
	}
}

int MPI_Finalized(int *flag)
{
	if(info == (MPI_Comm)0x1){
		*flag = 1;
	}
	else{
		*flag = 0;
	}
	return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
	if(!info || info == (MPI_Comm)0x1){
		return MPI_ERR_OTHER;
	}
	//printf("%d\n", info->proc_nr);
	*size = info->proc_nr;
	return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	if(!info || info == (MPI_Comm)0x1){
		return MPI_ERR_OTHER;
	}
	int i;
	for(i = 0; i < info->proc_nr; ++i){
		if(info->pid[i] == getpid()){
			*rank = i;
			break;
		}
	}
	return MPI_SUCCESS;
}

int GetSize(MPI_Datatype datatype)
{
	switch(datatype){
		case MPI_CHAR:
			return sizeof(char);
			break;
		case MPI_INT:
			return sizeof(int);
			break;
		case MPI_DOUBLE:
			return sizeof(double);
			break;
	}
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	if(!info || info == (MPI_Comm)0x1){
		return MPI_ERR_OTHER;
	}
	int rank, offset;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	offset = (getpagesize() * dest)/sizeof(int);

	sem_wait(big_mem_access[dest]);
	while(((int*)send_recv_mem)[offset + 0] != 0);
	((int*)send_recv_mem)[offset + 0] = 2;
	sem_post(big_mem_access[dest]);
	((int*)send_recv_mem)[offset + 1] = rank;
	((int*)send_recv_mem)[offset + 2] = dest;
	((int*)send_recv_mem)[offset + 3] = tag;
	((int*)send_recv_mem)[offset + 4] = count;

	memcpy(&(((int*)send_recv_mem)[offset + 5]), buf, GetSize(datatype) * count);
	((int*)send_recv_mem)[offset + 0] = 1;
	while(((int*)send_recv_mem)[offset + 0] != 3);
	((int*)send_recv_mem)[offset + 0] = 0;
	return MPI_SUCCESS;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	if(!info || info == (MPI_Comm)0x1){
		return MPI_ERR_OTHER;
	}
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int offset = (getpagesize() * rank)/sizeof(int);
	begin:

	while(((int*)send_recv_mem)[offset + 0] != 1);
	if(source != ((int*)send_recv_mem)[offset + 1] && source != MPI_ANY_SOURCE)
		goto begin;
	if(tag != ((int*)send_recv_mem)[offset + 3] && tag != MPI_ANY_TAG)
		goto begin;

	if(status != MPI_STATUS_IGNORE){
				status->MPI_TAG = ((int*)send_recv_mem)[offset + 3];
				status->MPI_SOURCE = ((int*)send_recv_mem)[offset + 1];
				status->_size = ((int*)send_recv_mem)[offset + 4];
			}
	memcpy(buf, ((int*)(send_recv_mem)) + offset + 5, GetSize(datatype) * count);
	((int*)send_recv_mem)[offset + 0] = 3;

	return MPI_SUCCESS;
}


int MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count)
{
	if(!info || info == (MPI_Comm)0x1){
		return MPI_ERR_OTHER;
	}
	*count = status->_size;
	return MPI_SUCCESS;
}