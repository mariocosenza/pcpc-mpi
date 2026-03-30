#include "mycollective.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void my_broadcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
 
    MPI_Bcast(buffer, count, datatype, root, comm);
    
    return;
}
void my_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
    if (rank == root) {
        MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    } else {
        MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }
    return;
}
void my_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
    if (rank == root) {
        MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    } else {
        MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }
    return;
}