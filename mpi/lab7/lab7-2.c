#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int num_elements = 20000000;
    int size, rank;
    double *array = calloc(num_elements, sizeof(double));

    MPI_Init(&argc, &argv);

    double start;
    unsigned int seed = (unsigned int)time(NULL);
    if (rank == 0)
    {
        for (int i = 0; i < num_elements; i++)
        {
            array[i] = rand_r(&seed);
        }
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    start = MPI_Wtime();
    double *local_array = calloc(num_elements / size, sizeof(double));
    MPI_Scatter(array, num_elements / size, MPI_DOUBLE, local_array, num_elements / size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double end = MPI_Wtime();
    double local_elapsed = end - start;
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);
    MPI_Finalize();
    return 0;
}