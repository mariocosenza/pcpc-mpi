#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define LIVE true
#define DEAD false

int N_GEN = 4;
uint32_t SEED = 34577;

typedef struct {
    uint32_t rows;
    uint32_t cols;
} Size_matrix;

typedef struct {
    Size_matrix size;
    void        *matrix;
    bool        *recv_l_ghost;
    bool        *recv_r_ghost;
    bool        *recv_t_ghost;
    bool        *recv_b_ghost;
} Game_matrix;

typedef struct {
    double time;
    int    rank;
} TimeRankPair;

uint32_t count_live_cells(uint64_t total_cells, bool *buffer) {
    uint32_t count = 0;
    for (uint64_t i = 0; i < total_cells; i++) {
        if (buffer[i] == LIVE) {
            count++;
        }
    }
    return count;
}

bool is_min_size(uint32_t proc_compute_size, uint32_t M, uint32_t N) {
    uint64_t cells_per_node = ((uint64_t)M * N) / proc_compute_size;
    
    return cells_per_node >= 1000000;
}

void partition_dimension(uint32_t total, int parts, int *sizes, int *offsets) {
    uint32_t q = total / (uint32_t)parts;
    uint32_t r = total % (uint32_t)parts;
    uint32_t small = (uint32_t)parts - r;
    uint32_t curr_offset = 0;

    for (int i = 0; i < parts; i++) {
        sizes[i] = (int)((uint32_t)i < small ? q : q + 1);
        if (offsets != NULL) {
            offsets[i] = (int)curr_offset;
        }
        curr_offset += (uint32_t)sizes[i];
    }
}

void parse_args(int argc, char **argv, uint32_t *M, uint32_t *N) {
    int opt;
    while ((opt = getopt(argc, argv, "N:M:G:S:")) != -1) {
        switch (opt) {
            case 'N': 
                *N = (uint32_t)atoi(optarg); 
                break;
            case 'M': 
                *M = (uint32_t)atoi(optarg); 
                break;
            case 'G': 
                N_GEN = atoi(optarg); 
                break;
            case 'S': 
                SEED = (uint32_t)atoi(optarg); 
                break;
        }
    }
}

void master_generate_and_distribute(uint32_t M, uint32_t N, int proc_compute_size, int first_worker_rank, int mpi_dims[2]) {
    int row_sizes[mpi_dims[0]];
    int row_offsets[mpi_dims[0]];
    int col_sizes[mpi_dims[1]];
    int col_offsets[mpi_dims[1]];
    
    partition_dimension(M, mpi_dims[0], row_sizes, row_offsets);
    partition_dimension(N, mpi_dims[1], col_sizes, col_offsets);

    bool (*global_matrix)[N] = malloc(sizeof(bool[M][N]));
    if (!global_matrix) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    srand(SEED);
    for (uint_fast32_t r = 0; r < M; r++) {
        for (uint_fast32_t c = 0; c < N; c++) {
            global_matrix[r][c] = (rand() % 2) != 0;
        }
    }

    for (int worker_rank = 0; worker_rank < proc_compute_size; worker_rank++) {
        int r_idx = worker_rank / mpi_dims[1];
        int c_idx = worker_rank % mpi_dims[1];
        
        int sizes[2]    = { (int)M, (int)N };
        int subsizes[2] = { row_sizes[r_idx], col_sizes[c_idx] };
        int starts[2]   = { row_offsets[r_idx], col_offsets[c_idx] };

        MPI_Datatype block_type;
        MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, MPI_C_BOOL, &block_type);
        MPI_Type_commit(&block_type);
        
        MPI_Send(&global_matrix[0][0], 1, block_type, first_worker_rank + worker_rank, 0, MPI_COMM_WORLD);
        
        MPI_Type_free(&block_type);
    }
    
    free(global_matrix);
}

static inline void play_inner_cells(void *current_grid_ptr, void *next_grid_ptr, uint_fast32_t r, uint_fast32_t c, uint32_t N) {
    bool (*current_grid)[N] = current_grid_ptr;
    bool (*next_grid)[N]    = next_grid_ptr;

    uint_fast8_t live_neighbors = 
          current_grid[r-1][c-1] + current_grid[r-1][c] + current_grid[r-1][c+1]
        + current_grid[r  ][c-1]                        + current_grid[r  ][c+1]
        + current_grid[r+1][c-1] + current_grid[r+1][c] + current_grid[r+1][c+1];

    if (live_neighbors == 3 || (live_neighbors == 2 && current_grid[r][c] == LIVE)) {
        next_grid[r][c] = LIVE;
    } else {
        next_grid[r][c] = DEAD;
    }
}

