#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int size, rank, rec_rank, source_rank, send_rank;
    MPI_Request request;
    MPI_Status status, r_status;
    MPI_Init(&argc, &argv);
    double start = MPI_Wtime();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size == 1)
    {
        fprintf(stderr, "At least use two process\n");
        MPI_Finalize();
        return -1;
    }

    if (rank != size - 1)
    {
        send_rank = rank + 1;
    }
    else
    {
        send_rank = 0;
    }

    if (rank == 0)
    {
        source_rank = size - 1;
    }
    else
    {
        source_rank = rank - 1;
    }

    int somma = rank;
    MPI_Isend(&rank, 1, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &request);
    MPI_Recv(&rec_rank, 1, MPI_INT, source_rank, 0, MPI_COMM_WORLD, &r_status);

    MPI_Wait(&request, &status);

    somma += rec_rank;
    int n_rank = rec_rank;
    for (int i = 0; i < size - 2; i++)
    {
        MPI_Isend(&n_rank, 1, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &request);
        MPI_Recv(&rec_rank, 1, MPI_INT, source_rank, 0, MPI_COMM_WORLD, &r_status);

        MPI_Wait(&request, &status);
        somma += rec_rank;
        n_rank = rec_rank;
    }

    printf("Somma: %d\n", somma);
    double end = MPI_Wtime();
    double local_elapsed = end - start;
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);
    double max_time, min_time, sum_time, max_sum;


    MPI_Reduce(&local_elapsed, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_elapsed, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_elapsed, &sum_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&somma, &max_sum, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);   
    fflush(stdout);

    if (rank == 0)
    {
        printf("Max elapsed time: %f seconds\n", max_time);
        printf("Min elapsed time: %f seconds\n", min_time);
        printf("Sum of all times: %f seconds\n", sum_time);
        printf("Max sum: %d\n", max_sum);
    }

    MPI_Finalize();

    return 0;
}