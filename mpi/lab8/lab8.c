#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdbool.h>

#define LIVE true
#define DEAD false
#define SEED 34577
#define M 600
#define N 600
#define N_GEN 4

typedef struct {
    int m;
    int n;
} Size_matrix;

typedef struct {
    Size_matrix *game_matrix_size;
    void *matrix;
    bool *l_ghost;
    bool *r_ghost;
    bool *t_ghost;
    bool *b_ghost;
} Game_matrix;

bool is_min_size(int proc_matrix_size, int matrix_m, int matrix_n) {
    if (((matrix_m / proc_matrix_size) < 100) || ((matrix_n / proc_matrix_size) < 100)) {
        return false;
    }
    return true;
}

void* game_matrix_rand(int m, int n, int rank)  {
    bool (*matrix)[n] = malloc(sizeof(bool[m][n]));
    
    if (matrix == NULL) return NULL;

    srand(SEED / (rank + 1)); 
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = rand() % 2 != 0;
        }
    }
    return matrix;
}

Size_matrix* matrix_size_proc(Size_matrix proc_matrix[], int proc_matrix_size, int matrix_m, int matrix_n) {
    int q_m = matrix_m / proc_matrix_size;
    int r_m = matrix_m % proc_matrix_size;
    int q_n = matrix_n / proc_matrix_size;
    int r_n = matrix_n % proc_matrix_size;

    for (int i = 0; i < proc_matrix_size; i++) {
        if (i < proc_matrix_size - r_m) {
            proc_matrix[i].m = q_m;
        } else {
            proc_matrix[i].m = q_m + 1;
        }
    }

    for (int j = 0; j < proc_matrix_size; j++) {
        if (j < proc_matrix_size - r_n) {
            proc_matrix[j].n = q_n;
        } else {
            proc_matrix[j].n = q_n + 1;
        }
    }

    return proc_matrix;
} 

void play(Game_matrix *game_matrix, void *new_matrix_ptr, int curr_m, int curr_n) {
    int m = game_matrix->game_matrix_size->m;
    int n = game_matrix->game_matrix_size->n;
    bool (*matrix)[n] = (bool (*)[n])game_matrix->matrix;
    bool (*new_matrix)[n] = (bool (*)[n])new_matrix_ptr;
    int live_count = 0;
    int i, j;

    if (curr_m == 0 && curr_n == 0) {
        live_count += game_matrix->t_ghost[0] + game_matrix->l_ghost[0];
    } else if (curr_m == 0 && curr_n == n - 1) {
        live_count += game_matrix->t_ghost[n-1] + game_matrix->r_ghost[0];
    } else if (curr_m == m - 1 && curr_n == 0) {
        live_count += game_matrix->b_ghost[0] + game_matrix->l_ghost[m-1];
    } else if (curr_m == m - 1 && curr_n == n - 1) {
        live_count += game_matrix->b_ghost[n-1] + game_matrix->r_ghost[m-1];
    }

    if (curr_m == 0 && curr_n > 0 && curr_n < n - 1) {
        live_count += game_matrix->t_ghost[curr_n-1] + game_matrix->t_ghost[curr_n] + game_matrix->t_ghost[curr_n+1];
    } else if (curr_m == m - 1 && curr_n > 0 && curr_n < m - 1) {
        live_count += game_matrix->b_ghost[curr_n-1] + game_matrix->b_ghost[curr_n] + game_matrix->b_ghost[curr_n+1];
    }
    
    if (curr_n == 0 && curr_m > 0 && curr_m < m - 1) {
        live_count += game_matrix->l_ghost[curr_m-1] + game_matrix->l_ghost[curr_m] + game_matrix->l_ghost[curr_m+1];
    } else if (curr_n == n - 1 && curr_m > 0 && curr_m < m - 1) {
        live_count += game_matrix->r_ghost[curr_m-1] + game_matrix->r_ghost[curr_m] + game_matrix->r_ghost[curr_m+1];
    }

    for (i = curr_m - 1; i <= curr_m + 1; i++) {
        for (j = curr_n - 1; j <= curr_n + 1; j++) {
            if (i == curr_m && j == curr_n) continue;
            if (i >= 0 && i < m && j >= 0 && j < n) {
                live_count += matrix[i][j];
            }
        }
    }

    if (matrix[curr_m][curr_n]) {
        new_matrix[curr_m][curr_n] = (live_count == 2 || live_count == 3);
    } else {
        new_matrix[curr_m][curr_n] = (live_count == 3);
    }
}


MPI_Request *send_top_bottom(MPI_Comm comm, Game_matrix *game_matrix, int n_up, int n_down) {
    int n = game_matrix->game_matrix_size->n;
    int m = game_matrix->game_matrix_size->m;
    bool (*matrix)[n] = game_matrix->matrix;
    MPI_Request *requests = malloc(2 * sizeof(MPI_Request));
    requests[0] = MPI_REQUEST_NULL;
    requests[1] = MPI_REQUEST_NULL;

    if (n_up != MPI_PROC_NULL) {
        MPI_Isend(matrix[0], n, MPI_C_BOOL, n_up, 1, comm, &requests[0]);
    }

    if (n_down != MPI_PROC_NULL) {
        MPI_Isend(matrix[m - 1], n, MPI_C_BOOL, n_down, 1, comm, &requests[1]);
    }

    return requests;
}

