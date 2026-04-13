#include "lab8-node0.h"
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define BLOCK_START(id, p, n)  ((size_t)(id) * (n) / (p))
#define BLOCK_SIZE(id, p, n)   (BLOCK_START((id) + 1, p, n) - BLOCK_START(id, p, n))

static inline uint_fast32_t get_block_id(size_t index, uint_fast32_t p, size_t n) {
    uint_fast32_t base = n / p;
    uint_fast32_t rest = n % p;
    size_t threshold = (size_t)rest * (base + 1);
    return (index < threshold) ? (uint_fast32_t)(index / (base + 1)) : (uint_fast32_t)(rest + (index - threshold) / base);
}

char *random_init(const size_t N, const size_t M, int size, int **out_sendcounts, int **out_displs) {
    srand(12345);
    int num_slaves = size - 1;
    
    int dims[2] = {0, 0};
    MPI_Dims_create(num_slaves, 2, dims);
    uint_fast32_t Px = dims[0]; 
    uint_fast32_t Py = dims[1];

    int *sendcounts = malloc(num_slaves * sizeof(int));
    int *displs = malloc(num_slaves * sizeof(int));

    char *global_grid = malloc(N * M * sizeof(char));
    size_t offset = 0;

    for (uint_fast32_t i = 0; i < (uint_fast32_t)num_slaves; i++) {
        uint_fast32_t coord_y = i / Py;
        uint_fast32_t coord_x = i % Py;
        
        int target_N = (int)BLOCK_SIZE(coord_y, Px, N);
        int target_M = (int)BLOCK_SIZE(coord_x, Py, M);
        int total_cells = target_N * target_M;

        sendcounts[i] = total_cells;
        displs[i] = (int)offset;

        for (int_fast32_t j = 0; j < total_cells; j++) {
            global_grid[offset++] = (char)(rand() & 1); 
        }
    }

    *out_sendcounts = sendcounts;
    *out_displs = displs;
    return global_grid;
}

void print_global_grid(const char *grid, size_t N, size_t M, uint_fast32_t Px, uint_fast32_t Py, const int *displs) {
    printf("\n=== MATRICE GLOBALE (%zu x %zu) ===\n", N, M);
    
    for (size_t r = 0; r < N; r++) {
        uint_fast32_t coord_y = get_block_id(r, Px, N);
        size_t local_r = r - BLOCK_START(coord_y, Px, N);

        for (size_t c = 0; c < M; c++) {
            uint_fast32_t coord_x = get_block_id(c, Py, M);
            size_t local_c = c - BLOCK_START(coord_x, Py, M);

            size_t target_M = BLOCK_SIZE(coord_x, Py, M);
            uint_fast32_t cart_rank = coord_y * Py + coord_x;
            
            size_t exact_index = (size_t)displs[cart_rank] + (local_r * target_M) + local_c;
            
            putchar(".O"[(unsigned char)grid[exact_index]]);
            putchar(' ');
        }
        putchar('\n');
    }
    printf("==============================\n\n");
}


int node0(const int rank, const int size, const size_t N, const size_t M, MPI_Comm comm) {

    int dims[2] = {0, 0};
    MPI_Dims_create(size - 1, 2, dims);
    int Px = dims[0]; 
    int Py = dims[1]; 

    char *matrix = random_init(N, M, comm, size);
    

    return 0;
} 