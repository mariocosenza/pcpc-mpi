#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define LIVE true
#define DEAD false

uint32_t SEED = 34577;
uint8_t N_GEN = 4;

typedef struct {
    int M;
    int N;
} Size_matrix;

typedef struct {
    int r;
    int c;
} Grid_dims;

typedef struct {
    Size_matrix size;
    void *matrix;
    bool *l_ghost;
    bool *r_ghost;
    bool *t_ghost;
    bool *b_ghost;
} Game_matrix;

uint32_t count_live_cells(uint32_t total_cells, bool *buffer) {
    uint32_t count = 0;
    for (uint_fast32_t i = 0; i < total_cells; i++) {
        if (buffer[i] == LIVE) {
            count++;
        }
    }
    return count;
}

bool is_min_size(int proc_matrix_size, int M, int N) {
    if (((M / proc_matrix_size) < 1000) || ((N / proc_matrix_size) < 1000)) {
        return false;
    }
    return true;
}

void partition_dimension(int total, int parts, int *sizes, int *offsets) {
    int q = total / parts;
    int r = total % parts;
    int small = parts - r;

    int curr_offset = 0;
    for (int i = 0; i < parts; i++) {
        sizes[i] = (i < small) ? q : q + 1;
        if (offsets != NULL) {
            offsets[i] = curr_offset;
        }
        curr_offset += sizes[i];
    }
}

void parse_args(int argc, char **argv, uint32_t *M, uint32_t *N) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            *N = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-M") == 0 && i + 1 < argc) {
            *M = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-G") == 0 && i + 1 < argc) {
            N_GEN = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-S") == 0 && i + 1 < argc) {
            SEED = (uint32_t)atoi(argv[++i]);
        }
    }
}

void game_matrix_rand(int M, int N, int proc_compute_size, int first_compute_rank) {
    int mpi_dims[2] = {0, 0};
    MPI_Dims_create(proc_compute_size, 2, mpi_dims);
    Grid_dims topo = {mpi_dims[0], mpi_dims[1]};

    int row_sizes[topo.r], row_offsets[topo.r];
    int col_sizes[topo.c], col_offsets[topo.c];

    partition_dimension(M, topo.r, row_sizes, row_offsets);
    partition_dimension(N, topo.c, col_sizes, col_offsets);

    bool (*global_matrix)[N] = malloc(sizeof(bool[M][N]));
    if (global_matrix == NULL) {
        return;
    }

    srand(SEED);
    for (int r = 0; r < M; r++) {
        for (int c = 0; c < N; c++) {
            global_matrix[r][c] = (rand() % 2) != 0 ? LIVE : DEAD;
        }
    }

    for (int worker_rank = 0; worker_rank < proc_compute_size; worker_rank++) {
        int coords_r = worker_rank / topo.c;
        int coords_c = worker_rank % topo.c;
        int local_M = row_sizes[coords_r];
        int local_N = col_sizes[coords_c];
        int row_offset = row_offsets[coords_r];
        int col_offset = col_offsets[coords_c];

        int sizes[2] = {M, N};
        int subsizes[2] = {local_M, local_N};
        int starts[2] = {row_offset, col_offset};
        
        MPI_Datatype block_type;
        MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, MPI_C_BOOL, &block_type);
        MPI_Type_commit(&block_type);

        int target_rank = first_compute_rank + worker_rank;
        MPI_Send(&global_matrix[0][0], 1, block_type, target_rank, 0, MPI_COMM_WORLD);
        MPI_Type_free(&block_type);
    }

    free(global_matrix);
}

void play_inner(void *game_matrix_ptr, void *new_matrix_ptr, int r, int c, int N) {
    bool (*matrix)[N] = game_matrix_ptr;
    bool (*new_matrix)[N] = new_matrix_ptr;
    
    int live_count = (matrix[r - 1][c - 1] == LIVE) + (matrix[r - 1][c] == LIVE) + (matrix[r - 1][c + 1] == LIVE) +
                     (matrix[r][c - 1] == LIVE)                                  + (matrix[r][c + 1] == LIVE) +
                     (matrix[r + 1][c - 1] == LIVE) + (matrix[r + 1][c] == LIVE) + (matrix[r + 1][c + 1] == LIVE);

    new_matrix[r][c] = ((live_count == 3) || (live_count == 2 && matrix[r][c] == LIVE)) ? LIVE : DEAD;
}

