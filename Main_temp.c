    #include <stdio.h>
    #include <stdlib.h> // atof , atoi
    #include <math.h>
    #include <unistd.h> // getopt
    #include <stdint.h>
    #include <string.h>
    #include <time.h>
    #include "demodulate.h"
    #include "Generator.h"

    extern char *optarg;

    typedef struct {
        double total_time;
        double min_lat;
        double max_lat;
        uint64_t count;
        uint64_t data_size;
        const char* name;
    } BlockStats;

    // Initialisation d'un bloc de stats
    void init_stats(BlockStats* s, const char* name, size_t size){
        s->total_time = 0;
        s->min_lat = 1e20; 
        s->max_lat = 0;
        s->count = 0;
        s->data_size = size;
        s->name = name;
    }

    void update_stats(BlockStats* s, double time){
        s->total_time += time;
        if(time < s->min_lat) s->min_lat = time;
        if(time > s->max_lat) s->max_lat = time;
        s->count++;
    }

    double get_time_sec(){
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    }

    #ifdef ENABLE_STATS
        #define STATS_START() t1 = get_time_sec()
        #define STATS_END(stat_block) t2 = get_time_sec(); \
        update_stats(stat_block,(double)(t2 - t1));
    #else 
        #define STATS_START() // Ne fait rien
        #define STATS_END(stat_block) // ne fait rien 
    #endif 






    int main(int argc, char* argv[]){

        float min_snr = 0.0f, max_snr = 0.0f, stp = 0.0f;
        unsigned int nb_error = 0, info_bits = 0, codeword_size = 0;
        char* decoder = NULL;
        char* nom_fichier = NULL;
        double t1, t2;
        int src_all_zeros=0;
        int opt;
        // "m:M:s:e:K:N:D:o:" -> les ":" indiquent que l'option attend un argument
        while ((opt = getopt(argc, argv, "m:M:s:e:K:N:D:o:z:")) != -1) {
            switch (opt){
            case 'm':
                min_snr = atof(optarg);
                break;
            case 'M':
                max_snr = atof(optarg);
                break;
            case 's':
                stp = atof(optarg);
                break;
            case 'e':
                nb_error = (unsigned int)atoi(optarg);
                break;
            case 'K':
                info_bits = (unsigned int)atoi(optarg);
                break;
            case 'N':
                codeword_size = (unsigned int)atoi(optarg);
                break;
            case 'D':
                decoder = optarg;
                break;
            case 'o':
                nom_fichier = optarg;
                break;      
            case 'z':
                src_all_zeros = atoi(optarg);
                break;            
            default:
                fprintf(stderr, "Option inconnue: %c\n", opt);
                return -1;
            }
        }

        // Vérification : toutes les options doivent être fournies
        if ( max_snr == 0.0f || stp == 0.0f || nb_error == 0 || info_bits == 0 || codeword_size == 0 || decoder == NULL) {
            fprintf(stderr, "ERREUR : Toutes les options (-m, -M, -s, -e, -K, -N, -D) sont requises\n");
            printf("Paramètres : m=%.2f, M=%.2f, s=%.2f, e=%u, K=%u, N=%u, D=%s\n", min_snr, max_snr, stp, nb_error, info_bits, codeword_size, decoder);
            return -1;
        }

        if (codeword_size % info_bits != 0){
            fprintf(stderr, "ERREUR : N (%u) doit être un multiple de K (%u)\n", codeword_size, info_bits);
            return -1;
        }

        printf("Paramètres : m=%.2f, M=%.2f, s=%.2f, e=%u, K=%u, N=%u, D=%s\n", min_snr, max_snr, stp, nb_error, info_bits, codeword_size, decoder);
        
        // Ouverture du fichier CSV
        FILE *csv_file = NULL;
        if (nom_fichier != NULL) {
            csv_file = fopen(nom_fichier, "w");
            if (csv_file == NULL) {
                perror("Erreur lors de l'ouverture du fichier");
                return EXIT_FAILURE;
            }
            // En-tête CSV
            fprintf(csv_file, "Eb/N0(dB),Es/N0(dB),sigma,be,fe,fn,BER,FER,Time(s),AvgTime(s),debit(Mbps)\n");
        }

        #ifndef ENABLE_STATS 
            BlockStats s_src, s_enc, s_mod, s_chn, s_dem, s_dec, s_mon;
            init_stats(&s_src, "Source"     , info_bits);
            init_stats(&s_enc, "Encoder"    , codeword_size);
            init_stats(&s_mod, "Modulator"  , codeword_size);
            init_stats(&s_chn, "Channel"    , codeword_size);
            init_stats(&s_dem, "Demodulator", codeword_size);
            init_stats(&s_dec, "Decoder"    , info_bits);
            init_stats(&s_mon, "Monitor"    , info_bits);
        #endif






        uint8_t *U_K = malloc(info_bits * sizeof(uint8_t)); // génèrer
        uint8_t *V_K = malloc(info_bits * sizeof(uint8_t)); // votes

        uint8_t *C_N = malloc(codeword_size * sizeof(uint8_t)); // rep de U_K
        int32_t *X_N = malloc(codeword_size * sizeof(int32_t)); // conversion de C_N pour la physique
        float *Y_N   = malloc(codeword_size * sizeof(float));   // ajout du bruit 
        float *L_N   = malloc(codeword_size * sizeof(float));   // exagérer la valeur Y_N

        if (U_K == NULL || V_K == NULL || C_N == NULL || X_N == NULL || Y_N == NULL || L_N == NULL) {
            fprintf(stderr, "Erreur : Échec de l'allocation mémoire.\n");
            free(U_K); free(V_K); free(C_N); free(X_N); free(Y_N); free(L_N);
            return EXIT_FAILURE;
        }
        
        size_t n_reps = codeword_size / info_bits;

        float R = (float)info_bits / (float)codeword_size;























        // Balayage des SNR
        for(float current_snr = min_snr; current_snr <= max_snr; current_snr+=stp){

            /* --------------------------------------------------------------------
            PHASE D'INITALISATION 
            */ --------------------------------------------------------------------

            BlockStats s_src, s_enc, s_mod, s_chn, s_dem, s_dec, s_mon;

            printf("début de simulation %f\n", current_snr);
            uint64_t be = 0, fe = 0, fn = 0;
            float es_n0_db = current_snr + 10*log10(R);

            float sigma = sqrt(1.0/   (2.0*pow(10.0, es_n0_db / 10.0))  );

            
            double start_time = get_time_sec();

            do {
                STATS_START();
                if(src_all_zeros) source_generate_all_zeros(U_K, info_bits);
                else              source_generate(U_K, info_bits);
                STATS_END(&s_src);

                STATS_START();
                codec_repetition_encode(U_K, C_N, info_bits, n_reps);
                STATS_END(&s_enc);

                STATS_START();
                modem_BPSK_modulate(C_N, X_N, codeword_size);
                STATS_END(&s_mod);

                STATS_START();
                channel_AWGN_add_noise(X_N, Y_N, codeword_size, sigma);
                STATS_END(&s_chn);
                
                STATS_START();
                modem_BPSK_demodulate(Y_N, L_N, codeword_size, sigma);
                STATS_END(&s_dem);

                STATS_START();
                if (strcmp(decoder, "rep-hard") == 0){
                    codec_repetition_hard_decode(L_N, V_K, info_bits, n_reps);
                } else {
                    codec_repetition_soft_decode(L_N, V_K, info_bits, n_reps);
                }
                STATS_END(&s_dec);          

                STATS_START();
                monitor_check_errors(U_K, V_K, info_bits, &be, &fe);
                STATS_END(&s_mon);

                // Incrémentation du nombre de trames simulées
                fn++;
            }  while (fe < nb_error); // boucle tant que nombre d'erreurs non atteint
            
            // Arrêt du chronomètre et calcul du temps écoulé 
            double end_time = get_time_sec();



            double time_tout = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            double avg_time_per_frame = time_tout / fn;

            double sim_thr = (double)(fn * info_bits) / (time_tout * 1e6);
            // Calcul des taux d'erreurs 
            double bit_error_rate = (double)be / (fn * info_bits);
            double frame_error_rate = (double)fe / fn;

            printf("SNR: %.2f | FER: %e | Thr: %.2f Mbps | Time: %.2fs\n", 
                    current_snr, frame_error_rate, sim_thr, time_tout);

            if (csv_file != NULL) {
                fprintf(csv_file, "%.2f,%.2f,%e,%lu,%lu,%lu,%e,%e,%f,%f,%f\n", 
                        current_snr, es_n0_db, sigma, be, fe, fn, 
                        bit_error_rate, frame_error_rate, time_tout, avg_time_per_frame, sim_thr);
            }

            // On affiche aussi le résumé: SNR | BER | FER | Nombre de trames simulées
            printf("fin du tour for %8.2f, be=%12e  fe=%12e  fn=%10lu\n", current_snr, bit_error_rate, frame_error_rate, fn);

            double somme_petit_composant = 0;

            #ifdef ENABLE_STATS
                // Affichage des statistiques à la fin du point SNR
                BlockStats* all_stats[] = {&s_src, &s_enc, &s_mod, &s_chn, &s_dem, &s_dec, &s_mon};
                
                printf("\n--- Per-Block Stats for SNR %.2f dB ---\n", current_snr);
                printf("%-10s | %-10s | %-10s | %-10s | %-10s | %-6s, %-10s\n", 
                    "Block", "Avg Lat(s)", "Min Lat(s)", "Max Lat(s)", "Thr(Mbps)", "% Time_tout", "Time_composant");
                printf("------------------------------------------------------------------------\n");
                
                for(int i = 0; i < 7; i++) {
                    BlockStats* s = all_stats[i];
                    somme_petit_composant += s->total_time;
                    double avg_lat = s->total_time / s->count;
                    double thr_mbps = (s->count * s->data_size) / (s->total_time * 1e6);
                    double pct_time = (s->total_time / time_tout) * 100.0;

                    printf("%-10s | %-10.2e | %-10.2e | %-10.2e | %-10.2f | %-5.2f%% | %f\n", 
                        s->name, avg_lat, s->min_lat, s->max_lat, thr_mbps, pct_time, s->total_time);
                }
                printf("------------------------------------------------------------------------\n\n");
                printf(" la somme des temps des petits composants : %f\n",somme_petit_composant);
            #endif
            }
        free(U_K);
        free(C_N);
        free(X_N);
        free(Y_N);
        free(L_N);
        free(V_K);
        
        if (csv_file != NULL) {
            fclose(csv_file);
            printf("Résultats sauvegardés dans : %s\n", nom_fichier);
        }


        return 0;
    }