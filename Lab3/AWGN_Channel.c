#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "project.h"


// add white Gaussian noise
void channel_AWGN_add_noise(const int32_t *X_N, float *Y_N, size_t N, float sigma){
    for (size_t i = 0; i < N; i++) {
        // Générer deux deux variables uniformes 𝑈1 et 𝑈2 dans l’intervalle ]0,1[
        float U1 = (rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
        float U2 = (rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
        // Utiliser la méthode de Box-Muller pour générer une variable gaussienne standard
        float Z0 = sqrtf(-2.0f * logf(U1)) * cosf(2.0f * (float)M_PI * U2);
        // Ajouter le bruit gaussien à la valeur du signal
        Y_N[i] = (float)X_N[i] + sigma * Z0;
    }
}
