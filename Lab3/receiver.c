#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "project.h"

// demodulator, just copies Y_N in L_N for now
void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma){
    for (size_t i = 0; i < N; i++) {
        // Calculer le LLR pour chaque bit reçu
        L_N[i] = 2.0f * Y_N[i] / (sigma * sigma);
    }
}

// hard decoder: first hard decides each LLR and then makes a majority vote
void codec_repetition_hard_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
    for (size_t i = 0; i < K; i++) {
        int zeros = 0;
        int ones = 0;
        for (size_t j = 0; j < n_reps; j++) {
            if (L_N[j*K + i] >= 0) {
                zeros++; // LLR >= 0 bit est à 0
            } else {
                ones++; // LLR < 0 bit est à 1
            }
        }
        V_K[i] = (zeros >= ones) ? 0 : 1; // Majorité de 0 ou de 1
    }
}


// soft decoder: computes the mean of each LLR to hard decide the bits
void codec_repetition_soft_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
    for (size_t i = 0; i < K; i++) {
        float sum = 0.0f;;
        for (size_t j = 0; j < n_reps; j++) {
            sum += L_N[j*K + i];
        }
        V_K[i] = (sum >= 0.0f) ? 0 : 1;
    }
}

// update `n_bit_errors` and `n_frame_errors` variables depending on `U_K` and `V_K`
void monitor_check_errors(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t
*n_bit_errors, uint64_t *n_frame_errors){
    int frame_error = 0;
    for (size_t i = 0; i < K; i++) {
        if (U_K[i] != V_K[i]) {
            (*n_bit_errors)++;
            frame_error = 1;
        }
    }
    if (frame_error) {
        (*n_frame_errors)++;
    }
}
