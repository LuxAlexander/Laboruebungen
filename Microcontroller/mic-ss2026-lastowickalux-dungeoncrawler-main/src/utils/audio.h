#ifndef AUDIO_H_
#define AUDIO_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "note.h"

// ATmega32: OC1A is on PD5
#define AUDIO_DDR  DDRD
#define AUDIO_PIN  PD5

// How many ms each "tick" advances (must match how often audio_tick() is called)
#define AUDIO_TICK_MS 50

// SFX note tables
// NOTE: frequencies are in plain Hz (not tenths-of-Hz)

static const uint16_t sfx_cat_pet_notes[]   = {880, 1320};
static const uint8_t  sfx_cat_pet_dur[]     = {8, 6};

static const uint16_t sfx_enemy_die_notes[] = {523, 659, 784, 1046};
static const uint8_t  sfx_enemy_die_dur[]   = {12, 12, 12, 4};

static const uint16_t sfx_gameover_notes[]  = {392, 349, 311, 246};
static const uint8_t  sfx_gameover_dur[]    = {4, 4, 4, 2};

typedef struct {
    const uint16_t* notes;
    const uint8_t*  durations;
    uint8_t         length;
    uint16_t        wholeNoteDurationMs;
} SoundEffect;


static SoundEffect currentSFX;
static int16_t     sfxNoteIndex        = -1; // -1 = no SFX running
static uint32_t    sfxNoteRemainingMs  = 0;

// Internal: write a frequency to Timer1 CTC, or silence the output

static void audio_play_note(uint16_t freq_hz)
{
    if (freq_hz == 0) {
        // Disconnect OC1A so the pin goes silent
        TCCR1A &= ~(1 << COM1A0);
        return;
    }
    // OCR1A = (F_CPU / (2 * prescaler * freq)) - 1   [prescaler = 8]
    OCR1A = (uint16_t)((F_CPU / (2UL * 8UL * freq_hz)) - 1);
    // Re-connect OC1A in case it was silenced before
    TCCR1A |= (1 << COM1A0);
}


// Call once at startup (before sei())

static void audio_init(void)
{
    // PD5 as output for OC1A, because that's where our speaker is connected
    AUDIO_DDR |= (1 << AUDIO_PIN);  // PD5 as output

    // CTC mode, prescaler 8
    // Toggle OC1A on compare match, so the output frequency is F_CPU/(2*prescaler*(1+OCR1A))
    TCCR1A = (1 << COM1A0);         // Toggle OC1A on compare match
    TCCR1B = (1 << WGM12) | (1 << CS11);

    // Start silent
    OCR1A = 0xFFFF;
    TCCR1A &= ~(1 << COM1A0);

    sfxNoteIndex       = -1;
    sfxNoteRemainingMs = 0;
}

// Trigger a sound effect - call this on game events, NOT every loop tick

void audio_trigger_sfx(uint8_t sfx_type)
{
    if (sfx_type == 1) {
        currentSFX.notes             = sfx_cat_pet_notes;
        currentSFX.durations         = sfx_cat_pet_dur;
        currentSFX.length            = 2;
        currentSFX.wholeNoteDurationMs = 800;
    } else if (sfx_type == 2) {
        currentSFX.notes             = sfx_enemy_die_notes;
        currentSFX.durations         = sfx_enemy_die_dur;
        currentSFX.length            = 4;
        currentSFX.wholeNoteDurationMs = 1000;
    } else if (sfx_type == 3) {
        currentSFX.notes             = sfx_gameover_notes;
        currentSFX.durations         = sfx_gameover_dur;
        currentSFX.length            = 4;
        currentSFX.wholeNoteDurationMs = 1600;
    } else {
        return; // unknown type — ignore
    }

    sfxNoteIndex       = 0;
    sfxNoteRemainingMs = 0; // force immediate note load on next tick
}

static void audio_tick(void)
{
    // Nothing queued
    if (sfxNoteIndex < 0) {
        return;
    }

    // Current note still playing
    if (sfxNoteRemainingMs > AUDIO_TICK_MS) {
        sfxNoteRemainingMs -= AUDIO_TICK_MS;
        return;
    }

    // Advance to next note
    sfxNoteIndex++;

    if (sfxNoteIndex >= currentSFX.length) {
        // SFX finished — silence output
        sfxNoteIndex = -1;
        audio_play_note(0);
        return;
    }

    // Load new note
    uint16_t freq = currentSFX.notes[sfxNoteIndex];
    uint8_t  dur  = currentSFX.durations[sfxNoteIndex];

    // duration value is a divisor of wholeNoteDurationMs
    // e.g. dur=4 means quarter-note = wholeNote / 4
    sfxNoteRemainingMs = currentSFX.wholeNoteDurationMs / dur;

    audio_play_note(freq);
}

#endif /* AUDIO_H_ */