#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
	Values have been checked with online hexeditor https://hexed.it/ and with YUView
	AI was used to explain rawdata file composition (Y-values from to,U,V) and to get idea of motion estimation (steps)

*/

typedef struct {
    int dx;
    int dy;
    double metric;
} MotionVector;

void perform_me(unsigned char *prev_y, unsigned char *curr_y, int width, int height, int alg) {
	//Default/0:SAD,1:MSE,2:PSNR
    int mb_size = 16;
    int search_range = 32;
    int p = search_range / 2; // Auslenkung +/- 16

    // Startpunkt Mitte mit 16x16
    int start_x = (width / 2) - (mb_size / 2);
    int start_y = (height / 2) - (mb_size / 2);

    // PSNR looks at Max
    double best_metric = (alg == 2) ? -1.0 : 1000000000.0;

    int best_dx = 0;
    int best_dy = 0;

    // Full Search im 32x32 Suchbereich
    for (int dy = -p; dy < p; dy++) {
        for (int dx = -p; dx < p; dx++) {

            double current_sum = 0;

            // SAD = Summe über alle Pixel (i,j) von |Pixel_aktuell(i,j) - Pixel_referenz(i,j)|
            // MSE = squared differences
            //PSNR=Verhältnis zwischen dem maximal möglichen Signalwert und MSE 
            for (int j = 0; j < mb_size; j++) {
                for (int i = 0; i < mb_size; i++) {
					// Index: (Y-Start + Zeilen-Offset j) * Bildbreite + (X-Start + Spalten-Offset i).
					int curr_p = curr_y[(start_y + j) * width + (start_x + i)];
					int prev_p = prev_y[(start_y + dy + j) * width + (start_x + dx + i)];
                    int diff = curr_p - prev_p;

                    switch (alg) {
                        case 1: // MSE
                        case 2: // PSNR basiert auf MSE
                            // Gewichtung der Unterschiede, groessere differenzen staerker bestraft
                            current_sum += (double)diff * diff;
                            break;
                        case 0:
                        default:
                            current_sum += abs(diff);
                            break;
                    }
                }
            }

            double final_metric = 0;
            if (alg == 1 || alg == 2) {
            	//Normierung
                double mse = current_sum / (mb_size * mb_size);
                if (alg == 2) {
                    // PSNR Berechnung: 10 * log10(MAX^2 / MSE)
                    if (mse > 0){
                    	final_metric = 10.0 * log10((255.0 * 255.0) / mse);
					} else final_metric = 99.0; // Case für MSE=0 (perfekter Match)
                } else final_metric = mse;
            } else final_metric = current_sum;


            // new best motion vector
            // Bei PSNR (alg 2) ist "besser" ein hoeherer Wert, sonst ein niedrigerer
            if ((alg == 2 && final_metric > best_metric) || (alg != 2 && final_metric < best_metric)) {
                best_metric = final_metric;
                best_dx = dx;
                best_dy = dy;
            }
        }
    }

    printf("\nMotion Estimation (Frame 9 -> 10)\n");
    printf("Block-Mitte Start: x=%d, y=%d\n", start_x, start_y);
    printf("Bewegungsvektor (MV): dx=%d, dy=%d\n", best_dx, best_dy);
    printf("Gewaehlte Metrik (%s)\n", (alg == 1 ? "MSE" : (alg == 2 ? "PSNR" : "SAD")));
    printf("MC-Ergebnis (Position im Ref-Frame 9): x=%d, y=%d\n", start_x + best_dx, start_y + best_dy);
}

int main(int argc, char *argv[]) {
     if (argc < 4) {
        fprintf(stderr, "Bitte gib: %s <Pfad> <Breite> <Hoehe> an\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    //konvertiert einen ASCII-String in eine Ganzzahl
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
        fprintf(stderr, "Fehler: Ungueltige Dimensionen (muessen gerade und positiv sein).\n");
        return EXIT_FAILURE;
    }
	//read binary
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Fehler beim Oeffnen der Datei");
        return EXIT_FAILURE;
    }

    // Speicherberechnung für ein Frame im 4:2:0 Format
    //Y full, U 1/4, V 1/4
    size_t y_size = (size_t)width * height;
    size_t uv_size = (size_t)(width / 2) * (height / 2);//Viertel
    size_t frame_size = y_size + 2 * uv_size;//z.B. 352*288*1,5

    unsigned char *buffer = (unsigned char *)malloc(frame_size);
    unsigned char *prev_frame_y = (unsigned char *)malloc(y_size); // Speicher für Frame 9 Y-Ebene
    unsigned char *curr_frame_y = (unsigned char *)malloc(y_size); // Speicher für Frame 10 Y-Ebene

    if (!buffer||!prev_frame_y||!curr_frame_y) {
        fprintf(stderr, "Speicherfehler.\n");
        fclose(file);
        return EXIT_FAILURE;
    }


    int frame_count = 0;
    while (fread(buffer, 1, frame_size, file) == frame_size) {

		printf("Frame %d eingelesen:\n", frame_count);

		//change y,x to look at different pixel, Beispiel anhand 2,2
        int y = 20;
        int x = 26;
        //check Y=(Frame(0->end)⋅FrameSize(152 064))+(y⋅W+x)
        //Second Frame z.B 1*152 064 + 2*352 + 2=152 064+704+2=152 770
        //online hexeditor https://hexed.it/ go to 152 770=>0xCD=205
        unsigned char y_val = buffer[y * width + x];

        // Zugriff auf U/V (entspricht dem Block bei row/2, col/2)
        unsigned char u_val = buffer[y_size + (y / 2) * (width / 2) + (x / 2)];
        //[2,2] in Frame 2 check OFFSET(F*Framesize)+Y_max+y*W/4+x/2=253 617=0x8A=>138
        unsigned char v_val = buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)];
		// check V = OFFSETframe+Y_max+Umax(W*H/4)+(y*W)/4+x/2=278961=0x66=>102
        printf("Pixel an (x=%d, y=%d): Y=%d, U=%d, V=%d\n\n", x, y, y_val, u_val, v_val);

        // ME Vorbereitung: Frame 9 sichern
        if (frame_count == 8) {
            for(size_t i=0; i<y_size; i++) prev_frame_y[i] = buffer[i];
        }

        // ME Ausführung: Frame 10
        if (frame_count == 9) {
            for(size_t i=0; i<y_size; i++) curr_frame_y[i] = buffer[i];
            perform_me(prev_frame_y, curr_frame_y, width, height,1);
            //SAD:MC-Ergebnis (Position im Ref-Frame 9): x=164, y=139
            //MSE:MC-Ergebnis (Position im Ref-Frame 9): x=163, y=139
            //PSNR:MC-Ergebnis (Position im Ref-Frame 9): x=163, y=139, von x:164->163
            break;
        }
        frame_count++;
    }

    free(buffer); free(prev_frame_y); free(curr_frame_y);
    fclose(file);
    return EXIT_SUCCESS;
}
