#include<stdio.h>
#include<mpi.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char **argv) {
    int rank, size;
    MPI_Status status;
    int start_process_n = 1;
    int s = 11000, start_number = 1;
   

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(rank);

    if (start_process_n < 0 || start_process_n > (size - 1)) {
        MPI_Finalize();
        return -1;
    }

    if (rank == start_process_n) {
        MPI_Send(&start_number, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    }

    int num, count = 0, avg = 0;
    for (int i = 0; i < 10; i++) {
    for(;;) {
        if (num > s) {
            break;
        }
        count++;
        if (rank != (size - 1)) {
            if (rank != 0) {
                MPI_Recv(&num, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(&num , 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, &status);
            }
            num += rand() % 101;
            MPI_Send(&num, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        } else {
            MPI_Recv(&num, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
            num += rand() % 101;
            MPI_Send(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    if (avg == 0) {
        avg = count;
    } else {
        avg = (avg + count) / 2;
    }
    if (num > s) {
        break;
    }
    count = 0;
}

    if (rank == 0) {
        printf("Round %d with num %d", avg, num);
    }

    MPI_Finalize();
    return 0;
}