MPI_Request *send_left_right(MPI_Comm comm, Game_matrix *game_matrix, int n_left, int n_right) {
    int n = game_matrix->game_matrix_size->n;
    int m = game_matrix->game_matrix_size->m;
    bool (*matrix)[n] = game_matrix->matrix;
    MPI_Request *requests = malloc(2 * sizeof(MPI_Request));
    requests[0] = MPI_REQUEST_NULL;
    requests[1] = MPI_REQUEST_NULL;

    if (n_left != MPI_PROC_NULL) {
        MPI_Isend(&matrix[0][0], 1, MPI_C_BOOL, n_left, 1, comm, &requests[0]);
    }

    if (n_right != MPI_PROC_NULL) {
        MPI_Isend(&matrix[0][m - 1], 1, MPI_C_BOOL, n_right, 1, comm, &requests[1]);
    }

    return requests;
}

MPI_Request *recv_top_bottom(MPI_Comm comm, Game_matrix *game_matrix, int n_up, int n_down) {
    int n = game_matrix->game_matrix_size->n;
    MPI_Request *requests = malloc(2 * sizeof(MPI_Request));
    requests[0] = MPI_REQUEST_NULL;
    requests[1] = MPI_REQUEST_NULL;
  
    if (n_up != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->t_ghost, n, MPI_C_BOOL, n_up, 1, comm, &requests[0]);
    }

    if (n_down != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->b_ghost, n, MPI_C_BOOL, n_down, 1, comm, &requests[1]);
    }

    return requests;
}

MPI_Request *recv_left_right(MPI_Comm comm, Game_matrix *game_matrix, int n_left, int n_right) {
    int n = game_matrix->game_matrix_size->n;
    MPI_Request *requests = malloc(2 * sizeof(MPI_Request));
    requests[0] = MPI_REQUEST_NULL;
    requests[1] = MPI_REQUEST_NULL;

    if (n_left != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->l_ghost, n, MPI_C_BOOL, n_left, 1, comm, &requests[0]);
    }

    if (n_right != MPI_PROC_NULL) {
        MPI_Irecv(game_matrix->r_ghost, n, MPI_C_BOOL, n_right, 1, comm, &requests[1]);
    }

    return requests;
}

int main(int argc, char **argv) {
    int rank, size, proc_compute_size;
    int m = M, n = N, opt;
    MPI_Comm splitted_comm, cart_comm;

    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'N':
                n = atoi(optarg); 
                break;
            case 'M':
                m = atoi(optarg); 
                break;
            default:
                fprintf(stderr, "WARN: Please provide -M and -N for matrix size, otherwise default value will be used");
        }
    }

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    proc_compute_size = size - 1;

    while (proc_compute_size > 1 && !is_min_size(proc_compute_size, m, n)) {proc_compute_size--;}

    if (proc_compute_size == 1) {
        printf("INFO: Using only one node\n");
    } else {
        printf("INFO: Using %d nodes\n", proc_compute_size);
    }

    if (rank < size - proc_compute_size) {
        MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, 0, &splitted_comm);
    } else {
        MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &splitted_comm);
    }

    if(splitted_comm != MPI_COMM_NULL) {
        int dims[2] = {};
        MPI_Dims_create(proc_compute_size, 2, dims);
        printf("%d, %d\n", dims[0], dims[1]);

        Size_matrix *mat_sizes = (Size_matrix *)malloc(proc_compute_size * sizeof(Size_matrix));
        matrix_size_proc(mat_sizes, proc_compute_size, m, n);

        for(int i = 0; i < proc_compute_size; i++) {
            printf("m: %d, n: %d\n", mat_sizes[i].m, mat_sizes[i].n);
        }

        bool (*matrix)[n] = game_matrix_rand(m, n, rank);
        
        Game_matrix *game_matrix = (Game_matrix *)malloc(sizeof(Game_matrix));

        game_matrix->matrix = matrix;
        game_matrix->game_matrix_size = mat_sizes;
        game_matrix->l_ghost = (bool *) malloc(sizeof(bool) * (mat_sizes->m + 2));
        game_matrix->r_ghost = (bool *) malloc(sizeof(bool) * (mat_sizes->m + 2));
        game_matrix->t_ghost = (bool *) malloc(sizeof(bool) * (mat_sizes->n));
        game_matrix->b_ghost = (bool *) malloc(sizeof(bool) * (mat_sizes->n));

        int n_up, n_down, n_left, n_right;

        MPI_Cart_create(splitted_comm, 2, dims, (int[]){0, 0}, 0, &cart_comm);

        MPI_Cart_shift(cart_comm, 0, 1, &n_up, &n_down);
        MPI_Cart_shift(cart_comm, 1, 1, &n_left, &n_right);

        printf("Rank %d: L -> %d, UP -> %d, R -> %d, DW -> %d\n", rank, n_left, n_up, n_right, n_down);

        MPI_Request* recv_tb = recv_top_bottom(cart_comm, game_matrix, n_up, n_down);
        MPI_Request* recv_lr = recv_left_right(cart_comm, game_matrix, n_left, n_right);

        MPI_Request* send_tb = send_top_bottom(cart_comm, game_matrix, n_up, n_down);
        MPI_Request* send_lr = send_left_right(cart_comm, game_matrix, n_left, n_right);

        MPI_Status stats[2];

        MPI_Waitall(2, recv_tb, stats);
        MPI_Waitall(2, recv_lr, stats);
        MPI_Waitall(2, send_tb, stats);
        MPI_Waitall(2, send_lr, stats);

        free(recv_tb);
        free(recv_lr);
        free(send_tb);
        free(send_lr);

        bool (*new_matrix)[N] = malloc(sizeof(bool[M][N]));
        play(game_matrix, new_matrix, 0, 0);
    }

    if (rank == 0) {
        for (int i = 0; i < N_GEN; i++) {
            printf("Gen: %d\n", i);
        }
    }

    MPI_Finalize();
    return 0;
}