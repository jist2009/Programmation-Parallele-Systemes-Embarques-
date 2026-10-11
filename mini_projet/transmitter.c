#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arm_neon.h>
#include "project.h"

// write into the buffer U_K
// void source_generate(uint8_t *U_K, size_t K){
//     for (size_t i = 0; i < K/8; i++) {
//         U_K[i] = rand() % 256;  // 8 bits aléatoires dans 1 octet
//     }
// }

void source_generate(uint8_t *U_K, size_t K){
    for (size_t i = 0; i < K; i++) {
        U_K[i] = rand() % 2 ; // Génère un nombre aléatoire : 0 ou 1
    }
}


// read from the buffer U_K and write into the buffer C_N
// void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps){
//     for (size_t i = 0; i < n_reps; i++) {
//         for (size_t j =0; j < K/8; j++) {
//             C_N[i*K/8 + j] = U_K[j]; // Répète U_K n_reps fois dans C_N
//         }
//     }
// }

void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps){
    for (size_t i = 0; i < n_reps; i++) {
        for (size_t j =0; j < K; j++) {
            C_N[i*K + j] = U_K[j]; // Répète U_K n_reps fois dans C_N
        }
    }
}


// read from C_N, write into X_N
void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N) {
    
    int32x4_t v_one = vdupq_n_s32(1);  
    int32x4_t v_two = vdupq_n_s32(2);  

    for (size_t i = 0; i <= N; i += 4) {
        // Charger 4 valeurs de C_N dans un registre int32
        int32x4_t c = {C_N[i], C_N[i+1], C_N[i+2], C_N[i+3]};
        // Calculer 1 - 2*c  donne  +1 si c=0,  -1 si c=1
        int32x4_t x = vsubq_s32(v_one, vmulq_s32(v_two, c));
        // Stocker dans X_N
        vst1q_s32(X_N + i, x);
    }
}


// void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N) {
//     int32x4_t v_one = vdupq_n_s32(1);  
//     int32x4_t v_two = vdupq_n_s32(2);
//     for (size_t i = 0; i <= N/8; i += 16) {
//         //extraire 16 octets 
//         uint8x16_t bytes = vld1q_u8(C_N + i);
//         // traiter chaque octet 
//         for (size_t j = 0; j < 16 ; j++){
//             uint8x16_t byte = vgetq_lanes(bytes, j);
//             //extraire les bits LSB
//             int32x4_t bits_lo = {(byte >> 0) & 1, (byte >> 1) & 1, (byte >> 2) & 1, (byte >> 3) & 1};
//             //extraire les bits MSB
//             int32x4_t bits_hi = {(byte >> 4) & 1, (byte >> 5) & 1, (byte >> 6) & 1, (byte >> 7) & 1};
//             // Calculer 1 - 2*c  donne  +1 si c=0,  -1 si c=1
//             int32x4_t x_lo = vsubq_s32(v_one, vmulq_s32(v_two, bits_lo));
//             int32x4_t x_hi = vsubq_s32(v_one, vmulq_s32(v_two, bits_hi));
//             // Stocker dans X_N
//             vst1q_s32(X_N + (i+j)*8    , x_lo);
//             vst1q_s32(X_N + (i+j)*8 + 4, x_lo);
//         }

//     }
// }