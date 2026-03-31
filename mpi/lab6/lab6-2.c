#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv) {
    int rank, size;
    int somma_totale = 0;

    MPI_Init(&argc, &argv);
    double start = MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    MPI_Allreduce(&rank, &somma_totale, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    printf("Somma: %d\n", somma_totale);

    double end = MPI_Wtime();
    double local_elapsed = end - start;

    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);
    double max_time, min_time, sum_time, max_sum;

    fflush(stdout);
    MPI_Reduce(&local_elapsed, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_elapsed, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_elapsed, &sum_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&somma_totale, &max_sum, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);   
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