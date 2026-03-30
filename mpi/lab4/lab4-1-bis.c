#include<stdio.h>
#include<mpi.h>
#include<string.h>
#include "mycollective.h"

int main(const int argc, char *argv[]) {
    int pcount, rank;
    int tag = 0;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &pcount);

    double start, end;
    MPI_Barrier(MPI_COMM_WORLD); /* tutti i processi sono inizializzati */
    start = MPI_Wtime();

    if (rank == 0) {
        int integer_to_send = 2;
        int integer_recived;
        char string[100];
        MPI_Send(&integer_to_send, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
        MPI_Recv(&integer_recived, 1, MPI_INT, 1, tag, MPI_COMM_WORLD, &status);
        printf("Rank 0: %d - %d\n", integer_to_send, integer_recived);
        scanf("%s", &string);
        MPI_Send(&string, strlen(string) + 1, MPI_CHAR, 1, tag, MPI_COMM_WORLD);
    } else if(rank == 1) {
        int integer_to_send = 4;
        int integer_recived;
        MPI_Recv(&integer_recived, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
        MPI_Send(&integer_to_send, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
        printf("Rank 1: %d - %d\n", integer_to_send, integer_recived);
        char string[100];
        MPI_Recv(&string, 100, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
        printf("%s", string);
    }

    int recv[3];
    if (rank == 0) {
        int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    
        my_broadcast(&arr,  sizeof(arr) / sizeof(arr[0]), MPI_INT, 0, MPI_COMM_WORLD);
    
        my_gather(&arr, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
        for (int i = 0; i < pcount - 1; i++){
            printf("Intero da rank %d: %d\n", i, arr[i]);
        }
        my_scatter(&arr, 3, MPI_INT, recv, 3, MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        MPI_Send(&rank, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
        my_scatter(NULL, 0, MPI_INT, recv, 3, MPI_INT, 0, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD); /* tutti i processi hanno terminato */
    end = MPI_Wtime();  
    if (rank == 0) { /* Master node scrive su stdout il tempo o su file */
        printf("Time in ms = %f\n", end-start);
    }
    MPI_Finalize();

    return 0;
}