void play(Game_matrix *game_matrix, void *new_matrix_ptr, int r, int c) {
    int M = game_matrix->size.M;
    int N = game_matrix->size.N;
    bool (*matrix)[N] = game_matrix->matrix;
    bool (*new_matrix)[N] = new_matrix_ptr;
    int live_count = 0;

    if (r == 0 && c == 0) {
        live_count += (game_matrix->t_ghost[0] == LIVE) + (game_matrix->t_ghost[1] == LIVE)
            + (game_matrix->l_ghost[0] == LIVE) + (game_matrix->l_ghost[1] == LIVE) + (game_matrix->l_ghost[2] == LIVE);
    } else if (r == 0 && c == N - 1) {
        live_count += (game_matrix->t_ghost[N - 2] == LIVE) + (game_matrix->t_ghost[N - 1] == LIVE)
            + (game_matrix->r_ghost[0] == LIVE) + (game_matrix->r_ghost[1] == LIVE) + (game_matrix->r_ghost[2] == LIVE);
    } else if (r == M - 1 && c == 0) {
        live_count += (game_matrix->b_ghost[0] == LIVE) + (game_matrix->b_ghost[1] == LIVE)
            + (game_matrix->l_ghost[M - 1] == LIVE) + (game_matrix->l_ghost[M] == LIVE) + (game_matrix->l_ghost[M + 1] == LIVE);
    } else if (r == M - 1 && c == N - 1) {
        live_count += (game_matrix->b_ghost[N - 2] == LIVE) + (game_matrix->b_ghost[N - 1] == LIVE)
            + (game_matrix->r_ghost[M - 1] == LIVE) + (game_matrix->r_ghost[M] == LIVE) + (game_matrix->r_ghost[M + 1] == LIVE);
    }

    if (r == 0 && c > 0 && c < N - 1) {
        live_count += (game_matrix->t_ghost[c - 1] == LIVE) + (game_matrix->t_ghost[c] == LIVE) + (game_matrix->t_ghost[c + 1] == LIVE);
    } else if (r == M - 1 && c > 0 && c < N - 1) {
        live_count += (game_matrix->b_ghost[c - 1] == LIVE) + (game_matrix->b_ghost[c] == LIVE) + (game_matrix->b_ghost[c + 1] == LIVE);
    }
    
    if (c == 0 && r > 0 && r < M - 1) {
        live_count += (game_matrix->l_ghost[r] == LIVE) + (game_matrix->l_ghost[r + 1] == LIVE) + (game_matrix->l_ghost[r + 2] == LIVE);
    } else if (c == N - 1 && r > 0 && r < M - 1) {
        live_count += (game_matrix->r_ghost[r] == LIVE) + (game_matrix->r_ghost[r + 1] == LIVE) + (game_matrix->r_ghost[r + 2] == LIVE);
    }

    for (int i = r - 1; i <= r + 1; i++) {
        for (int j = c - 1; j <= c + 1; j++) {
            if (i == r && j == c) continue;
            if (i >= 0 && i < M && j >= 0 && j < N) {
                if (matrix[i][j] == LIVE) {
                    live_count++;
                }
            }
        }
    }

    new_matrix[r][c] = ((live_count == 3) || (live_count == 2 && matrix[r][c] == LIVE)) ? LIVE : DEAD;
}

void recv_top_bottom(MPI_Comm comm, Game_matrix *game_matrix, int rank_top, int rank_bottom, MPI_Request req[2]) {
    int N = game_matrix->size.N;
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;
    if (rank_top != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->t_ghost, N, MPI_C_BOOL, rank_top, 1, comm, &req[0]);
    }
    if (rank_bottom != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->b_ghost, N, MPI_C_BOOL, rank_bottom, 1, comm, &req[1]);
    }
}

void send_top_bottom(MPI_Comm comm, Game_matrix *game_matrix, int rank_top, int rank_bottom, MPI_Request req[2]) {
    int M = game_matrix->size.M;
    int N = game_matrix->size.N;
    bool (*matrix)[N] = game_matrix->matrix;
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;
    if (rank_top != MPI_PROC_NULL) {
        MPI_Isend(matrix[0], N, MPI_C_BOOL, rank_top, 1, comm, &req[0]);
    }
    if (rank_bottom != MPI_PROC_NULL) {
        MPI_Isend(matrix[M - 1], N, MPI_C_BOOL, rank_bottom, 1, comm, &req[1]);
    }
}

