#include <stdio.h>
#include <string.h>
#include <ao/ao.h>
#include <math.h>
#include <stdlib.h>

 typedef enum {
    NOTE_NONE = 5000,
    NOTE_B0 = 309,
    NOTE_C1 = 327,
    NOTE_CS1 = 346,
    NOTE_D1 = 367,
    NOTE_DS1 = 389,
    NOTE_E1 = 412,
    NOTE_F1 = 437,
    NOTE_FS1 = 462,
    NOTE_G1 = 490,
    NOTE_GS1 = 519,
    NOTE_A1 = 550,
    NOTE_AS1 = 583,
    NOTE_B1 = 617,
    NOTE_C2 = 654,
    NOTE_CS2 = 693,
    NOTE_D2 = 734,
    NOTE_DS2 = 778,
    NOTE_E2 = 824,
    NOTE_F2 = 873,
    NOTE_FS2 = 925,
    NOTE_G2 = 980,
    NOTE_GS2 = 1038,
    NOTE_A2 = 1100,
    NOTE_AS2 = 1165,
    NOTE_B2 = 1235,
    NOTE_C3 = 1308,
    NOTE_CS3 = 1386,
    NOTE_D3 = 1468,
    NOTE_DS3 = 1556,
    NOTE_E3 = 1648,
    NOTE_F3 = 1746,
    NOTE_FS3 = 1850,
    NOTE_G3 = 1960,
    NOTE_GS3 = 2077,
    NOTE_A3 = 2200,
    NOTE_AS3 = 2331,
    NOTE_B3 = 2469,
    NOTE_C4 = 2616,
    NOTE_CS4 = 2772,
    NOTE_D4 = 2937,
    NOTE_DS4 = 3111,
    NOTE_E4 = 3296,
    NOTE_F4 = 3492,
    NOTE_FS4 = 3700,
    NOTE_G4 = 3920,
    NOTE_GS4 = 4153,
    NOTE_A4 = 4400,
    NOTE_AS4 = 4662,
    NOTE_B4 = 4939,
    NOTE_C5 = 5233,
    NOTE_CS5 = 5544,
    NOTE_D5 = 5873,
    NOTE_DS5 = 6223,
    NOTE_E5 = 6593,
    NOTE_F5 = 6985,
    NOTE_FS5 = 7400,
    NOTE_G5 = 7840,
    NOTE_GS5 = 8306,
    NOTE_A5 = 8800,
    NOTE_AS5 = 9323,
    NOTE_B5 = 9878,
    NOTE_C6 = 10465,
    NOTE_CS6 = 11087,
    NOTE_D6 = 11747,
    NOTE_DS6 = 12445,
    NOTE_E6 = 13185,
    NOTE_F6 = 13969,
    NOTE_FS6 = 14800,
    NOTE_G6 = 15680,
    NOTE_GS6 = 16612,
    NOTE_A6 = 17600,
    NOTE_AS6 = 18647,
    NOTE_B6 = 19755,
    NOTE_C7 = 20930,
    NOTE_CS7 = 22175,
    NOTE_D7 = 23493,
    NOTE_DS7 = 24890,
    NOTE_E7 = 26370,
    NOTE_F7 = 27938,
    NOTE_FS7 = 29600,
    NOTE_G7 = 31360,
    NOTE_GS7 = 33224,
    NOTE_A7 = 35200,
    NOTE_AS7 = 37293,
    NOTE_B7 = 39511,
    NOTE_C8 = 41860,
    NOTE_CS8 = 44349,
    NOTE_D8 = 46986,
    NOTE_DS8 = 49780,
} Note;

