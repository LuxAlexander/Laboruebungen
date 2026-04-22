//#include <Arduino.h>
#include <avr/io.h>
#include "melody/note.h"
#include <stdio.h>
#include <util/delay.h>
#include "melody/melody2.h"
#include <string.h>

//#define F_CPU 8000000UL

void Timer1SetupCTC(){
	DDRD |=(1<<PD5);
	TCCR1A |=(1<<COM1A0);
	TCCR1B |=(1<<WGM12);

	TCCR1B |=(1<<CS11);

	//COMPARE WERT
	OCR1A=1135;

}
void TimerStop(){
	TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
}
void TimerPlayNote(uint16_t frequencyTenths){
    // Multiplying F_CPU by 10UL forces a 32-bit calculation
    // 16UL is 2 * 8 (your prescaler)
    OCR1A = ((F_CPU * 10UL) / (16UL * frequencyTenths)) - 1;
}

void variable_delay_ms(uint16_t delay_time) {
    for (uint16_t i = 0; i < delay_time; i++) {
        _delay_ms(1); // '1' is a constant, so the compiler is happy!
    }
}

int timer(void)
{
	Timer1SetupCTC();
	
	Melody melody1=melodyNew();
	while (1)
	{
		for(int i=0;i<melody1.length;i++){
			TimerPlayNote(melody1.notes[i]);
			variable_delay_ms(melody1.wholeNoteDurationMs/(melody1.durations[i]));
			/*uint16_t fulltime = melody1.wholeNoteDurationMs/(melody1.durations[i]);
			uint16_t notetime = fulltime*(100-melody1.perNotePausePercent)*0.01;
			variable_delay_ms(notetime);
			TimerPlayNote(NOTE_NONE);
			variable_delay_ms(fulltime-notetime);*/
		}
	}
	return 0;
}