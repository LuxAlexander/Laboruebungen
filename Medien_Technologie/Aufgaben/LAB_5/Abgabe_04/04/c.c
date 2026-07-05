#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x264.h>
#include <time.h>

/*
    Neu: zerolatency
    Ausführung: make->z.B. ./OKÜ04c.exe foreman.yuv 352 288 output.h264
    Ausgabe: Statistiken (Delay mean 4-8ms, max 11-25ms)
	
    Ausgangsbasis File LB-OKÜ 04 b
    
    Delay needs to be less than 1000ms/30=>33ms
    Easiest way is through the encoder options (standard tuning)
    
    https://code.videolan.org/videolan/x264/-/blob/master/x264.h?ref_type=heads
    available tunings are: "film", "animation", "grain", "stillimage", "psnr", "ssim", "fastdecode", "zerolatency", 0 
    
    https://forum.doom9.org/showthread.php?t=164688
    - zerolatency:
    --bframes 0 --force-cfr --no-mbtree
    --sync-lookahead 0 --sliced-threads
    --rc-lookahead 0
    
    zerolatency options: 
    force-cfr: timecodes are copied from the input file, provided the output file is not raw. 
    sync-lookahead: sets the number of frames to be used as a buffer.
    sliced-threads: enables slice-based threading (worse quality but adds no latency).
    bframes: sets the number of concurrent B-frames (0 disables bidirectional prediction, removing reordering delay).
    no-mbtree: disables macroblock tree ratecontrol (avoids tracking lookahead data across multiple frames).
    rc-lookahead: sets the number of frames to look ahead for ratecontrol

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
    // NEU: zerolatency, if needed can be pushed closer to 30ms by using slow
    if (x264_param_default_preset(&param, "medium","zerolatency") < 0) {
        fprintf(stderr, "Fehler beim Setzen des Presets.\n");
        fclose(file_in);
        fclose(file_out);
        return EXIT_FAILURE;
    }
    /* If I wanted to I could also change the delay time throug parameter
    param.i_frame_reference = 2;
    param.analyse.i_me_method = X264_ME_TESA;
    param.analyse.i_me_range = 64;
    param.analyse.i_subpel_refine = 2;
	*/
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

    //Statistik-Variablen
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
            
            // Werte erhöhen und Max checken, anhand IS_X264_TYPE_X
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
            
            // Same fuer Flush
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
    //Mean
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
	
    //Statistik
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
