#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"

// write into the buffer U_K
void source_generate(uint8_t *U_K, size_t K){
    for (size_t i = 0; i < K; i++) {
        U_K[i] = rand() % 2 ; // Génère un nombre aléatoire : 0 ou 1
    }
}
// write only zeros in U_K
void source_generate_all_zeros(uint8_t *U_K, size_t K){
    // écrire uniquement des zéros dans U_K
    memset(U_K, 0, K * sizeof(uint8_t));
}

// read from the buffer U_K and write into the buffer C_N
void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps){
    for (size_t i = 0; i < n_reps; i++) {
        for (size_t j =0; j < K; j++) {
            C_N[i*K + j] = U_K[j]; // Répète U_K n_reps fois dans C_N
        }
    }
}


// read from C_N, write into X_N
void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N){
    for (size_t i = 0; i < N ; i++) {
        X_N[i] = (C_N[i] == 0) ? 1 : -1; // Modulation BPSK : 0 -> +1, 1 -> -1
    }
}

// write only ones in X_N
void modem_BPSK_modulate_all_ones(const uint8_t *C_N, int32_t *X_N, size_t N)  {
    (void)C_N;
    // écrire uniquement des 1 dans X_N
    for (size_t i = 0; i < N; i++)
        X_N[i] = 1;
}
