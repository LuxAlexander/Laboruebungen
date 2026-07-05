#include <stdio.h>
#include <stdlib.h>

/*
	Values have been checked with online hexeditor https://hexed.it/ and with YUView
	AI was used to explain rawdata file composition (Y-values from to,U,V) and to get idea of motion estimation (steps)

*/

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
    if (!buffer) {
        fprintf(stderr, "Speicherfehler.\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    int frame_count = 0;
    // Liest Frames nacheinander ein, solange ganze frames gelesen
    while (fread(buffer, 1, frame_size, file) == frame_size) {
        printf("Frame %d eingelesen:\n\n", frame_count);

		//change y,x to look at different pixel, Beispiel anhand 2,2
        int y = 2;
        int x = 2;
        //check Y=(Frame(0->end)⋅FrameSize(152 064))+(y⋅W+x)
        //Second Frame z.B 1*152 064 + 2*352 + 2=152 064+704+2=152 770
        //online hexeditor https://hexed.it/ go to 152 770=>0xCD=205
        unsigned char y_val = buffer[y * width + x];

        // Zugriff auf U/V (entspricht dem Block bei row/2, col/2)
        unsigned char u_val = buffer[y_size + (y / 2) * (width / 2) + (x / 2)];
        //[2,2] in Frame 2 check OFFSET(F*Framesize)+Y_max+y*W/4+x/2=253 617=0x8A=>138
        unsigned char v_val = buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)];
		// check V = OFFSETframe+Y_max+Umax(W*H/4)+(y*W)/4+x/2=278961=0x66=>102
        printf("Pixel an (x=%d, y=%d): Y=%d, U=%d, V=%d\n", x, y, y_val, u_val, v_val);

		//YUView kann man sich diese Spalten,Zeilen anzeigen und sieht, dass Y und V perfekt passen und U um 1 abweicht(moegliche rundung)
		if(frame_count<10){
		printf("Y-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y * width + x], (size_t)(frame_count * frame_size) + (y * width + x));

		printf("U-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + (y / 2 * width / 2 + x / 2));

		printf("V-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + uv_size + (y / 2 * width / 2 + x / 2));
		}
        frame_count++;
	}

    free(buffer);
    fclose(file);
    return EXIT_SUCCESS;
}