void build_left_right_edges(Game_matrix *game_matrix, bool *left_edge, bool *right_edge) {
    int M = game_matrix->size.M;
    int N = game_matrix->size.N;
    bool (*matrix)[N] = game_matrix->matrix;
    
    left_edge[0] = game_matrix->t_ghost[0];
    right_edge[0] = game_matrix->t_ghost[N - 1];
    
    for (int i = 0; i < M; i++) {
        left_edge[i + 1] = matrix[i][0];
        right_edge[i + 1] = matrix[i][N - 1];
    }
    
    left_edge[M + 1] = game_matrix->b_ghost[0];
    right_edge[M + 1] = game_matrix->b_ghost[N - 1];
}

void recv_left_right(MPI_Comm comm, Game_matrix *game_matrix, int rank_left, int rank_right, MPI_Request req[2]) {
    int M = game_matrix->size.M;
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;
    if (rank_left != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->l_ghost, M + 2, MPI_C_BOOL, rank_left, 2, comm, &req[0]);
    }
    if (rank_right != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->r_ghost, M + 2, MPI_C_BOOL, rank_right, 2, comm, &req[1]);
    }
}

void send_left_right(MPI_Comm comm, Game_matrix *game_matrix, int rank_left, int rank_right, bool *left_edge, bool *right_edge, MPI_Request req[2]) {
    int M = game_matrix->size.M;
    req[0] = MPI_REQUEST_NULL;
    req[1] = MPI_REQUEST_NULL;
    if (rank_left != MPI_PROC_NULL) {
        MPI_Isend(left_edge, M + 2, MPI_C_BOOL, rank_left, 2, comm, &req[0]);
    }
    if (rank_right != MPI_PROC_NULL) {
        MPI_Isend(right_edge, M + 2, MPI_C_BOOL, rank_right, 2, comm, &req[1]);
    }
}