static inline uint_fast8_t read_cell_or_ghost(Game_matrix *gm, int_fast32_t r, int_fast32_t c) {
    uint32_t max_rows = gm->size.rows;
    uint32_t max_cols = gm->size.cols;
    bool (*local_grid)[max_cols] = gm->matrix;

    if (r < 0 && c < 0) {
        return gm->recv_l_ghost[0];
    }
    if (r < 0 && c >= (int_fast32_t)max_cols) {
        return gm->recv_r_ghost[0];
    }
    if (r >= (int_fast32_t)max_rows && c < 0) {
        return gm->recv_l_ghost[max_rows + 1];
    }
    if (r >= (int_fast32_t)max_rows && c >= (int_fast32_t)max_cols) {
        return gm->recv_r_ghost[max_rows + 1];
    }

    if (r < 0) {
        return gm->recv_t_ghost[c];
    }
    if (r >= (int_fast32_t)max_rows) {
        return gm->recv_b_ghost[c];
    }

    if (c < 0) {
        return gm->recv_l_ghost[r + 1];
    }
    if (c >= (int_fast32_t)max_cols) {
        return gm->recv_r_ghost[r + 1];
    }

    return local_grid[r][c];
}

static inline void play_border_cells(Game_matrix *gm, void *next_grid_ptr, uint32_t r, uint32_t c) {
    uint32_t max_cols = gm->size.cols;
    bool (*next_grid)[max_cols]    = next_grid_ptr;
    bool (*current_grid)[max_cols] = gm->matrix;

    uint_fast8_t live_neighbors = 0;
    
    for (int_fast32_t dr = -1; dr <= 1; dr++) {
        for (int_fast32_t dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            live_neighbors += read_cell_or_ghost(gm, (int_fast32_t)r + dr, (int_fast32_t)c + dc);
        }
    }

    if (live_neighbors == 3 || (live_neighbors == 2 && current_grid[r][c] == LIVE)) {
        next_grid[r][c] = LIVE;
    } else {
        next_grid[r][c] = DEAD;
    }
}

void async_recv_top_bottom(MPI_Comm comm, Game_matrix *gm, int top_rank, int bot_rank, MPI_Request req[2]) {
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;

    if (top_rank != MPI_PROC_NULL) {
        MPI_Irecv(gm->recv_t_ghost, (int)gm->size.cols, MPI_C_BOOL, top_rank, 1, comm, &req[0]);
    }
    if (bot_rank != MPI_PROC_NULL) {
        MPI_Irecv(gm->recv_b_ghost, (int)gm->size.cols, MPI_C_BOOL, bot_rank, 1, comm, &req[1]);
    }
}

void async_send_top_bottom(MPI_Comm comm, Game_matrix *gm, int top_rank, int bot_rank, MPI_Request req[2]) {
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;
    
    bool (*local_grid)[gm->size.cols] = gm->matrix;

    if (top_rank != MPI_PROC_NULL) {
        MPI_Isend(local_grid[0], (int)gm->size.cols, MPI_C_BOOL, top_rank, 1, comm, &req[0]);
    }
    if (bot_rank != MPI_PROC_NULL) {
        MPI_Isend(local_grid[gm->size.rows - 1], (int)gm->size.cols, MPI_C_BOOL, bot_rank, 1, comm, &req[1]);
    }
}

void pack_left_right_send_buffers(Game_matrix *gm, bool *send_l_buf, bool *send_r_buf) {
    bool (*local_grid)[gm->size.cols] = gm->matrix;
    
    send_l_buf[0] = gm->recv_t_ghost[0];
    send_r_buf[0] = gm->recv_t_ghost[gm->size.cols - 1];
    
    for (uint_fast32_t r = 0; r < gm->size.rows; r++) {
        send_l_buf[r + 1] = local_grid[r][0];
        send_r_buf[r + 1] = local_grid[r][gm->size.cols - 1];
    }
    
    send_l_buf[gm->size.rows + 1] = gm->recv_b_ghost[0];
    send_r_buf[gm->size.rows + 1] = gm->recv_b_ghost[gm->size.cols - 1];
}

