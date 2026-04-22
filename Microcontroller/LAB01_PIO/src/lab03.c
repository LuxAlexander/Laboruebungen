#include "avrhal/adc.h"
#include "avrhal/usart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <util/atomic.h>

#include <stdio.h>

#define buffer 255


int main(void)
{
    usartSetup(USART_B9600, USART_CONFIG_8N1);
    adcSetup();
    sei();

    char message[buffer];


    snprintf(message, sizeof(message), "Hello to Lab 3 :-) \n\r");

    usartWriteString(message);
    adcSetupFreeRunning();
    
    while (1) {
            
        // Jetzt erst die Werte verarbeiten/senden
        uint16_t adc_x, adc_y;

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_x = adcLastRead(0);
            adc_y = adcLastRead(1);
        }

        usartPrint("ADC x:%d y:%d\r\n", adc_x, adc_y);
        
        _delay_ms(100);
    }

    return 0;
}