void run_worker(int M, int N, int proc_compute_size, MPI_Comm splitted_comm, MPI_Comm gather_comm) {
    int mpi_dims[2] = {0, 0};
    MPI_Dims_create(proc_compute_size, 2, mpi_dims);
    Grid_dims topo = {mpi_dims[0], mpi_dims[1]};

    int worker_rank;
    MPI_Comm_rank(splitted_comm, &worker_rank);

    int coords[2] = {0, 0};
    MPI_Comm cart_comm;
    MPI_Cart_create(splitted_comm, 2, mpi_dims, (int[]){0, 0}, 0, &cart_comm);
    MPI_Cart_coords(cart_comm, worker_rank, 2, coords);

    int row_sizes[topo.r], col_sizes[topo.c];
    partition_dimension(M, topo.r, row_sizes, NULL);
    partition_dimension(N, topo.c, col_sizes, NULL);

    uint_fast16_t local_M = row_sizes[coords[0]];
    uint_fast16_t local_N = col_sizes[coords[1]];
    
    bool (*matrix)[local_N] = malloc(sizeof(bool[local_M][local_N]));
    MPI_Recv(matrix, local_M * local_N, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    Game_matrix *game_matrix = malloc(sizeof(Game_matrix));
    game_matrix->size.M = local_M;
    game_matrix->size.N = local_N;
    game_matrix->matrix = matrix;
    game_matrix->l_ghost = calloc(local_M + 2, sizeof(bool));
    game_matrix->r_ghost = calloc(local_M + 2, sizeof(bool));
    game_matrix->t_ghost = calloc(local_N, sizeof(bool));
    game_matrix->b_ghost = calloc(local_N, sizeof(bool));
    
    bool *left_edge = malloc((local_M + 2) * sizeof(bool));
    bool *right_edge = malloc((local_M + 2) * sizeof(bool));
    bool (*new_matrix)[local_N] = malloc(sizeof(bool[local_M][local_N]));
    
    if (!left_edge || !right_edge || !new_matrix || !game_matrix) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int rank_top, rank_bottom, rank_left, rank_right;
    MPI_Cart_shift(cart_comm, 0, 1, &rank_top, &rank_bottom);
    MPI_Cart_shift(cart_comm, 1, 1, &rank_left, &rank_right);
    
    MPI_Request recv_tb_req[2], send_tb_req[2], recv_lr_req[2], send_lr_req[2];
    MPI_Status wait_stats[2];

    for (int gen = 0; gen < N_GEN; gen++) {
        uint32_t local_live = count_live_cells(local_M * local_N, game_matrix->matrix);
        MPI_Reduce(&local_live, NULL, 1, MPI_UNSIGNED, MPI_SUM, 0, gather_comm);

        if (gen < N_GEN - 1) {
            recv_top_bottom(cart_comm, game_matrix, rank_top, rank_bottom, recv_tb_req);
            send_top_bottom(cart_comm, game_matrix, rank_top, rank_bottom, send_tb_req);

            for (uint_fast16_t r = 1; r + 1 < local_M; r++) {
                for (uint_fast16_t c = 1; c + 1 < local_N; c++) {
                    play_inner(game_matrix->matrix, new_matrix, r, c, local_N);
                }
            }

            MPI_Waitall(2, recv_tb_req, wait_stats);
            MPI_Waitall(2, send_tb_req, wait_stats);

            build_left_right_edges(game_matrix, left_edge, right_edge);
            recv_left_right(cart_comm, game_matrix, rank_left, rank_right, recv_lr_req);
            send_left_right(cart_comm, game_matrix, rank_left, rank_right, left_edge, right_edge, send_lr_req);

            MPI_Waitall(2, recv_lr_req, wait_stats);
            MPI_Waitall(2, send_lr_req, wait_stats);

            for (uint_fast16_t c = 0; c < local_N; c++) {
                play(game_matrix, new_matrix, 0, c);
                if (local_M > 1) {
                    play(game_matrix, new_matrix, local_M - 1, c);
                }
            }

            for (uint_fast16_t r = 1; r + 1 < local_M; r++) {
                play(game_matrix, new_matrix, r, 0);
                if (local_N > 1) {
                    play(game_matrix, new_matrix, r, local_N - 1);
                }
            }

            void *tmp_matrix = game_matrix->matrix;
            game_matrix->matrix = new_matrix;
            new_matrix = tmp_matrix;
        }
    }

    free(game_matrix->l_ghost);
    free(game_matrix->r_ghost);
    free(game_matrix->t_ghost);
    free(game_matrix->b_ghost);
    free(left_edge);
    free(right_edge);
    free(new_matrix);
    free(game_matrix->matrix);
    free(game_matrix);
    MPI_Comm_free(&cart_comm);
}

void run_master(int M, int N, int proc_compute_size, int first_compute_rank, MPI_Comm gather_comm) {
    game_matrix_rand(M, N, proc_compute_size, first_compute_rank);

    for (int gen = 0; gen < N_GEN; gen++) {
        uint32_t local_live = 0;
        uint32_t live = 0;
        MPI_Reduce(&local_live, &live, 1, MPI_UNSIGNED, MPI_SUM, 0, gather_comm);
        printf("Gen: %d computed. Live cells: %d\n", gen, live);
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    int rank, size, proc_compute_size;
    int M = 10000, N = 10000;
    
    parse_args(argc, argv, &M, &N);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "ERROR: At least 2 processes are required\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    proc_compute_size = size - 1;
    while (proc_compute_size > 1 && !is_min_size(proc_compute_size, M, N)) {
        proc_compute_size--;
    }
    int first_compute_rank = size - proc_compute_size;

    if (rank == 0) {
        if (proc_compute_size == 1) {
            printf("INFO: Using only one node\n");
        } else {
            printf("INFO: Using %d nodes\n", proc_compute_size);
        }
    }

    MPI_Comm splitted_comm, gather_comm;
    int color_split = (rank < first_compute_rank) ? MPI_UNDEFINED : 1;
    MPI_Comm_split(MPI_COMM_WORLD, color_split, rank, &splitted_comm);
    
    int color_gather = (rank == 0 || rank >= first_compute_rank) ? 1 : MPI_UNDEFINED;
    MPI_Comm_split(MPI_COMM_WORLD, color_gather, rank, &gather_comm);
    
    if (splitted_comm != MPI_COMM_NULL) {
        run_worker(M, N, proc_compute_size, splitted_comm, gather_comm);
        MPI_Comm_free(&splitted_comm);
    }

    if (rank == 0) {
        run_master(M, N, proc_compute_size, first_compute_rank, gather_comm);
    }

    if (gather_comm != MPI_COMM_NULL) {
        MPI_Comm_free(&gather_comm);
    }

    MPI_Finalize();
    return 0;
}