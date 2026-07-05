#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x264.h>
#include <time.h> // NEU: clock_gettime und struct timespec

/*
    Neu: Statistik (Variablen,checkType,Ausgabe)
    Ausführung: make->z.B. ./OKÜ04b.exe foreman.yuv 352 288 output.h264
    Ausgabe: Statistik mit Anzahl des Frames, Mean, Max

    Ausgangsbasis LB-OKÜ 04 a
*/

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Bitte gib: %s <Pfad> <Breite> <Hoehe> <Ausgabe-Pfad> an\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    //konvertiert einen ASCII-String in eine Ganzzahl
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    char *out_filename = argv[4];

    if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
        fprintf(stderr, "Fehler: Ungueltige Dimensionen (muessen gerade und positiv sein).\n");
        return EXIT_FAILURE;
    }
    //read binary
    FILE *file_in = fopen(filename, "rb");
    if (!file_in) {
        perror("Fehler beim Oeffnen der Datei");
        return EXIT_FAILURE;
    }
    
    //Ausgabedatei öffnen
    FILE *file_out = fopen(out_filename, "wb");
    if (!file_out) {
        perror("Fehler beim Oeffnen der Ausgabedatei");
        fclose(file_in);
        return EXIT_FAILURE;
    }
    
    // Speicherberechnung für ein Frame im 4:2:0 Format
    //Y full, U 1/4, V 1/4
    size_t y_size = (size_t)width * height;
    size_t uv_size = (size_t)(width / 2) * (height / 2);//Viertel
    size_t frame_size = y_size + 2 * uv_size;//z.B. 352*288*1,5

    unsigned char *buffer = (unsigned char *)malloc(frame_size);
    if (!buffer) {
        fprintf(stderr, "Speicherfehler.\n");
        fclose(file_in);
        return EXIT_FAILURE;
    }
    
    //x264 Initialisierung
    x264_param_t param;
    x264_picture_t pic_in;
    x264_picture_t pic_out;
    x264_t *encoder;

    // Standard-Parameter setzen (Preset: medium, Tune: standard/none)
    if (x264_param_default_preset(&param, "medium", NULL) < 0) {
        fprintf(stderr, "Fehler beim Setzen des Presets.\n");
        fclose(file_in);
        fclose(file_out);
        return EXIT_FAILURE;
    }

    // Video-Eigenschaften konfigurieren
    param.i_csp = X264_CSP_I420; // IYUV / YUV 4:2:0 planar
    param.i_width = width;
    param.i_height = height;
    param.b_vfr_input = 0;       // Konstante Framerate
    param.i_fps_num = 25;        // Standard: 25 Bilder pro Sekunde
    param.i_fps_den = 1;

    // Basis-Profil erzwingen (erhoeht Kompatibilitaet)
    if (x264_param_apply_profile(&param, "baseline") < 0) {
        fprintf(stderr, "Fehler beim Anwenden des Profils.\n");
        fclose(file_in);
        fclose(file_out);
        return EXIT_FAILURE;
    }

    // Zuweisung des Eingangsbild-Puffers gemäss Farbraum
    if (x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height) < 0) {
        fprintf(stderr, "Fehler bei der Speicherallokation fuer das Bild.\n");
        fclose(file_in);
        fclose(file_out);
        return EXIT_FAILURE;
    }

    // Encoder oeffnen
    encoder = x264_encoder_open(&param);
    if (!encoder) {
        fprintf(stderr, "Fehler beim Oeffnen des Encoders.\n");
        x264_picture_clean(&pic_in);
        fclose(file_in);
        fclose(file_out);
        return EXIT_FAILURE;
    }

    int frame_count = 0;

    x264_nal_t *nals;
    int i_nals;

    // NEU: Statistik-Variablen
    int count_i = 0, count_p = 0, count_b = 0;
    double sum_i = 0.0, sum_p = 0.0, sum_b = 0.0;
    double max_i = 0.0, max_p = 0.0, max_b = 0.0;
    
    // Liest Frames nacheinander ein, solange ganze frames gelesen
    
    while (fread(buffer, 1, frame_size, file_in) == frame_size) {
        printf("Frame %d eingelesen:\n\n", frame_count);
        
        // Kopiert die Daten aus dem 'buffer' in die x264-Struktur
        //copy a specific number of bytes from one memory location to another. DEST,Source
        memcpy(pic_in.img.plane[0], buffer, y_size);
        memcpy(pic_in.img.plane[1], buffer + y_size, uv_size);
        memcpy(pic_in.img.plane[2], buffer + y_size + uv_size, uv_size);
        
        pic_in.i_pts = frame_count;

        // Zeitstempel vor Kodierung erstellen und an opaque übergeben
        struct timespec *start_time = malloc(sizeof(struct timespec));
        if (start_time != NULL) {
            clock_gettime(CLOCK_MONOTONIC, start_time);
        }
        pic_in.opaque = start_time;

        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
        if (frame_size_out > 0) {
            fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
            
            // NEU: Werte erhöhen und Max checken, anhand IS_X264_TYPE_X
            if (pic_out.opaque != NULL) {
                struct timespec end_time;
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                struct timespec *orig_start = (struct timespec *)pic_out.opaque;
                
                double delay_ms = (end_time.tv_sec - orig_start->tv_sec) * 1000.0 +
                                  (end_time.tv_nsec - orig_start->tv_nsec) / 1000000.0;
                
                if (IS_X264_TYPE_I(pic_out.i_type)) {
                    count_i++;
                    sum_i += delay_ms;
                    if (delay_ms > max_i) max_i = delay_ms;
                } else if (IS_X264_TYPE_B(pic_out.i_type)) {
                    count_b++;
                    sum_b += delay_ms;
                    if (delay_ms > max_b) max_b = delay_ms;
                } else {
                    count_p++;
                    sum_p += delay_ms;
                    if (delay_ms > max_p) max_p = delay_ms;
                }
                
                free(orig_start);
            }
        }
        
        frame_count++;
    }

    //Delayed Frames leeren (Flush
    while (x264_encoder_delayed_frames(encoder)) {
        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, NULL, &pic_out);
        if (frame_size_out > 0) {
            fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
            
            // NEU: Same fuer Flush
            if (pic_out.opaque != NULL) {
                struct timespec end_time;
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                struct timespec *orig_start = (struct timespec *)pic_out.opaque;
                
                double delay_ms = (end_time.tv_sec - orig_start->tv_sec) * 1000.0 +
                                  (end_time.tv_nsec - orig_start->tv_nsec) / 1000000.0;
                
                if (IS_X264_TYPE_I(pic_out.i_type)) {
                    count_i++;
                    sum_i += delay_ms;
                    if (delay_ms > max_i) max_i = delay_ms;
                } else if (IS_X264_TYPE_B(pic_out.i_type)) {
                    count_b++;
                    sum_b += delay_ms;
                    if (delay_ms > max_b) max_b = delay_ms;
                } else {
                    count_p++;
                    sum_p += delay_ms;
                    if (delay_ms > max_p) max_p = delay_ms;
                }
                
                free(orig_start);
            }
        }
    }
    // NEU: mean
    double mean_i=0.0,mean_p=0.0,mean_b=0.0;
	if(count_i>0){
		mean_i=sum_i / count_i;
	}
	if(count_p>0){
		mean_p=sum_p / count_p;
	}
	if(count_b>0){
		mean_b=sum_b / count_b;
	}
	
    // NEU: Statistik
    printf("STATISTIK:\n");
    printf("Frametyp \t Anzahl \t Durchschnitt Delay \t Max Delay \n");
    printf("I-Frames \t %d \t \t %.2f ms \t \t %.2f ms\n", count_i, mean_i, max_i);
    printf("P-Frames \t %d \t \t %.2f ms \t \t %.2f ms\n", count_p, mean_p, max_p);
    printf("B-Frames \t %d \t \t %.2f ms \t \t %.2f ms\n\n", count_b, mean_b, max_b);

    x264_encoder_close(encoder);
    x264_picture_clean(&pic_in);
    free(buffer);
    fclose(file_in);
    fclose(file_out);
    
    return EXIT_SUCCESS;
}
