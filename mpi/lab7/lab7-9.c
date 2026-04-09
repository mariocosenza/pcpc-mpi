#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>

int min(const int arr[], int *min_val, int size) {
    if (arr == NULL || size < 1) return -1;
    *min_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < *min_val) {
            *min_val = arr[i];
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int N = 10, M = 12;
    int size, rank;
    double start;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (M % size != 0) {
        if (rank == 0) printf("M deve essere divisibile per size\n");
        MPI_Finalize();
        return 1;
    }

    int k = M / size;
    int array[N][M];

    if (rank == 0) {
        unsigned int seed = (unsigned int)time(NULL);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                array[i][j] = rand_r(&seed) % 1000;
            }
        }
    }

    MPI_Datatype column_type, resized_column;
    MPI_Type_vector(N, 1, M, MPI_INT, &column_type);
    MPI_Type_create_resized(column_type, 0, sizeof(int), &resized_column);
    MPI_Type_commit(&resized_column);

    start = MPI_Wtime();

    int recv_buffer[k * N];
    MPI_Scatter(array, k, resized_column, recv_buffer, k * N, MPI_INT, 0, MPI_COMM_WORLD);

    int local_mins[k];
    int temp_col[N];

    for (int j = 0; j < k; j++) {
        for (int i = 0; i < N; i++) {
            temp_col[i] = recv_buffer[i * k + j];
        }
        min(temp_col, &local_mins[j], N);
    }

    int global_mins[M];
    MPI_Gather(local_mins, k, MPI_INT, global_mins, k, MPI_INT, 0, MPI_COMM_WORLD);

    double end = MPI_Wtime();
    double local_elapsed = end - start;

    if (rank == 0) {
        for (int j = 0; j < M; j++) {
            printf("Colonna %d: %d\n", j, global_mins[j]);
        }
    }

    fflush(stdout);
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);

    MPI_Type_free(&resized_column);
    MPI_Finalize();
    return 0;
}
