//Calculate the maximum and minimum of arrays of integer values 
//using P processes and blocking and non blocking communication operations.

#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>

int min(const int arr[], int *min_val, int size) {
    if (arr == NULL) {
        return -1;
    }
    if (size < 1) {
        return -1;
    }
    *min_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < *min_val) {
            *min_val = arr[i];
        }
    }
    return 0;
}

int max(const int arr[], int *max_val, int size) {
    if (arr == NULL) {
        return -1;
    }
    if (size < 1) {
        return -1;
    }
    *max_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > *max_val) {
            *max_val = arr[i];
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int N = 14;
    int arr[] = {
        1, 2, 4, 5, 6, 7, 8, 9, 99, 100, 101, 201, 301, 401
    };

    int size, rank;
    MPI_Status status;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

  
    int sum = 0;
    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) {
        sendcounts[i] = (N / size) + (i < (N % size) ? 1 : 0);
        displs[i] = sum;
        sum += sendcounts[i];
    }


    int local_n = sendcounts[rank];
    int *recv_buffer = malloc(local_n * sizeof(int));


    MPI_Request request;
    MPI_Iscatterv(arr, sendcounts, displs, MPI_INT, 
                  recv_buffer, local_n, MPI_INT, 
                  0, MPI_COMM_WORLD, &request);

  
    MPI_Wait(&request, MPI_STATUS_IGNORE);

    int min_l, max_l;
    min(recv_buffer, &min_l, local_n);
    max(recv_buffer, &max_l, local_n);

    int *all_mins = NULL;
    int *all_maxs = NULL;

    if (rank == 0) {
        all_mins = malloc(size * sizeof(int));
        all_maxs = malloc(size * sizeof(int));
    }


    MPI_Gather(&min_l, 1, MPI_INT, all_mins, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&max_l, 1, MPI_INT, all_maxs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    

    if (rank == 0) {
        int min_f, max_f;
        min(all_mins, &min_f, size);
        max(all_maxs, &max_f, size);
        printf("Min array: %d | Max array: %d\n", min_f, max_f);
    }

    MPI_Finalize();


    return 0;
}