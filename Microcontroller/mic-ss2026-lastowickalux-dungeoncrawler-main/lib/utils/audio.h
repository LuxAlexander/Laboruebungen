#ifndef AUDIO_H_
#define AUDIO_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "note.h"     // Enthält deine NOTE_Definitions
//#include "melody/melody2.h"   // Enthält die Melody-Struktur

// ATmega32 spezifisch: OC1A ist an Pin PD5 gebunden
#define AUDIO_DDR  DDRD
#define AUDIO_PIN  PD5

// Strukturen für die Soundeffekte (Frequenzen in Zehntel-Hz)
static const uint16_t sfx_cat_pet_notes[]   = { 8800, 13200 };
static const uint8_t  sfx_cat_pet_dur[]     = { 8,    6 };

static const uint16_t sfx_enemy_die_notes[] = { 5230, 6590, 7840, 10460 };
static const uint8_t  sfx_enemy_die_dur[]   = { 12,   12,   12,   4 };

static const uint16_t sfx_gameover_notes[]  = { 3920, 3490, 3110, 2460 };
static const uint8_t  sfx_gameover_dur[]    = { 4,    4,    4,    2 };

typedef struct {
    const uint16_t* notes;
    const uint8_t* durations;
    uint8_t length;
    uint16_t wholeNoteDurationMs;
} SoundEffect;

// Globale Musik-Statemachine Variablen
//static Melody gameMelody;
static int16_t currentNoteIndex = 0;
static uint32_t noteRemainingDurationMs = 0;
static uint8_t isMusicPlaying = 0;

static SoundEffect currentSFX;
static int16_t sfxNoteIndex = -1; // -1 bedeutet: Kein SFX läuft

// --- Hardware-Funktionen ---

static void audio_init(void) {
    AUDIO_DDR |= (1 << AUDIO_PIN);   // PD5 als Ausgang für den Buzzer
    TCCR1A |= (1 << COM1A0);         // Toggle OC1A bei Compare Match
    TCCR1B |= (1 << WGM12);          // CTC-Modus (Clear Timer on Compare)
    TCCR1B |= (1 << CS11);           // Prescaler 8 aktivieren
    
    // Hintergrundmusik laden und aktivieren
    //gameMelody = melodyNew();
    currentNoteIndex = 0;
    noteRemainingDurationMs = 0;
    isMusicPlaying = 1; 
}

/**
 * Startet einen Soundeffekt. Unterbricht temporär die Musik.
 */
static void audio_trigger_sfx(uint8_t sfx_type) {
    if (sfx_type == 1) { // Cat Pet
        currentSFX.notes = sfx_cat_pet_notes;
        currentSFX.durations = sfx_cat_pet_dur;
        currentSFX.length = 2;
        currentSFX.wholeNoteDurationMs = 800;
    } 
    else if (sfx_type == 2) { // Enemy Die
        currentSFX.notes = sfx_enemy_die_notes;
        currentSFX.durations = sfx_enemy_die_dur;
        currentSFX.length = 4;
        currentSFX.wholeNoteDurationMs = 1000;
    } 
    else if (sfx_type == 3) { // Game Over
        currentSFX.notes = sfx_gameover_notes;
        currentSFX.durations = sfx_gameover_dur;
        currentSFX.length = 4;
        currentSFX.wholeNoteDurationMs = 1600;
    }
    
    sfxNoteIndex = 0;            // SFX-Statemachine auf erste Note setzen
    noteRemainingDurationMs = 0; // Sofortigen Notenwechsel erzwingen
}

#endif /* AUDIO_H_ */