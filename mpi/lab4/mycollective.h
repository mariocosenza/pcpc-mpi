#ifndef MYCOLLECTIVE_H
#define MYCOLLECTIVE_H

#include <mpi.h>

void my_broadcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
void my_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
void my_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);

#endif /* MYCOLLECTIVE_H */