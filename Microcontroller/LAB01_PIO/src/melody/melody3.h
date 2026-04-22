#ifndef MELODY__H__
#define MELODY__H__

#include "note.h"

/* Score source: https://github.com/robsoncouto/arduino-songs and https://github.com/hibit-dev/buzzer */

/*! Melody struct, storing pointers to an array of notes and durations */
typedef struct {
    /*! Pointer to the notes array. */
    const Note* notes;
    /*! Pointer to the durations array.  */
    const uint8_t* durations;
    /*! The length of the \var notes and \var durations array (both are of same length). */
    uint16_t length;
    /*! The name of the melody. */
    const char* name;
    /*! The duration of a whole note (duration == 1) in milliseconds. */
    uint16_t wholeNoteDurationMs;
    /*! Additional pause after each note in percent of the note duration. */
    uint8_t perNotePausePercent;
} Melody;


static Melody melodyNew()
{
    static const char* name = "Harry Potter";

    static Note notes[] = {
        NOTE_NONE, NOTE_D4,
        NOTE_G4, NOTE_AS4, NOTE_A4,
        NOTE_G4, NOTE_D5,
        NOTE_C5,
        NOTE_A4,
        NOTE_G4, NOTE_AS4, NOTE_A4,
        NOTE_F4, NOTE_GS4,
        NOTE_D4,
        NOTE_D4,

        NOTE_G4, NOTE_AS4, NOTE_A4,
        NOTE_G4, NOTE_D5,
        NOTE_F5, NOTE_E5,
        NOTE_DS5, NOTE_B4,
        NOTE_DS5, NOTE_D5, NOTE_CS5,
        NOTE_CS4, NOTE_B4,
        NOTE_G4,
        NOTE_AS4,

        NOTE_D5, NOTE_AS4,
        NOTE_D5, NOTE_AS4,
        NOTE_DS5, NOTE_D5,
        NOTE_CS5, NOTE_A4,
        NOTE_AS4, NOTE_D5, NOTE_CS5,
        NOTE_CS4, NOTE_D4,
        NOTE_D5,
        NOTE_NONE, NOTE_AS4,

        NOTE_D5, NOTE_AS4,
        NOTE_D5, NOTE_AS4,
        NOTE_F5, NOTE_E5,
        NOTE_DS5, NOTE_B4,
        NOTE_DS5, NOTE_D5, NOTE_CS5,
        NOTE_CS4, NOTE_AS4,
        NOTE_G4
    };

    static uint8_t durations[] = {
        2, 4,
        4, 8, 4,
        2, 4,
        2,
        2,
        4, 8, 4,
        2, 4,
        1,
        4,

        4, 8, 4,
        2, 4,
        2, 4,
        2, 4,
        4, 8, 4,
        2, 4,
        1,
        4,

        2, 4,
        2, 4,
        2, 4,
        2, 4,
        4, 8, 4,
        2, 4,
        1,
        4, 4,

        2, 4,
        2, 4,
        2, 4,
        2, 4,
        4, 8, 4,
        2, 4,
        1
    };
    return (Melody){
        .notes = notes, .name = name, .durations = durations, .length = sizeof(notes) / sizeof(Note), .wholeNoteDurationMs = 3000, .perNotePausePercent = 10
    };
}

#endif
