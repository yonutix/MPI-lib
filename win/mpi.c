#include "mpi.h"
struct mpi_comm *mpi_comm_world = NULL;

int MPI_Init(int *argc, char ***argv)
{
  HANDLE hMapFile;
  HANDLE h_buffer;
  MPI_Comm pBuf;
  HANDLE info_sem;
  char name[50];
  unsigned int i;
  if(info != NULL || info == (MPI_Comm)0x1){
    return MPI_ERR_OTHER;
  }

  hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS,
    FALSE,
    (LPCTSTR)INFO_SH_MEM);

  info = (MPI_Comm) MapViewOfFile(hMapFile,
    FILE_MAP_ALL_ACCESS,
    0,
    0,
    sizeof(*info));

  info_sem = OpenSemaphore(NULL,
    FALSE,
    (LPCTSTR)INFO_SEM);
  WaitForSingleObject(info_sem, INFINITE);

  for(i = 0; i < info->proc_nr; ++i){
    if(info->pid[i] == 0){
      info->pid[i] = GetCurrentProcessId();
      break;
    }
  }

  ReleaseSemaphore(info_sem, 1, NULL);
  CloseHandle(hMapFile);
  CloseHandle(info_sem);

  //Open shared momory for each buffer
  h_buffer = OpenFileMapping(FILE_MAP_ALL_ACCESS,
    FALSE,
    (LPCTSTR)BUFFER_MEM);  

  mpi_buffer = (void*)MapViewOfFile(h_buffer, 
    FILE_MAP_ALL_ACCESS,
    0,
    0,
    (SIZE_T)2*PAGESIZE*info->proc_nr);

  //Semaphores for each memory of each process
  send_sem = malloc(sizeof(HANDLE) * info->proc_nr);
  for(i = 0; i < info->proc_nr; ++i){
    sprintf(name, "name_%d", i);
    send_sem[i] = OpenSemaphore(NULL,
      FALSE,
      (LPCTSTR)name);
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
  int i;
  if(info == NULL || info == (MPI_Comm)0x1){
    return MPI_ERR_OTHER;
  }
  for(i = 0; i < info->proc_nr; ++i){
    CloseHandle(send_sem[i]);
  }
  UnmapViewOfFile(info);
  info = (MPI_Comm)0x1;
  UnmapViewOfFile(mpi_buffer);
  
  return MPI_SUCCESS;
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
  *size = info->proc_nr;
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  int i;
  if(!info || info == (MPI_Comm)0x1){
    return MPI_ERR_OTHER;
  }
  for(i = 0; i < info->proc_nr; ++i){
    if(info->pid[i] == GetCurrentProcessId()){
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
  return 0;
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  int rank, offset;
  if(!info || info == (MPI_Comm)0x1){
    return MPI_ERR_OTHER;
  }
  
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	offset = (PAGESIZE * dest)/sizeof(int);
  
  WaitForSingleObject(send_sem[dest], INFINITE);
	while(((int*)mpi_buffer)[offset + 0] != 0);
	((int*)mpi_buffer)[offset + 0] = 2;
  ReleaseSemaphore(send_sem[dest], 1, NULL);
	((int*)mpi_buffer)[offset + 1] = rank;
	((int*)mpi_buffer)[offset + 2] = dest;
	((int*)mpi_buffer)[offset + 3] = tag;
	((int*)mpi_buffer)[offset + 4] = count;

	memcpy(&(((int*)mpi_buffer)[offset + 5]), buf, GetSize(datatype) * count);
	((int*)mpi_buffer)[offset + 0] = 1;
	while(((int*)mpi_buffer)[offset + 0] != 3);
	((int*)mpi_buffer)[offset + 0] = 0;
  
	return MPI_SUCCESS;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int rank;
  int offset;
  if(!info || info == (MPI_Comm)0x1){
    return MPI_ERR_OTHER;
  }
  
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	offset = (PAGESIZE * rank)/sizeof(int);
	begin:

	while(((int*)mpi_buffer)[offset + 0] != 1);
	if(source != ((int*)mpi_buffer)[offset + 1] && source != MPI_ANY_SOURCE)
		goto begin;
	if(tag != ((int*)mpi_buffer)[offset + 3] && tag != MPI_ANY_TAG)
		goto begin;

	if(status != MPI_STATUS_IGNORE){
				status->MPI_TAG = ((int*)mpi_buffer)[offset + 3];
				status->MPI_SOURCE = ((int*)mpi_buffer)[offset + 1];
				status->_size = ((int*)mpi_buffer)[offset + 4];
			}
	memcpy(buf, ((int*)(mpi_buffer)) + offset + 5, GetSize(datatype) * count);
	((int*)mpi_buffer)[offset + 0] = 3;
  
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