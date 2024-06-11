#ifndef MPI_H_ABI
#define MPI_H_ABI

enum {
    MPI_SUCCESS                        =  0,
};

int MPI_Init(int *argc, char ***argv);
int PMPI_Init(int *argc, char ***argv);
double MPI_Wtick(void);
double MPI_Wtime(void);
double PMPI_Wtick(void);
double PMPI_Wtime(void);

#endif  /* MPI_H_ABI */
