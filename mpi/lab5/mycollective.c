#include "mycollective.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int reduce_array(int *send_array, int *recv_array, int size, MY_OP op, int root, MPI_Comm comm) {
    MPI_Op mpi_op = MPI_MIN;
    if (op == MAX) {
        mpi_op = MPI_MAX;
    }
    return MPI_Reduce(send_array, recv_array, size, MPI_INT, mpi_op, root, comm);
}

int ireduce_array(int *send_array, int *recv_array, int size, MY_OP op, int root, MPI_Comm comm, MPI_Request *request) {
    MPI_Op mpi_op = MPI_MIN;
    if (op == MAX) {
        mpi_op = MPI_MAX;
    }
    return MPI_Ireduce(send_array, recv_array, size, MPI_INT, mpi_op, root, comm, request);
}

int wait_array(MPI_Request *requests, int size, MPI_Status *status) {
    return MPI_Waitall(size, requests, status);
}

MPI_Request *ibcast(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag) //same size
{
    int size;
    MPI_Comm_size(comm, &size);
    MPI_Request request_arr[size];
    for (int i = 0; i < size; i++) {
        MPI_Isend(send_buff, count, datatype, i, tag, comm, &request_arr[i]);
    }
    return request_arr;
}

MPI_Request *iscatter(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag) //same size
{
    int size;
    MPI_Comm_size(comm, &size);
    MPI_Request request_arr[size];

    for (int i = 0; i < size; i++) {
        void* local_send[count];
        int t = 0;
        for (int j = i; j < i + count; j++) {
            local_send[t] = &send_buff[j];
            t++;
        }
        MPI_Isend(local_send, count, datatype, i, tag, comm, &request_arr[i]);
    }
    return request_arr;
}

MPI_Request *igather(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag, int root) //same size
{
    int size;
    MPI_Comm_size(comm, &size);
    MPI_Request request_arr[size];
    for (int i = 0; i < size; i++) {
        MPI_Isend(send_buff, count, datatype, root, tag, comm, &request_arr[i]);
    }
    return request_arr;
}




