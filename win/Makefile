CFLAGS = /nologo /W3 /DDLL_IMPORTS

build: mpirun.obj sompi.lib
	link /nologo /out:mpirun.exe mpirun.obj

sompi.lib: sompi.dll

sompi.dll: mpi.obj
	link /nologo /dll /out:sompi.dll /implib:sompi.lib mpi.obj

mpi.obj: mpi.c
	cl $(CFLAGS) /c mpi.c

mpirun.obj : mpirun.c
	cl $(CFLAGS) /c mpirun.c

clean:
	del *.obj mpirun.exe sompi.lib sompi.dll
