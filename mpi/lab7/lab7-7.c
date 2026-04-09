#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int size, rank;
    const int N = 1000;
    int array[N]; 
    MPI_Init(&argc, &argv);
    srand(rank);
    if (rank == 0)
    {
        for (int i = 0; i < N; i++)
        {
            array[i] = rand() % 1000;
        }
    }
    double start = MPI_Wtime();
  
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int partial_array[N / size];

    MPI_Scatter(array, N / size, MPI_INT, partial_array, N / size, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < N / size; i++) {
        if (i == 0 && rank == 0) {
            partial_array[0] = array[N-1] + array[0] + array[1];
        } else if (rank == size - 1 && i == (N / size) - 1) {
            partial_array[N-1] = partial_array[N-2] + partial_array[N-1] + array[0];
        } else {
            partial_array[i] = partial_array[i-1] + partial_array[i] + partial_array[i+1];
        }
    }

    MPI_Gather(partial_array, N / size, MPI_INT, array, N / size, MPI_INT, 0, MPI_COMM_WORLD);

    double end = MPI_Wtime();
    double local_elapsed = end - start;
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);
    if (rank == 0) {
        printf("Result: ");
        for (int i = 0; i < N; i++) {
            printf("%d ", array[i]);
        }
        printf("\n");
    }
    MPI_Finalize();
    return 0;
}