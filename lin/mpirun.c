#include "mpi.h"

void* InitPIDs()
{	/* memory descriptor */
	int rc;
 	void* mem;

 	mem_access = sem_open(SEM_MEM_NAME, O_CREAT, 0644, info->proc_nr); 
	DIE(mem_access == SEM_FAILED, "sem_open failed");

	set_id = sem_open(SET_ID, O_CREAT, 0644, 1); 
	DIE(set_id == SEM_FAILED, "sem_open failed");
	/* create shm */
	info->shm_fd = shm_open(PID_FD_NAME, O_CREAT | O_RDWR, 0644);
	DIE(info->shm_fd == -1, "shm_open");
 
	/* resize shm to fit our needs */
	rc = ftruncate(info->shm_fd, getpagesize());
	DIE(rc == -1, "ftruncate");
 
	//printf("gigel %d\n", getpagesize());

	mem = mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, info->shm_fd, 0);
	DIE(mem == MAP_FAILED, "mmap");
 	memcpy(mem, info, sizeof(*info));
	return mem;
}

void* InitSendRecvMem()
{
	int rc;
	send_recv_fd = shm_open(RECV_SEND_MEM, O_CREAT | O_RDWR, 0644);

 	rc = ftruncate(send_recv_fd, 2 * info->proc_nr * getpagesize());
 	DIE(rc == -1, "ftruncate");

 	send_recv_mem = mmap(0, 2 * info->proc_nr * getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, send_recv_fd, 0);
 	DIE(send_recv_mem == MAP_FAILED, "mmap");
 	int i;
 	memset(send_recv_mem, 0x0, 2 * info->proc_nr * getpagesize());
 	big_mem_access = malloc(sizeof(sem_t*) * info->proc_nr);

 	for(i = 0; i < info->proc_nr; ++i){
 		char name[40];
 		sprintf(name, "sem_name_%d", i);
	 	big_mem_access[i] = sem_open(name, O_CREAT, 0644, 1); 
		DIE(big_mem_access[i] == SEM_FAILED, "sem_open failed");
	}

	return send_recv_mem;
}

void DestroySendRecvMem()
{
	int rc;

	/* unmap shm */
	rc = munmap(send_recv_mem, 2 * info->proc_nr * getpagesize());
	DIE(rc == -1, "munmap");
 
	/* close descriptor */
	rc = close(send_recv_fd);
	DIE(rc == -1, "close");
 
	rc = shm_unlink(RECV_SEND_MEM);
	DIE(rc == -1, "unlink");
	int i;

	for(i = 0; i < info->proc_nr; ++i){
 		char name[40];
 		sprintf(name, "sem_name_%d", i);

		rc = sem_close(big_mem_access[i]);
		DIE(rc == -1, "sem_close");
	 
		rc = sem_unlink(name);
		DIE(rc == -1, "sem_unlink");
	}
	free(big_mem_access);

}

void DestroyPIDs(void * mem)
{
	int rc;

	/* unmap shm */
	rc = munmap(mem, getpagesize());
	DIE(rc == -1, "munmap");
 
	/* close descriptor */
	rc = close(info->shm_fd);
	DIE(rc == -1, "close");
 
	rc = shm_unlink(PID_FD_NAME);
	DIE(rc == -1, "unlink");

	rc = sem_close(mem_access);
	DIE(rc == -1, "sem_close");
 
	rc = sem_unlink(SEM_MEM_NAME);
	DIE(rc == -1, "sem_unlink");

	rc = sem_close(set_id);
	DIE(rc == -1, "sem_close");
 
	rc = sem_unlink(SET_ID);
	DIE(rc == -1, "sem_unlink");
}


int main(int argc, char** argv)
{
	int i;
	if(argc < 4){
		printf("Usage:%s -np <process number> <exe file>\n", argv[0]);
		return 0;
	}
	info = malloc(sizeof(*info));
	info->proc_nr = atoi(argv[2]);
	int pid;
	void* mem = InitPIDs();
	void* send_recv_mem = InitSendRecvMem();
	for(i = 0; i < info->proc_nr; ++i){
		pid = fork();
		if(pid == 0){
			execv(argv[3], argv+4);
		}
	}
	while(wait(NULL) > 0);
	DestroySendRecvMem(send_recv_mem);
	DestroyPIDs(mem);
	free(info);
	//printf("Root end\n");
	return 0;
}