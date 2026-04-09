#include "mycollective.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int my_broadcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            if (i != root) {
                MPI_Send(buffer, count, datatype, i, 0, comm);
            }
        }
    } else {
        MPI_Recv(buffer, count, datatype, root, 0, comm, MPI_STATUS_IGNORE);
    }
    
    return 0;
}



int my_scatter(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int root, void *recv_buff, int recv_count, MPI_Datatype recv_datatype) {
    int rank, size;
    int send_type_size, recv_type_size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Type_size(datatype, &send_type_size);
    MPI_Type_size(recv_datatype, &recv_type_size);

    if (count < size * recv_count) {
        return -1;
    }

    if (recv_count * recv_type_size > recv_count * send_type_size) {
        return -1;
    }
    
    if (rank == root) {
        char *send_bytes = (char *)send_buff;

        for (int i = 0; i < size; i++) {
            if (i != root) {
                MPI_Send(send_bytes + (i * recv_count * send_type_size), recv_count, datatype, i, 0, comm);
            }
        }

        memcpy(recv_buff, send_bytes + (rank * recv_count * send_type_size), (size_t)(recv_count * recv_type_size));
    } else {
        MPI_Recv(recv_buff, recv_count, recv_datatype, root, 0, comm, MPI_STATUS_IGNORE);
    }

    return 0;
}

int my_gather(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int root, void *recv_buff, int recv_count, MPI_Datatype recv_datatype) {
    int rank, size;
    int send_type_size, recv_type_size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Type_size(datatype, &send_type_size);
    MPI_Type_size(recv_datatype, &recv_type_size);
    
    if (count * send_type_size != recv_count * recv_type_size) {
        return -1;
    }

    if (rank != root) {
        MPI_Send(send_buff, count, datatype, root, 0, comm);
    } else {
        char *recv_bytes = (char *)recv_buff;
        for (int i = 0; i < size; i++) {
            if (i != root) {
                MPI_Recv(recv_bytes + (i * recv_count * recv_type_size), recv_count, recv_datatype, i, 0, comm, MPI_STATUS_IGNORE);
            } else {
                memcpy(recv_bytes + (i * recv_count * recv_type_size), send_buff, (size_t)(recv_count * recv_type_size));
            }
        }    
    }

    return 0;
}