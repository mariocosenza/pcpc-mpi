#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>
//#include "lab8-node0.h"

#define NO_NEIGHBOR MPI_PROC_NULL
#define ALIVE 'L'
#define DEAD 'D'

inline char** play_game(char **matrix, const uint8_t n, const uint8_t m) {
    uint_fast8_t c_row, c_col, live; 
  
    c_row = (uint_fast8_t) n / 2;
    c_col = (uint_fast8_t) m / 2;


    for (uint8_t i = 0; i < n; i++) {
        for (uint8_t j = 0; j < m; j++) {
            if (i == c_row && j == c_col) {
                continue;
            }

            if (matrix[i][j] == ALIVE) {
                live++;
            } 
        }
    }

    if (live < 2 || live > 3) {
        matrix[c_row][c_col] = DEAD;
    } else if (live == 3) {
        matrix[c_row][c_col] = ALIVE;
    }
    

    return matrix;
}

int main(int argc, char **argv) {

    const uint8_t N = 10, M = 20;
    const int ndims = 2;
    MPI_Comm comm_grid;
    int dims[] = {0, 0};
    int size, rank, num_slaves, color;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Dims_create(size - 1, ndims, dims);

    num_slaves = size - 1;

    if (num_slaves > N * M) {
        fprintf(stderr, "Using more process than the matrix size\n");
    }


    int is_excessive = 0;
    if (num_slaves > (N * M) || dims[0] > N || dims[1] > M || num_slaves == 0) {
        is_excessive = 1;
    }

    if (rank == 0) {
        color = 0;
    } else if (is_excessive) {
        color = MPI_UNDEFINED;
    } else {
        color = 1;
    }
    
    if (color == MPI_UNDEFINED) {
        printf("Process %d: Not needed for grid %dx%d. Stoppig.\n", rank, N, M);
        MPI_Finalize();
        return 0;
    } else if (color == 0) {
       //node0(rank, size, N, M, comm);
    }
    

    MPI_Comm active_comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &active_comm);

    int periods[] = {0, 0};
    int reorder = 1;
    MPI_Cart_create(active_comm, ndims, dims, periods, reorder, &comm_grid);



    MPI_Finalize();

    return 0;
}