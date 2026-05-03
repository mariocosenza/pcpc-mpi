#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

uint32_t SEED = 1234;
int PATTERN = 0; 
uint8_t READ = 0;

void write_matrix_to_file_fast(uint32_t M, uint32_t N) {
    FILE *fp = fopen("matrix.bin", "wb");
    if (!fp) {
        perror("Errore nell'apertura del file");
        exit(EXIT_FAILURE);
    }

    uint8_t *row_buffer = calloc(N, sizeof(uint8_t));
    if (!row_buffer) {
        printf("Errore di allocazione memoria\n");
        exit(EXIT_FAILURE);
    }

    if (PATTERN == 0) srand(SEED);

    for (uint32_t r = 0; r < M; r++) {
        if (PATTERN != 0) {
            memset(row_buffer, 0, N * sizeof(uint8_t));
        }

        if (PATTERN == 0) {
            for (uint32_t c = 0; c < N; c++) {
                row_buffer[c] = (rand() % 2 == 0);
            }
        } else if (PATTERN == 1 && r < 3 && M >= 3 && N >= 3) {
            // Glider top left
            if (r == 0) { row_buffer[1] = 1; }
            else if (r == 1) { row_buffer[2] = 1; }
            else if (r == 2) { row_buffer[0] = 1; row_buffer[1] = 1; row_buffer[2] = 1; }
        } else if (PATTERN == 2 && M >= 3 && N >= 3) {
            // Blinker center
            if (r == M / 2) { row_buffer[N / 2 - 1] = 1; row_buffer[N / 2] = 1; row_buffer[N / 2 + 1] = 1; }
        } else if (PATTERN == 3 && M >= 2 && N >= 2) {
            // Block center
            if (r == M / 2 || r == M / 2 + 1) { row_buffer[N / 2] = 1; row_buffer[N / 2 + 1] = 1; }
        }

        fwrite(row_buffer, sizeof(uint8_t), N, fp);
    }

    free(row_buffer);
    fclose(fp);
    
    const char* pattern_names[] = {"Random", "Glider", "Blinker", "Block"};
    printf("Matrice %u x %u scritta su matrix.bin (Pattern: %s)\n", M, N, pattern_names[PATTERN]);
}

void print_while_reading_matrix(uint32_t M, uint32_t N) {
    FILE *fp = fopen("matrix.bin", "rb");
    if (!fp) {
        perror("Errore nell'apertura del file");
        exit(EXIT_FAILURE);
    }

    uint8_t *row_buffer = malloc(N * sizeof(uint8_t));
    if (!row_buffer) {
        printf("Errore di allocazione memoria per la riga\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    for (uint32_t r = 0; r < M; r++) {
        if (fread(row_buffer, sizeof(uint8_t), N, fp) != N) {
            fprintf(stderr, "Errore nella lettura della riga %u\n", r);
            break;
        }
        for (uint32_t c = 0; c < N; c++) {
            printf("%c", row_buffer[c] ? 'O' : '.');
        }
        printf("\n");
    }

    free(row_buffer);
    fclose(fp);
}



void parse_args(int argc, char **argv, uint32_t *M, uint32_t *N) {
    int opt;
    while ((opt = getopt(argc, argv, "N:M:S:P:")) != -1) {
        switch (opt) {
            case 'N': *N = (uint32_t)atoi(optarg); break;
            case 'M': *M = (uint32_t)atoi(optarg); break;
            case 'S': SEED = (uint32_t)atoi(optarg); break;
            case 'P': PATTERN = atoi(optarg); break;
            case 'R' : READ = 1; break;
        }
    }
}

int main(int argc, char **argv) {
    uint32_t M = 100;
    uint32_t N = 100;
    parse_args(argc, argv, &M, &N);
    
    if (READ) {
        print_while_reading_matrix(M, N);
    } else {
        write_matrix_to_file_fast(M, N);
    }
    
    return 0;
}