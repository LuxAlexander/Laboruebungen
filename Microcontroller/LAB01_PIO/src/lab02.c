/*#include "avrhal/ugly-usart.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define LOOP_DELAY_MS 100

int serialconnection(void)
{
    usartSetup(9600);
    uint8_t message[] = "Hello from ATmega32\n\r";
    sei();

    while (1) {
        _delay_ms(LOOP_DELAY_MS);
        usartWriteString(message);
    }
    cli();
    return 0;
}
*/