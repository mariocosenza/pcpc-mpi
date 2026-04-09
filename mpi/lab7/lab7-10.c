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

int max(const int arr[], int *max_val, int size) {
    if (arr == NULL || size < 1) return -1;
    *max_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > *max_val) {
            *max_val = arr[i];
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int N = 12, M = 12; 
    int size, rank;
    double start, end, local_elapsed;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (N % size != 0 || M % size != 0) {
        if (rank == 0) {
            printf("Error: Matrix dimensions N (%d) and M (%d) must be divisible by process count (%d)\n", N, M, size);
        }
        MPI_Finalize();
        return 1;
    }

    int rows_per_proc = N / size;
    int cols_per_proc = M / size;
    int array[N][M];

    if (rank == 0) {
        unsigned int seed = (unsigned int)time(NULL);
        printf("--- Original Matrix ---\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                array[i][j] = rand_r(&seed) % 1000;
                printf("%4d ", array[i][j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    start = MPI_Wtime();

    int recv_rows[rows_per_proc][M];
    
    MPI_Scatter(array, rows_per_proc * M, MPI_INT, recv_rows, rows_per_proc * M, MPI_INT, 0, MPI_COMM_WORLD);

    int local_maxes[rows_per_proc];
    for (int i = 0; i < rows_per_proc; i++) {
        max(recv_rows[i], &local_maxes[i], M);
    }

    int global_maxes[N];
    MPI_Gather(local_maxes, rows_per_proc, MPI_INT, global_maxes, rows_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Datatype column_type, resized_column;
    MPI_Type_vector(N, 1, M, MPI_INT, &column_type);
    MPI_Type_create_resized(column_type, 0, sizeof(int), &resized_column);
    MPI_Type_commit(&resized_column);

    int recv_cols_buffer[cols_per_proc * N];
    MPI_Scatter(array, cols_per_proc, resized_column, recv_cols_buffer, cols_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    int local_mins[cols_per_proc];
    int temp_col[N];

    for (int j = 0; j < cols_per_proc; j++) {
        for (int i = 0; i < N; i++) {
            temp_col[i] = recv_cols_buffer[i * cols_per_proc + j];
        }
        min(temp_col, &local_mins[j], N);
    }

    int global_mins[M];
    MPI_Gather(local_mins, cols_per_proc, MPI_INT, global_mins, cols_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    end = MPI_Wtime();
    local_elapsed = end - start;

    if (rank == 0) {
        printf("--- Row Maximums ---\n");
        for (int i = 0; i < N; i++) {
            printf("Row %d Max: %d\n", i, global_maxes[i]);
        }
        
        printf("\n--- Column Minimums ---\n");
        for (int j = 0; j < M; j++) {
            printf("Column %d Min: %d\n", j, global_mins[j]);
        }
        printf("\n");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    fflush(stdout);
    printf("Process %d: Elapsed time = %f seconds\n", rank, local_elapsed);

    MPI_Type_free(&resized_column);
    MPI_Type_free(&column_type);
    MPI_Finalize();
    
    return 0;
}