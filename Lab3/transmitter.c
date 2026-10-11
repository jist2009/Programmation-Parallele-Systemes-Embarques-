#include <stdint.h>
#include <stdlib.h>
#include "project.h"

// write into the buffer U_K
void source_generate(uint8_t *U_K, size_t K){
    for (size_t i = 0; i < K; i++) {
        U_K[i] = rand() % 2 ; // Génère un nombre aléatoire : 0 ou 1
    }
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
