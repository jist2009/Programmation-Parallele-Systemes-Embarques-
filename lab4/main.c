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
    float SNR_min = 0;
    float SNR_max = 0;
    float step = 0;
    size_t fmax = 0;
    // Paramètres du système
    size_t K = 0 ;			//nombre de bits du message 
    size_t N = 0;  			//La taille du mot de code doit etre multiple de K 
    char    decoder[16];
    int num_simu = 0;
    int src_all_zeros = 0 ;
    int mod_all_ones = 0;
    size_t qf = 0 ;
    size_t qs = 8;  
    int use_quantizer = 0;
    
    // input arguments 
    int opt;
    while ((opt = getopt(argc, argv, "m:M:s:e:K:N:D:o:zqf:S:")) != -1) {
        switch (opt) {
            case 'm': SNR_min = atof(optarg);                        break;
            case 'M': SNR_max = atof(optarg);                        break;
            case 's': step    = atof(optarg);                        break;
            case 'e': fmax    = (size_t)atoi(optarg);                break;
            case 'K': K       = (size_t)atoi(optarg);                break;
            case 'N': N       = (size_t)atoi(optarg);                break;
            case 'D': strncpy(decoder, optarg, sizeof(decoder)-1);   break;
            case 'o': num_simu = atoi(optarg);                       break;
            case 'z': src_all_zeros = 1;                             break;
            case 'q': mod_all_ones = 1;                              break;                            
            case 'f': qf = (size_t)atoi(optarg); use_quantizer = 1; break;
            case 'S': qs = (size_t)atoi(optarg);                    break;
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
    // Vérification des paramètres de quantification
    if (use_quantizer) {
        if (qs < 1) {
	    fprintf(stderr, "Attention : qs (%zu) < 1, corrige a 1\n", qs);
	    qs = 1;
        }
    	if (qs > 8) {
	    fprintf(stderr, "Attention : qs (%zu) > 8, corrige a 8\n", qs);
	    qs = 8;
        }
        if (qf > qs) {
	    fprintf(stderr, "Attention : qf (%zu) > qs (%zu), corrige a qs\n", qf, qs);
	    qf = qs;
        }
    }
    
    // variable 
    float R = (float)K / (float)N;
    size_t n_reps = N / K;
    
    // Ajouter le fichier CSV
    char filename[64];
    snprintf(filename, sizeof(filename), "sim%d.csv",num_simu);
    
    // Allocation des buffers
    uint8_t *U_K = malloc(K * sizeof(uint8_t));
    uint8_t *C_N = malloc(N * sizeof(uint8_t));
    int32_t *X_N = malloc(N * sizeof(int32_t));
    float   *Y_N = malloc(N * sizeof(float));
    float   *L_N = malloc(N * sizeof(float));
    uint8_t *V_K = malloc(K * sizeof(uint8_t));
    int8_t *L8_N = malloc(N * sizeof(int8_t));
    
    // Ouvrir le fichier CSV 
    FILE *csv = fopen(filename, "w");
    if (csv == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir %s\n", filename);
        return 1;
    }
    // Écrire l'en-tête du CSV
    fprintf(csv, "Eb/N0(dB),Es/N0(dB),sigma,be,fe,fn,BER,FER,time(s),AvgTime(s),Sim_thr(Mbps)\n");
    
    #ifdef ENABLE_STATS
        // Ouvrir le fichier CSV des stats par bloc
        char filename_stats[64];
        snprintf(filename_stats, sizeof(filename_stats), "stats_sim%d.csv", num_simu);
        FILE *csv_stats = fopen(filename_stats, "w");
        if (csv_stats == NULL) {
            fprintf(stderr, "Erreur : impossible d'ouvrir stats_blocs.csv\n");
            return 1;
        }
    #endif

    // initialiser U_K à 0 lorsque la source est désactivé 
    if (mod_all_ones) memset(U_K, 0, K * sizeof(uint8_t));
    
    // Boucle externe sur les SNR
    for (float Eb_N0 = SNR_min; Eb_N0 <= SNR_max; Eb_N0 += step){
    	float Es_N0 = Eb_N0 + 10.0f * log10f(R);  // bs =1 
    	float sigma = sqrtf(1.0f / (2.0f * powf(10.0f, Es_N0 / 10.0f)));
    	uint64_t be = 0;  // bit errors
    	uint64_t fe = 0;  // frame errors
    	uint64_t fn = 0;  // nombre de trames simulées
    	// mesure du temps avant la boucle
        clock_t start = clock();

        #ifdef ENABLE_STATS
            // variables pour mesurer le temps de chaque bloc avec une precision en nanosecondes
            struct timespec t1, t2;
            double lat = 0;
            // pour chaque bloc : 
            // sum = somme des latences 
            // min = latence minimale 
            // max = latence maximale 
            double src_sum=0, src_min=1e18, src_max=0;
            double enc_sum=0, enc_min=1e18, enc_max=0;
            double mod_sum=0, mod_min=1e18, mod_max=0;
            double chn_sum=0, chn_min=1e18, chn_max=0;
            double dem_sum=0, dem_min=1e18, dem_max=0;
            double dec_sum=0, dec_min=1e18, dec_max=0;
            double mon_sum=0, mon_min=1e18, mon_max=0;
        #endif
    	
    	// boucle interne Mote Carlo
    	do {
    	    // Émetteur 
            // --- Source ---
            // task 3
            if (!mod_all_ones) {
                #ifdef ENABLE_STATS
                    clock_gettime(CLOCK_MONOTONIC, &t1);
                #endif
                // task2
                    if(src_all_zeros) source_generate_all_zeros(U_K, K);
                    else              source_generate(U_K, K);
                    
                #ifdef ENABLE_STATS
                    clock_gettime(CLOCK_MONOTONIC, &t2);
                    lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                    src_sum += lat;
                    if (lat < src_min) src_min = lat;
                    if (lat > src_max) src_max = lat;
                #endif
            }
	        
    	    // --- Encodeur ---
            // task 3
            if (!mod_all_ones){
                #ifdef ENABLE_STATS
                    clock_gettime(CLOCK_MONOTONIC, &t1);
                #endif
                    codec_repetition_encode(U_K, C_N, K, n_reps);
                #ifdef ENABLE_STATS
                    clock_gettime(CLOCK_MONOTONIC, &t2);
                    lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                    enc_sum += lat;
                    if (lat < enc_min) enc_min = lat;
                    if (lat > enc_max) enc_max = lat;
                #endif
            }

            // --- Modulateur ---
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t1);
            #endif
                // task 3
                if (mod_all_ones)
                    modem_BPSK_modulate_all_ones(C_N, X_N, N);
                else
                    modem_BPSK_modulate(C_N, X_N, N);
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t2);
                lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                mod_sum += lat;
                if (lat < mod_min) mod_min = lat;
                if (lat > mod_max) mod_max = lat;
            #endif
    	    
    	    // AWGN Channal 
    	    #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t1);
            #endif
                channel_AWGN_add_noise(X_N, Y_N, N, sigma);
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t2);
                lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                chn_sum += lat;
                if (lat < chn_min) chn_min = lat;
                if (lat > chn_max) chn_max = lat;
            #endif
    	    
    	    // Récepteur 
            // --- Démodulateur ---
    	    #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t1);
            #endif
                modem_BPSK_demodulate(Y_N, L_N, N, sigma);
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t2);
                lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                dem_sum += lat;
                if (lat < dem_min) dem_min = lat;
                if (lat > dem_max) dem_max = lat;
            #endif

            // Quantification 
            if (use_quantizer) quantizer_transform8(L_N, L8_N, N, qs, qf);
            // --- Décodeur ---
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t1);
            #endif
                if (strcmp(decoder,"rep-hard") == 0) 
                    codec_repetition_hard_decode(L_N, V_K, K, n_reps);
                else if (strcmp(decoder,"rep-soft") == 0) 
                    codec_repetition_soft_decode(L_N, V_K, K, n_reps);
                else if (strcmp(decoder,"rep-hard8") == 0)
                    codec_repetition_hard_decode8(L8_N, V_K, K, n_reps);
                else if (strcmp(decoder,"rep-soft8") == 0)
                    codec_repetition_soft_decode8(L8_N, V_K, K, n_reps);
                else if (strcmp(decoder,"rep-hard8-neon") == 0)
                    codec_repetition_hard_decode8_neon(L8_N, V_K, K, n_reps);
                else if (strcmp(decoder,"rep-soft8-neon") == 0)
                    codec_repetition_soft_decode8_neon(L8_N, V_K, K, n_reps);
                
            
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t2);
                lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                dec_sum += lat;
                if (lat < dec_min) dec_min = lat;
                if (lat > dec_max) dec_max = lat;
            #endif
    	    
            
            // Monitor 	
    	    #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t1);
            #endif
                monitor_check_errors(U_K, V_K, K, &be, &fe);
            #ifdef ENABLE_STATS
                clock_gettime(CLOCK_MONOTONIC, &t2);
                lat = ((t2.tv_sec-t1.tv_sec)*1e6 + (t2.tv_nsec-t1.tv_nsec)/1000.0);
                mon_sum += lat;
                if (lat < mon_min) mon_min = lat;
                if (lat > mon_max) mon_max = lat;
            #endif


    	    fn++;  	
    	}while(fe < fmax);
    	
    	// mesure du temps après la boucle
        clock_t end = clock();
        double time_total     = (double)(end - start) / CLOCKS_PER_SEC;
        double time_moy_frame = time_total / (double)fn;
        // e débit du simulateur en Mbps **Task0**
        double Sim_thr = ((double)fn * (double)K) / time_total / 1e6; // en Mbps
        
    	// Calcule de BER et FER 
    	float BER = (float)be / ((float)fn * (float)K);
        float FER = (float)fe / (float)fn;
        
        // Écriture dans le CSV 
        fprintf(csv, "%.1f,%.1f,%.6f,%lu,%lu,%lu,%e,%e,%.6f,%.9f,%.3f\n",
                Eb_N0, Es_N0, sigma, be, fe, fn, BER, FER,
                time_total, time_moy_frame,Sim_thr); 	
        
        fflush(csv);
        // Affichage terminal pour suivre la progression
        fprintf(stderr, "SNR=%.1f dB | FER=%.2e | BER=%.2e | Sim_thr=%.3f Mbps | fn=%lu\n",
                Eb_N0, FER, BER, Sim_thr, fn);

        #ifdef ENABLE_STATS
            double total_us = src_sum + enc_sum + mod_sum + chn_sum
                            + dem_sum + dec_sum + mon_sum;
            
            // titre pour séparer pour ce SNR               
            fprintf(csv_stats, "\nSNR = %.1f dB\n", Eb_N0);
            fprintf(csv_stats, "%-15s | %10s | %10s | %10s | %12s | %10s\n",
                    "Bloc", "Moy(us)", "Min(us)", "Max(us)", "Debit(Mbps)", "%(temps)");
            fprintf(csv_stats, "---------------+-----------+-----------+-----------+--------------+-----------\n");

        #define WRITE_STATS(nom, sum, mn, mx, bits) \
            fprintf(csv_stats, "%-15s | %10.3f | %10.3f | %10.3f | %12.3f | %9.1f%%\n", \
                    nom,\
                    (sum)/(double)fn, \
                    (sum) > 0 ? (mn) : 0.0, \
                    (sum) > 0 ? (mx) : 0.0, \
                    (sum) > 0 ? ((double)fn*(bits))/((sum)*1e-6)/1e6 : 0.0, \
                    total_us > 0 ? (sum)/total_us*100.0 : 0.0)

            WRITE_STATS("Source",       src_sum, src_min, src_max, K);
            WRITE_STATS("Encodeur",     enc_sum, enc_min, enc_max, N);
            WRITE_STATS("Modulateur",   mod_sum, mod_min, mod_max, N);
            WRITE_STATS("Canal",        chn_sum, chn_min, chn_max, N);
            WRITE_STATS("Demodulateur", dem_sum, dem_min, dem_max, N);
            WRITE_STATS("Decodeur",     dec_sum, dec_min, dec_max, K);
            WRITE_STATS("Moniteur",     mon_sum, mon_min, mon_max, K);

            fflush(csv_stats);

            #undef WRITE_STATS
        #endif

        
    } 
    #ifdef ENABLE_STATS
        fclose(csv_stats);
    #endif  
    fclose(csv);
    
    //Libération mémoire
    free(U_K); free(C_N); free(X_N);
    free(Y_N); free(L_N); free(V_K);
    free(L8_N);
    return 0;
}
