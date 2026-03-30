#include<mpi.h>

typedef enum {
    MIN,
    MAX,
} MY_OP;


int reduce_array(int *send_array, int *recv_array, int size, MY_OP op, int root, MPI_Comm comm);
int ireduce_array(int *send_array, int *recv_array, int size, MY_OP op, int root, MPI_Comm comm, MPI_Request *request);
int wait_array(MPI_Request *requests, int size, MPI_Status *status);

MPI_Request *ibcast(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag);
MPI_Request *iscatter(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag);
MPI_Request *igather(void *send_buff, int count, MPI_Datatype datatype, MPI_Comm comm, int tag, int root);