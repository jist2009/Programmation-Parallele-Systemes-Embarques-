#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <arm_neon.h>
#include "project.h"

// demodulator, just copies Y_N in L_N for now
void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma){
    // calculer la constante une seule fois hors de la boucle
    float scale = 2.0f / (sigma * sigma);

    // Remplir un registre avec cette constante
    float32x4_t v_scale = vdupq_n_f32(scale);

    for (size_t i = 0; i <= N; i += 4) {
        // Charger 4 floats de Y_N 
        float32x4_t y = vld1q_f32(Y_N + i);
        // Multiplier par la constante 
        float32x4_t l = vmulq_f32(y, v_scale);
        // Stocker 4 floats dans L_N 
        vst1q_f32(L_N + i, l);
    }
}

void quantizer_transform8(const float *L_N, int8_t *L8_N, size_t N, size_t s, size_t f) {
    int8_t max_val =  (1 << (s-1)) - 1;  //  valeur maximale
    int8_t min_val = -(1 << (s-1));      //  valeur minimale
    float scale = (float)(1 << f);       // facteur d'échelle

    for (size_t i = 0; i < N; i++) {
        float val = L_N[i] * scale;
        int32_t rounded = (int32_t)roundf(val);    // arrondir à l'entier le plus proche
        if      (rounded > max_val) rounded = max_val;
        else if (rounded < min_val) rounded = min_val;
        L8_N[i] = (int8_t)rounded;
    }
}


void codec_repetition_hard_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps) {
    
    int8x16_t ones = vdupq_n_s8(1);
    
    // boucle sur les groupes de 16 bits
    for (size_t i = 0; i < K; i += 16) {
        
        // initialiser les votes à 0 
        int8x16_t sum_votes = vdupq_n_s8(0);
        
        // boucle sur les répétitions
        for (size_t j = 0; j < n_reps; j++) {
            
            // charger 16 LLR de la répétition j
            int8x16_t val = vld1q_s8(&L8_N[j*K + i]);
            
            // masque : -1 si négatif, 0 si positif
            int8x16_t mask = (int8x16_t)vcltzq_s8(val);
            
            // transformer en -1 ou +1
            int8x16_t vote = vaddq_s8(vaddq_s8(mask, mask), ones);
            
            // accumuler les votes
            sum_votes = vaddq_s8(sum_votes, vote);
        }
        
        // si sum_votes < 0 → majorité négative → bit = 1
        uint8x16_t final_mask = vcltzq_s8(sum_votes);
        
        // convertir FF→1 et 00→0
        int8x16_t res = vandq_s8((int8x16_t)final_mask, ones);
        
        // stocker dans V_K
        vst1q_s8((int8_t*)&V_K[i], res);
    }
}

void codec_repetition_soft_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps) {
    int8x16_t ones = vdupq_n_s8(1);    
    for (size_t i = 0; i < K; i += 16) {        
        // initialiser la somme à 0
        int8x16_t sum = vdupq_n_s8(0);        
        // boucle sur les répétitions
        for (size_t j = 0; j < n_reps; j++) {           
            // charger 16 LLR de la répétition j
            int8x16_t val = vld1q_s8(&L8_N[j*K + i]);            
            // additionner avec saturation
            sum = vqaddq_s8(sum, val);
        }
        
        // si sum < 0 → bit = 1
        uint8x16_t mask = vcltzq_s8(sum);
        
        // convertir FF→1 et 00→0
        int8x16_t res = vandq_s8((int8x16_t)mask, ones);
        
        // stocker dans V_K
        vst1q_s8((int8_t*)&V_K[i], res);
    }
}

// update `n_bit_errors` and `n_frame_errors` variables depending on `U_K` and `V_K`
void monitor_check_errors(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors){

    uint64_t bit_errors = 0;
    int frame_error = 0;
    int cpt = 0;
    uint8_t tmp[16];

    // Accumulateur pour compter les erreurs bit
    uint8x16_t vacc = vdupq_n_u8(0);  

    for (size_t i = 0; i <= K; i += 16) {
        // Charger 16 octets de U_K et V_K 
        uint8x16_t u = vld1q_u8(U_K + i);
        uint8x16_t v = vld1q_u8(V_K + i);

        // XOR : 1 quand les bits sont différents 
        uint8x16_t diff = veorq_u8(u, v);

        // Accumuler les erreurs bit
        vacc = vaddq_u8(vacc, diff);
        cpt++;
        if (cpt == 255) {
	    vst1q_u8(tmp, vacc);
	    for (int j = 0; j < 16; j++) {
	        bit_errors += tmp[j];
	        if (tmp[j] > 0) frame_error = 1;
	   }
	   vacc  = vdupq_n_u8(0);
	   cpt = 0;
    	}	
    }
    vst1q_u8(tmp, vacc);
    for (int j = 0; j < 16; j++) {
        bit_errors += tmp[j];
        if (tmp[j] > 0) frame_error = 1;
   }
    *n_bit_errors   += bit_errors;
    *n_frame_errors += frame_error;
}
