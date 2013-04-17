#include "mpi.h"


int main(int argc, char** argv)
{
  unsigned int i;
  BOOL ret;
  PROCESS_INFORMATION *processInfo;
  SECURITY_ATTRIBUTES sa;
  STARTUPINFO startupInfo;
  HANDLE fileMapping;
  HANDLE info_sem;
  HANDLE h_buffer;
  char name[50];
  ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);

  ZeroMemory(&startupInfo, sizeof(startupInfo));

  if(argc < 4){
    printf("Usage:%s -np <process number> <exe file>\n", argv[0]);
    return 0;
  }

  //Mapare memorie informatii
  fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    sizeof(struct mpi_comm),
    (LPCTSTR)INFO_SH_MEM);

  info = (MPI_Comm)MapViewOfFile(fileMapping, 
    FILE_MAP_ALL_ACCESS,
    0,
    0,
    (SIZE_T)sizeof(*info));

  info->proc_nr = atoi(argv[2]);

  //Creare semafor informatii
  info_sem = CreateSemaphore(NULL,
    1,
    100,
    (LPCTSTR)INFO_SEM);

  //Maparea memoriei proceselor
  h_buffer = CreateFileMapping(INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    2*PAGESIZE*info->proc_nr,
    (LPCTSTR)BUFFER_MEM);

  mpi_buffer = (void*)MapViewOfFile(h_buffer, 
    FILE_MAP_ALL_ACCESS,
    0,
    0,
    (SIZE_T)2*PAGESIZE*info->proc_nr);


  //Creaza semafoarele pentru memoriile proceselor
  send_sem = malloc(sizeof(HANDLE) * info->proc_nr);
  for(i = 0; i < info->proc_nr; ++i){
    sprintf(name, "name_%d", i);
    send_sem[i] = CreateSemaphore(NULL,
      1,
      100,
      (LPCTSTR)name);
  }

  //Crearea proceselor
  processInfo = malloc(sizeof(PROCESS_INFORMATION) * info->proc_nr);
  for(i = 0; i < info->proc_nr; ++i){
    ZeroMemory(&processInfo[i], sizeof(processInfo));
    ret = CreateProcess(NULL,
      (LPSTR)argv[3],
      (LPSECURITY_ATTRIBUTES)&sa,
      NULL,
      TRUE,
      NORMAL_PRIORITY_CLASS,
      NULL,
      NULL,
      &startupInfo,
      &processInfo[i]);
  }

  for(i = 0; i < info->proc_nr; ++i){
    WaitForSingleObject(processInfo[i].hProcess, INFINITE);
    CloseHandle(processInfo[i].hProcess);
    CloseHandle(processInfo[i].hThread);
  }
  free(processInfo);

  for(i = 0; i < info->proc_nr; ++i){
    CloseHandle(send_sem[i]);
  }
  UnmapViewOfFile(info);
  CloseHandle(fileMapping);
  UnmapViewOfFile(mpi_buffer);
  CloseHandle(h_buffer);
  CloseHandle(info_sem);
  return 0;
}