#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x264.h>

/*
	Neu: 5 Argumente, x264 Initialisierung und Data Mapping (memcpy, farbebenen plane[0]-[2]), Rest Daten Schreiben (Flush)
	Ausführung: make->z.B. ./MTM04a.exe foreman.yuv 352 288 output.h264
	Ausgabe: Pixel YUV Values +
	
	x264 [info]: frame I:2      Avg QP:24.82  size: 18437
	Anzahl der I frames 2 + groesse
	x264 [info]: frame P:298    Avg QP:26.16  size:  2437
	Anzahl Bewegungsgeschaetzter Bilder P-Frames + groesse
	
	x264 [info]: mb I  I16..4:  8.1%  0.0% 91.9%
	Makroblock Aufteilung, 16x16->4x4 wie viele in prozent so codiert wurden
	x264 [info]: mb P  I16..4:  0.3%  0.0%  1.5%  P16..4: 49.1% 21.8%  8.7%  0.0%  0.0%  skip:18.7%
	P-Frames als I-Frames Codiert, der Rest gleiches Prinzip wie oben, Bei 18.7% kein Unterschied zum Vorherigen Bild
	Intra-Vorhersagerichtungen, Farb- und Referenzstatistiken, finale Datenrate
	
	Ausgangsbasis File LB-VK 03.+ Example File from x264: https://code.videolan.org/videolan/x264/-/blob/master/example.c?ref_type=heads
	
	nice to know (Info from Stack Overflow):
	i_csp = X264_CSP_I420;
	i_plane = 3;
	plane[0] = pointer to Y-plane;
	i_stride[0] = stride in bytes for Y-plane;
	plane[1] = pointer to U-plane;
	i_stride[1] = stride in bytes for U-plane;
	plane[2] = pointer to V-plane;
	i_stride[2] = stride in bytes for V-plane;
		
	Used Gemini to go over the Parameters which I needed to fill with const and generated the syntax for x264_encoder_encode() 
	
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

        int y = 2;
        int x = 2;
        unsigned char y_val = buffer[y * width + x];
        unsigned char u_val = buffer[y_size + (y / 2) * (width / 2) + (x / 2)];
        unsigned char v_val = buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)];
        printf("Pixel an (x=%d, y=%d): Y=%d, U=%d, V=%d\n", x, y, y_val, u_val, v_val);

        if(frame_count<10){
            printf("Y-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y * width + x], (size_t)(frame_count * frame_size) + (y * width + x));
            printf("U-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + (y / 2 * width / 2 + x / 2));
            printf("V-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + uv_size + (y / 2 * width / 2 + x / 2));
        }

        // Frame an x264 übergeben und kodieren
        // Kopiert die Daten aus deinem 'buffer' in die x264-Struktur
        //copy a specific number of bytes from one memory location to another. DEST,Source
        memcpy(pic_in.img.plane[0], buffer, y_size);
        memcpy(pic_in.img.plane[1], buffer + y_size, uv_size);
        memcpy(pic_in.img.plane[2], buffer + y_size + uv_size, uv_size);
        
        pic_in.i_pts = frame_count;

        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
        if (frame_size_out > 0) {
            fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
        }
        
        frame_count++;
    }

    //Delayed Frames leeren (Flush
    while (x264_encoder_delayed_frames(encoder)) {
        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, NULL, &pic_out);
        if (frame_size_out > 0) {
            fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
        }
    }

    x264_encoder_close(encoder);
    x264_picture_clean(&pic_in);
    free(buffer);
    fclose(file_in);
    fclose(file_out);
    
    return EXIT_SUCCESS;
}