int main(int argc, char **argv)
{
	/*Notes:
	 * Noten->Frequenzen:
	 * https://tonstudio-wissen.de/wp-content/uploads/Frequenztabelle-Musik-T%C3%B6ne-1.pdf
	 * Life Is Strange:
	 * Song:
	 * https://www.youtube.com/watch?v=1KL5l2ocz7Q
	 * Noten:
	 * https://tabs.ultimate-guitar.com/tab/daughter/the-right-way-around-tabs-6015233
	 * 
	 * Star Wars:
	 * Notes:
	 * https://musescore.com/official_scores/scores/6950468
	 * Song:
	 * https://www.youtube.com/watch?v=_D0ZQPqeJkk
	 * Klavier Check:
	 * https://www.musicca.com/de/klavier
	 * 
	 * AI has been used to cross check the note sheets and translate them into notes and durations
	 * 
	 * */
    ao_device *device;
    ao_sample_format format;
    int default_driver;
    char *buffer;
    int buf_size;
    int sample;


    //LIFE IS STRANGE (Before the Storm)
    /*
    float notes[] = {
        NOTE_GS2, NOTE_GS2, NOTE_AS2, NOTE_AS2,                                     // Zeile 1
        NOTE_AS2, NOTE_F3, NOTE_F3, NOTE_F3, NOTE_F3, NOTE_CS3, NOTE_CS3, NOTE_CS3, NOTE_CS3, // Zeile 2
        NOTE_DS3, NOTE_F3, NOTE_GS2, NOTE_GS2, NOTE_AS2, NOTE_AS2,                  // Zeile 3
        NOTE_AS2, NOTE_F3, NOTE_F3, NOTE_F3, NOTE_F3, NOTE_CS3, NOTE_CS3, NOTE_CS3, NOTE_CS3, // Pose Slide
        NOTE_DS3, NOTE_F3, NOTE_GS2, NOTE_GS2, NOTE_AS2, NOTE_AS2
    };
    float durations[] = {
        0.4, 0.4, 0.4, 0.4,
        0.1, 0.3, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4,
        0.4, 0.4, 0.4, 0.4, 0.4, 0.4,
        0.1, 0.3, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4,
        0.4, 0.4, 0.4, 0.4, 0.4, 0.4
    };*/

    
    //STAR WARS
    float notes[] = {
        NOTE_G3, NOTE_D4, 
        NOTE_C4, NOTE_B3, NOTE_A3, NOTE_G4, NOTE_D4, 
        NOTE_C4, NOTE_B3, NOTE_A3, NOTE_G4, NOTE_D4, 
        NOTE_C4, NOTE_B3, NOTE_C4, NOTE_A3
    };
    float durations[] = {
        0.4, 0.4, 
        0.15, 0.15, 0.15, 0.5, 0.3,
        0.15, 0.15, 0.15, 0.5, 0.3,
        0.15, 0.15, 0.15, 0.6 
    };

    int num_notes = sizeof(notes) / sizeof(notes[0]);//len

    ao_initialize();
    default_driver = ao_default_driver_id();

    memset(&format, 0, sizeof(format));
    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;
    format.byte_format = AO_FMT_LITTLE;

    device = ao_open_live(default_driver, &format, NULL);
    if (device == NULL) {
        fprintf(stderr, "Error opening device.\n");
        return 1;
    }

    fprintf(stderr, "Playing Star Wars\n");
    //fprintf(stderr, "Playing Life Is Strange\n");


    for (int n = 0; n < num_notes; n++) {
		//Dauer*Sampling= Anzahl der noetigen Messpunkte
        int num_samples = (int)(format.rate * durations[n]);
        buf_size = (format.bits / 8) * format.channels * num_samples;
        buffer = calloc(buf_size, sizeof(char)); 
        
        //Tonhoehe der Sinuswelle, Noten sind in zehntel definiert
        float freq = (float)notes[n] / 10.0;

        for (int i = 0; i < num_samples; i++) {
			//Grundlautstärke
            float amplitude = 0.25;
            
            if (i > num_samples / 2) {
				//Ab der Hälfte anfangen, die Noten/Toene langsam ausklingen lassen
                int t = i - (num_samples / 2);//Zeit von 0 bis Ende
                //Exponentielle Abnahme, verhindert Knackgerausche am Ende
                amplitude *= exp(-(float)t / 1500.0);//Kleinerer Teiler bewirkt schneller gegen 0
            }
			
			//Tonerzeugung, sin erzeugt Reinton einer Periode, fuer die freq, i/sample=zeit, amplitude an den Datentyp (int) anpassen
            sample = (int)(amplitude * 32767.0 * sin(2 * M_PI * freq * ((float)i / format.rate)));

			//Auf Beiden Kanaelen, Ausgabe => Mono, Low and High Byte spalten, 
            buffer[4*i]   = buffer[4*i+2] = sample & 0xff;
            buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
        }
		//audiogeraet, sound to play, groesse, 
        ao_play(device, buffer, buf_size);
        free(buffer);
    }

    ao_close(device);
    ao_shutdown();

    return 0;
}
