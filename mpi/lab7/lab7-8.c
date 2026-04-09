// Develop an MPI program that, given a matrix of size NxM and P processes,
//  calculates the maximum for each row of the matrix using P processes fairly.
//  Upon completion, the master process writes the maximum of each row to standard output.

#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>

int max(const int arr[], int *max_val, int size)
{
    if (arr == NULL)
    {
        return -1;
    }
    if (size < 1)
    {
        return -1;
    }
    *max_val = arr[0];
    for (int i = 1; i < size; i++)
    {
        if (arr[i] > *max_val)
        {
            *max_val = arr[i];
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int N = 10, M = 10;
    int array[M][N];

    unsigned int seed = (unsigned int)time(NULL);
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            array[i][j] = rand_r(&seed) % 1000;
        }
    }
    
    MPI_Init(&argc, &argv);

    double start;
    int size, rank;
    int s_max;
    int r_max[size];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int recv_array[M / size][N];

    start = MPI_Wtime();
    
    MPI_Scatter(&array, (M / size) * N, MPI_INT, &recv_array,(M / size) * N, MPI_INT, 0, MPI_COMM_WORLD);

    int temp_max =  recv_array[0][0];
    for (int i = 0; i < M /size; i++) {
        max(array[i], &temp_max, N);
        if (temp_max > s_max) {
            s_max = temp_max;
        }
    }

    MPI_Gather(&s_max, 1, MPI_INT, &r_max, size, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        max(r_max, &s_max, size);
        printf("Max= %d\n", s_max);
        fflush(stdout);
    }

    double end = MPI_Wtime();
    double local_elapsed = end - start;
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);
    MPI_Finalize();
    return 0;
}