#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x264.h>
#include <time.h> // NEU: clock_gettime und struct timespec

/*
    Neu: Zeitstempel und Differenz
    Ausführung: make->z.B. ./OKÜ04a.exe foreman.yuv 352 288 output.h264
    Ausgabe: Eingelesener Frame und zu jedem Frame das Delay.

    Ausgangsbasis File LB-MTM 04. a
    
    x264_picture_t (x264.h):
    void *opaque; private user data. copied from input to output frames.
   
    
    NiceToHave:
    https://code.videolan.org/videolan/x264/
    https://en.cppreference.com/c/chrono/clock
    
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
    
    // Liest Frames nacheinander ein, solange ganze frames gelesen
    
    while (fread(buffer, 1, frame_size, file_in) == frame_size) {
        printf("Frame %d eingelesen:\n\n", frame_count);

        // Frame an x264 übergeben und kodieren
        // Kopiert die Daten aus dem 'buffer' in die x264-Struktur
        //copy a specific number of bytes from one memory location to another. DEST,Source
        memcpy(pic_in.img.plane[0], buffer, y_size);
        memcpy(pic_in.img.plane[1], buffer + y_size, uv_size);
        memcpy(pic_in.img.plane[2], buffer + y_size + uv_size, uv_size);
        
        pic_in.i_pts = frame_count;//frame Dekodierungs-Zeitstempel

        // NEU: Zeitstempel vor Kodierung erstellen und in opaque schreiben
        struct timespec *start_time = malloc(sizeof(struct timespec));
        if (start_time != NULL) {
            clock_gettime(CLOCK_MONOTONIC, start_time);
        }
        pic_in.opaque = start_time;

        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
        if (frame_size_out > 0) {
            fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
            
            // NEU: Zeitdifferenz End-Start
            if (pic_out.opaque != NULL) {
                struct timespec end_time;
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                struct timespec *orig_start = (struct timespec *)pic_out.opaque;
                
                //1s=10³ms, 1ms=10⁶ns
                double delay_ms = (end_time.tv_sec - orig_start->tv_sec) * 1000.0 +
                                  (end_time.tv_nsec - orig_start->tv_nsec) / 1000000.0;
                
                printf("\t Frame PTS %lld: Kodierungsdelay = %.2f ms\n", (long long)pic_out.i_pts, delay_ms);
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
            
            // NEU: Zeitdifferenz bei Flush
            if (pic_out.opaque != NULL) {
                struct timespec end_time;
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                struct timespec *orig_start = (struct timespec *)pic_out.opaque;
                
                double delay_ms = (end_time.tv_sec - orig_start->tv_sec) * 1000.0 +
                                  (end_time.tv_nsec - orig_start->tv_nsec) / 1000000.0;
                
                //i_pts is int64_t but without casting can cause problems
                printf("\t Frame PTS %lld (Delayed): Kodierungsdelay = %.2f ms\n", (long long)pic_out.i_pts, delay_ms);
                free(orig_start);
            }
        }
    }

    x264_encoder_close(encoder);//Auch Ausgabe der Kodierungsstatistik
    x264_picture_clean(&pic_in);
    free(buffer);
    fclose(file_in);
    fclose(file_out);
    
    return EXIT_SUCCESS;
}
