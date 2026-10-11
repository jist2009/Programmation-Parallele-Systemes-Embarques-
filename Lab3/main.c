// binome : Rabab Boudih & Stéphane Ji
	

#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h> 
#include "project.h"

int main(int argc, char *argv[]) {
    //Initialisation du générateur pseudo-aléatoire
    srand(time(NULL));
    
    // Paralètres du Signal
    float SNR_min, SNR_max, step;
    size_t fmax;
    // Paramètres du système
    size_t K;			//nombre de bits du message 
    size_t N;  			//La taille du mot de code doit etre multiple de K 
    char    decoder[16];
    
    // input arguments 
    int opt;
    while ((opt = getopt(argc, argv, "m:M:s:e:K:N:D:")) != -1) {
        switch (opt) {
            case 'm': SNR_min = atof(optarg);                        break;
            case 'M': SNR_max = atof(optarg);                        break;
            case 's': step    = atof(optarg);                        break;
            case 'e': fmax    = (size_t)atoi(optarg);                break;
            case 'K': K       = (size_t)atoi(optarg);                break;
            case 'N': N       = (size_t)atoi(optarg);                break;
            case 'D': strncpy(decoder, optarg, sizeof(decoder)-1);   break;
            default:
                fprintf(stderr, "Usage: %s -m min -M max -s step -e fmax -K K -N N -D decoder\n", argv[0]);
                return 1;
        }
    }
    
    // Vérification N multiple de K
    if (N % K != 0) {
        fprintf(stderr, "Erreur : N (%zu) doit etre un multiple de K (%zu)\n", N, K);
        return 1;
    }
    
    // variable 
    float R = (float)K / (float)N;
    size_t n_reps = N / K;
    
    // Ajouter le fichier CSV
    char filename[64];
    snprintf(filename, sizeof(filename), "sim_K%zu_N%zu_%s.csv", K, N, decoder);
    
    // Allocation des buffers
    uint8_t *U_K = malloc(K * sizeof(uint8_t));
    uint8_t *C_N = malloc(N * sizeof(uint8_t));
    int32_t *X_N = malloc(N * sizeof(int32_t));
    float   *Y_N = malloc(N * sizeof(float));
    float   *L_N = malloc(N * sizeof(float));
    uint8_t *V_K = malloc(K * sizeof(uint8_t));
    
    // Ouvrir le fichier CSV
    FILE *csv = fopen(filename, "w");
    if (csv == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir %s\n", filename);
        return 1;
    }
    // Écrire l'en-tête du CSV
    fprintf(csv, "Eb/N0(dB),Es/N0(dB),sigma,be,fe,fn,BER,FER,time(s),AvgTime(s)\n");
    
    // Boucle externe sur les SNR
    for (float Eb_N0 = SNR_min; Eb_N0 <= SNR_max; Eb_N0 += step){
    	float Es_N0 = Eb_N0 + 10.0f * log10f(R);  // bs =1 
    	float sigma = sqrtf(1.0f / (2.0f * powf(10.0f, Es_N0 / 10.0f)));
    	uint64_t be = 0;  // bit errors
    	uint64_t fe = 0;  // frame errors
    	uint64_t fn = 0;  // nombre de trames simulées
    	// mesure du temps avant la boucle
        clock_t start = clock();
    	
    	// boucle interne Mote Carlo
    	do {
    	    // Émetteur 
	    source_generate(U_K, K);
    	    codec_repetition_encode(U_K, C_N, K, n_reps);
    	    modem_BPSK_modulate(C_N, X_N, N);
    	    
    	    // AWGN Channal 
    	    channel_AWGN_add_noise(X_N, Y_N, N, sigma);
    	    
    	    // Récepteur 
    	    modem_BPSK_demodulate(Y_N, L_N, N, sigma);
    	    if (strcmp(decoder,"rep-hard") == 0) 
    	    	codec_repetition_hard_decode(L_N, V_K, K, n_reps);
    	    else if (strcmp(decoder,"rep-soft") == 0) 
    	    	codec_repetition_soft_decode(L_N, V_K, K, n_reps);
    	    // Monitor 	
    	    monitor_check_errors(U_K, V_K, K, &be, &fe);
    	    fn++;  	
    	}while(fe < fmax);
    	
    	// mesure du temps après la boucle
        clock_t end = clock();
        double time_total     = (double)(end - start) / CLOCKS_PER_SEC;
        double time_moy_frame = time_total / (double)fn;
        
    	// Calcule de BER et FER 
    	float BER = (float)be / ((float)fn * (float)K);
        float FER = (float)fe / (float)fn;
        
        // Écriture dans le CSV 
        fprintf(csv, "%.1f,%.1f,%.6f,%lu,%lu,%lu,%e,%e,%.6f,%.9f\n",
                Eb_N0, Es_N0, sigma, be, fe, fn, BER, FER,
                time_total, time_moy_frame); 	
    }   
    fclose(csv);
    
    //Libération mémoire
    free(U_K); free(C_N); free(X_N);
    free(Y_N); free(L_N); free(V_K);
    
    return 0;
}
