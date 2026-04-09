#include "mycollective.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // my broadcast test with wtime
    int num_elements = 1000000;
    double *array = calloc(num_elements, sizeof(double));
    double start = MPI_Wtime();
    if (rank == 0)
    {
        unsigned int seed = (unsigned int)time(NULL);
        for (int i = 0; i < num_elements; i++)
        {
            array[i] = rand_r(&seed);
        }
    }
    my_broadcast(array, num_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double end = MPI_Wtime();
    double local_elapsed = end - start;
    printf("Process %d: Elapsed time for my_broadcast = %f seconds\n", rank, local_elapsed);
    free(array);

    // my scatter test with wtime
    int send_count = 1000000;
    double *send_array = NULL;
    double *recv_array = calloc(send_count / size, sizeof(double));
    if (rank == 0)
    {
        send_array = calloc(send_count, sizeof(double));
        unsigned int seed = (unsigned int)time(NULL);
        for (int i = 0; i < send_count; i++)
        {
            send_array[i] = rand_r(&seed);
        }
    }
    start = MPI_Wtime();
    my_scatter(send_array, send_count, MPI_DOUBLE, MPI_COMM_WORLD, 0, recv_array, send_count / size, MPI_DOUBLE);
    end = MPI_Wtime();
    local_elapsed = end - start;
    printf("Process %d: Elapsed time for my_scatter = %f seconds\n", rank, local_elapsed);
    free(send_array);
    free(recv_array);

    //my gather test with wtime
    int recv_count = 1000000;
    double *send_gather_array = calloc(recv_count / size, sizeof(double));
    double *recv_gather_array = NULL;
    if (rank == 0) {
        recv_gather_array = calloc(recv_count, sizeof(double));
    }
    unsigned int seed = (unsigned int)time(NULL);
    for (int i = 0; i < recv_count / size; i++) {
        send_gather_array[i] = rand_r(&seed);
    }
    start = MPI_Wtime();
    my_gather(send_gather_array, recv_count / size, MPI_DOUBLE, MPI_COMM_WORLD, 0, recv_gather_array, recv_count / size, MPI_DOUBLE);
    end = MPI_Wtime();
    local_elapsed = end - start;
    printf("Process %d: Elapsed time for my_gather = %f seconds\n",
              rank, local_elapsed);
    free(send_gather_array);
    free(recv_gather_array);

    MPI_Finalize();
    return 0;
}