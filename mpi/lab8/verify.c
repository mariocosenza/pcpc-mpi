#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

uint32_t count_live_cells(uint64_t total_cells, uint8_t *buffer) {
    uint32_t count = 0;
    for (uint64_t i = 0; i < total_cells; i++) {
        if (buffer[i] == 1) {
            count++;
        }
    }
    return count;
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
    u_int64_t count_live = 0;
    for (uint32_t r = 0; r < M; r++) {
        if (fread(row_buffer, sizeof(uint8_t), N, fp) != N) {
            fprintf(stderr, "Errore nella lettura della riga %u\n", r);
            break;
        }
        for (uint32_t c = 0; c < N; c++) {
            count_live += count_live_cells(N, row_buffer[c]);
           
        }
        printf("\n");
    }
    printf("Totale celle vive: %lu\n", count_live);

    free(row_buffer);
    fclose(fp);
}

void parse_args(int argc, char **argv, uint32_t *M, uint32_t *N) {
    int opt;
    while ((opt = getopt(argc, argv, "N:M:S:P:")) != -1) {
        switch (opt) {
            case 'N': *N = (uint32_t)atoi(optarg); break;
            case 'M': *M = (uint32_t)atoi(optarg); break;
        }
    }
}

int main(int argc, char **argv) {
    uint32_t M = 100;
    uint32_t N = 100;
    parse_args(argc, argv, &M, &N);
    
    
    print_while_reading_matrix(M, N);
     
    
    return 0;
}