#include "avrhal/adc.h"
#include "avrhal/display.h"
#include "avrhal/time.h"
#include "avrhal/usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include <sprite.h>
#include "utils/dungeon.h"

#define buffer 255
//Drei Individuelle Fragen zum Source Code
//Fragen wie, BIT Setzen, Interrupts, ADC, USART, SPI, E-Prom, etc.
//E-Prom, für externen Speicher
//alle unnötigen Files, usw. löschen
//Erste Abgebe:16.06
//Danach Zweite Abgebe über Git, zum Refactoring (Feedback einbringen)
int main(void)
{
    // Hardware initializations (usart, adc, timers...)
    uint16_t loop_counter = 0;

    usartSetup(USART_B9600, USART_CONFIG_8N1);
    adcSetup();
    
    // Generiere echten Zufallswert über den analogen Eingang für den Seed
    mapInitRandomRooms(42);


    sei();
    adcSetupFreeRunning();
    displaySetup();
    
    // Erste Karte zeichnen
    mapDrawOverview();

    uint16_t adc_x = 0;
    uint16_t adc_y = 0;
    uint8_t index = 0;


    while (1) {
        displayClearBuffer();

        // 1. Read ADC safely
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_x = adcLastRead(0);
            adc_y = adcLastRead(1);
        }

        
        loop_counter++; 

        mapHandleInput(adc_x, adc_y, loop_counter);
        mapUpdateDisplay();
        // Optional: Add a small delay to reduce CPU usage (and debounce joystick)
        _delay_ms(50);
        //usartPrint("ADC x:%d y:%d\r\n", adc_x, adc_y);

        // 2. Scale coordinates (ADC 0-1023 -> Display 128x64)
        // Puppy is 32x32, so we limit the range so it stays on screen
        uint8_t posX = adc_x / 10; // Result ~0 to 102
        uint8_t posY = adc_y / 20; // Result ~0 to 51

        // 3. Draw the Cross (using scaled coordinates)
        displayDrawHorizontalLine(posX, posY, 5);
        displayDrawVerticalLine(posX + 2, posY - 2, 5);

        // 4. DRAW BITMAP CORRECTLY
        // Get the struct from your header and pass it by pointer
        //no magic numbers
        Bitmap puppy = imagePuppy((index / 10) % 4); 
        displayDrawBitmap(posX, posY, &puppy);

        displayUpdate();

        // 5. Manage Animation Speed
        index++;
        if (index >= 40) index = 0;

        _delay_ms(30); // Add a small delay so the animation isn't too fast
    }
    return 0;       
}