void async_recv_left_right(MPI_Comm comm, Game_matrix *gm, int left_rank, int right_rank, MPI_Request req[2]) {
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;

    if (left_rank != MPI_PROC_NULL) {
        MPI_Irecv(gm->recv_l_ghost, (int)gm->size.rows + 2, MPI_C_BOOL, left_rank, 2, comm, &req[0]);
    }
    if (right_rank != MPI_PROC_NULL) {
        MPI_Irecv(gm->recv_r_ghost, (int)gm->size.rows + 2, MPI_C_BOOL, right_rank, 2, comm, &req[1]);
    }
}

void async_send_left_right(MPI_Comm comm, Game_matrix *gm, int left_rank, int right_rank, bool *send_l_buf, bool *send_r_buf, MPI_Request req[2]) {
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;

    if (left_rank != MPI_PROC_NULL) {
        MPI_Isend(send_l_buf, (int)gm->size.rows + 2, MPI_C_BOOL, left_rank, 2, comm, &req[0]);
    }
    if (right_rank != MPI_PROC_NULL) {
        MPI_Isend(send_r_buf, (int)gm->size.rows + 2, MPI_C_BOOL, right_rank, 2, comm, &req[1]);
    }
}

void run_worker(int rank, uint32_t M, uint32_t N, int compute_sz, int mpi_dims[2], MPI_Comm split_comm, MPI_Comm gather_comm) {
    int w_rank;
    MPI_Comm_rank(split_comm, &w_rank);

    MPI_Comm cart_comm;
    int coords[2];
    MPI_Cart_create(split_comm, 2, mpi_dims, (int[]){0, 0}, 0, &cart_comm);
    MPI_Cart_coords(cart_comm, w_rank, 2, coords);

    int row_sizes[mpi_dims[0]];
    int col_sizes[mpi_dims[1]];
    partition_dimension(M, mpi_dims[0], row_sizes, NULL);
    partition_dimension(N, mpi_dims[1], col_sizes, NULL);

    uint32_t local_rows = (uint32_t)row_sizes[coords[0]];
    uint32_t local_cols = (uint32_t)col_sizes[coords[1]];

    if (local_rows == 0 || local_cols == 0) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    bool (*local_grid)[local_cols] = malloc(sizeof(bool[local_rows][local_cols]));
    if (!local_grid) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    MPI_Recv(local_grid, (int)(local_rows * local_cols), MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    bool *recv_l_ghost = calloc(local_rows + 2, sizeof(bool));
    bool *recv_r_ghost = calloc(local_rows + 2, sizeof(bool));
    bool *recv_t_ghost = calloc(local_cols, sizeof(bool));
    bool *recv_b_ghost = calloc(local_cols, sizeof(bool));
    
    bool *send_l_buf   = malloc((local_rows + 2) * sizeof(bool));
    bool *send_r_buf   = malloc((local_rows + 2) * sizeof(bool));
    
    bool (*next_local_grid)[local_cols] = malloc(sizeof(bool[local_rows][local_cols]));

    if (!recv_l_ghost || !recv_r_ghost || !recv_t_ghost || !recv_b_ghost || !send_l_buf || !send_r_buf || !next_local_grid) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    Game_matrix gm = {
        .size         = { local_rows, local_cols },
        .matrix       = local_grid,
        .recv_l_ghost = recv_l_ghost,
        .recv_r_ghost = recv_r_ghost,
        .recv_t_ghost = recv_t_ghost,
        .recv_b_ghost = recv_b_ghost,
    };

    int top_rank;
    int bot_rank;
    int left_rank;
    int right_rank;
    
    MPI_Cart_shift(cart_comm, 0, 1, &top_rank, &bot_rank);
    MPI_Cart_shift(cart_comm, 1, 1, &left_rank, &right_rank);

    MPI_Request req_recv_tb[2];
    MPI_Request req_send_tb[2];
    MPI_Request req_recv_lr[2];
    MPI_Request req_send_lr[2];
    MPI_Status  mpi_stats[2];

    for (int g = 0; g < N_GEN; g++) {
        uint32_t live_count = count_live_cells((uint64_t)local_rows * local_cols, (bool *)gm.matrix);
        MPI_Reduce(&live_count, NULL, 1, MPI_UINT32_T, MPI_SUM, 0, gather_comm);

        if (g < N_GEN - 1) {
            async_recv_top_bottom(cart_comm, &gm, top_rank, bot_rank, req_recv_tb);
            async_send_top_bottom(cart_comm, &gm, top_rank, bot_rank, req_send_tb);

            for (uint_fast32_t r = 1; r + 1 < local_rows; r++) {
                for (uint_fast32_t c = 1; c + 1 < local_cols; c++) {
                    play_inner_cells(gm.matrix, next_local_grid, r, c, local_cols);
                }
            }

            MPI_Waitall(2, req_recv_tb, mpi_stats);
            MPI_Waitall(2, req_send_tb, mpi_stats);

            pack_left_right_send_buffers(&gm, send_l_buf, send_r_buf);
            
            async_recv_left_right(cart_comm, &gm, left_rank, right_rank, req_recv_lr);
            async_send_left_right(cart_comm, &gm, left_rank, right_rank, send_l_buf, send_r_buf, req_send_lr);

            MPI_Waitall(2, req_recv_lr, mpi_stats);
            MPI_Waitall(2, req_send_lr, mpi_stats);

            for (uint_fast32_t c = 0; c < local_cols; c++) {
                play_border_cells(&gm, next_local_grid, 0, c);
                if (local_rows > 1) {
                    play_border_cells(&gm, next_local_grid, local_rows - 1, c);
                }
            }
            
            for (uint_fast32_t r = 1; r + 1 < local_rows; r++) {
                play_border_cells(&gm, next_local_grid, r, 0);
                if (local_cols > 1) {
                    play_border_cells(&gm, next_local_grid, r, local_cols - 1);
                }
            }

            void *tmp_ptr   = gm.matrix;
            gm.matrix       = next_local_grid;
            next_local_grid = tmp_ptr;
        }
    }

    free(recv_l_ghost);
    free(recv_r_ghost);
    free(recv_t_ghost);
    free(recv_b_ghost);
    free(send_l_buf);
    free(send_r_buf);
    free(next_local_grid);
    free(gm.matrix);
    
    MPI_Comm_free(&cart_comm);
}

void run_master(uint32_t M, uint32_t N, int compute_sz, int first_rank, int mpi_dims[2], MPI_Comm gather_comm) {
    master_generate_and_distribute(M, N, compute_sz, first_rank, mpi_dims);
    
    for (int g = 0; g < N_GEN; g++) {
        uint32_t tot_live = 0;
        MPI_Reduce(MPI_IN_PLACE, &tot_live, 1, MPI_UINT32_T, MPI_SUM, 0, gather_comm);
        printf("Gen: %d | Live: %u\n", g, tot_live);
    }
}

int main(int argc, char **argv) {
    uint32_t M = 10000;
    uint32_t N = 10000;
    
    parse_args(argc, argv, &M, &N);

    MPI_Init(&argc, &argv);

    int rank;
    int num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    double start_time = MPI_Wtime();

    int compute_sz = num_procs - 1;
    while (compute_sz > 1 && !is_min_size((uint32_t)compute_sz, M, N)) {
        compute_sz--;
    }
    
    int first_worker_rank = num_procs - compute_sz;

    if (rank == 0) {
        printf("Nodes: %d\n", compute_sz);
    }

    int mpi_dims[2] = {0, 0};
    MPI_Dims_create(compute_sz, 2, mpi_dims);

    if (mpi_dims[0] <= 0 || mpi_dims[1] <= 0) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    MPI_Comm split_comm;
    MPI_Comm gather_comm;
    
    int split_color  = (rank < first_worker_rank) ? MPI_UNDEFINED : 1;
    int gather_color = (rank == 0 || rank >= first_worker_rank) ? 1 : MPI_UNDEFINED;
    
    MPI_Comm_split(MPI_COMM_WORLD, split_color, rank, &split_comm);
    MPI_Comm_split(MPI_COMM_WORLD, gather_color, rank, &gather_comm);

    if (rank == 0) {
        run_master(M, N, compute_sz, first_worker_rank, mpi_dims, gather_comm);
    }
    
    if (split_comm != MPI_COMM_NULL) {
        run_worker(rank, M, N, compute_sz, mpi_dims, split_comm, gather_comm);
        MPI_Comm_free(&split_comm);
    }
    
    if (gather_comm != MPI_COMM_NULL) {
        MPI_Comm_free(&gather_comm);
    }

    TimeRankPair local_time = { MPI_Wtime() - start_time, rank };
    TimeRankPair max_time;
    
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("Max Time: %f s (Rank: %d)\n", max_time.time, max_time.rank);
    }

    MPI_Finalize();
    return 0;
}