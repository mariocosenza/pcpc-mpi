#include <mpi.h>

int my_broadcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int my_scatter(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int root, void *recv_buff, int recv_count, MPI_Datatype recv_datatype);
int my_gather(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int root, void *recv_buff, int recv_count, MPI_Datatype recv_datatype);