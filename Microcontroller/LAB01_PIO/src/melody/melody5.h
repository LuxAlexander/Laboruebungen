#ifndef MELODY__H__
#define MELODY__H__

#include "note.h"

/* Score source: https://github.com/CytronTechnologies/MakerUno_Examples/blob/master/MakerUno_Melody/ */

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
    static const char* name = "???";
    static Note notes[] = {
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,  NOTE_NONE,
  NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,  NOTE_NONE,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_G5, NOTE_F5,
  NOTE_F4, NOTE_F4, NOTE_F5, NOTE_F5, NOTE_G5, NOTE_F5, NOTE_DS5, NOTE_D5,  NOTE_NONE,

  NOTE_D5, NOTE_D5, NOTE_D5, NOTE_D5, NOTE_D5,  NOTE_NONE,
  NOTE_D5, NOTE_D5, NOTE_DS5, NOTE_F5, NOTE_AS4, NOTE_G5,  NOTE_NONE,  NOTE_NONE,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,  NOTE_NONE,
  NOTE_F5, NOTE_F5, NOTE_G5, NOTE_F5, NOTE_DS5, NOTE_C5,
   NOTE_NONE, NOTE_D5, NOTE_DS5,
  
  NOTE_B5, NOTE_B5, NOTE_AS5,  NOTE_NONE,
  NOTE_A5, NOTE_A5, NOTE_AS5,  NOTE_NONE,
  NOTE_B5, NOTE_B5, NOTE_AS5,  NOTE_NONE,
  NOTE_A5, NOTE_A5, NOTE_AS5,  NOTE_NONE,

  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_F5, NOTE_AS5,
  NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5,
  NOTE_C5, NOTE_A4, NOTE_F5,  NOTE_NONE,

  NOTE_A4, NOTE_AS4, NOTE_AS4,

  NOTE_G6, NOTE_F6,  NOTE_NONE, NOTE_G6, NOTE_F6,
    };

    static uint8_t durations[] = {
   4, 4, 4, 2, 2, 2,
  4, 4, 4, 4, 8, 2, 2,
  4, 4, 4, 2, 2,
  4, 4, 4, 4, 4, 4, 8, 2, 2, // 27

  4, 4, 4, 2, 2, 4,
  4, 4, 4, 2, 2, 1, 1, 4,
  4, 4, 4, 2, 2, 4,
  4, 4, 4, 4, 8, 4,
  8, 4, 4, // 29

  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 4, 4, // 16

  4, 4, 4, 4, 2, 2,
  4, 4, 4, 4, 2,
  4, 4, 1, 2, // 15

  4, 4, 1,
  2, 2, 1, 2, 2
    };

    return (Melody){
        .notes = notes, .name = name, .durations = durations, .length = sizeof(notes) / sizeof(Note), .wholeNoteDurationMs = 2000, .perNotePausePercent = 20
    };
}

#endif
