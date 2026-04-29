#include "avrhal/adc.h"
#include "avrhal/display.h"
#include "avrhal/time.h"
#include "avrhal/usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include <sprite.h>

#define buffer 255

int main(void)
{
    adcSetup();
    sei();
    adcSetupFreeRunning();
    displaySetup();

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

        // 2. Scale coordinates (ADC 0-1023 -> Display 128x64)
        // Puppy is 32x32, so we limit the range so it stays on screen
        uint8_t posX = adc_x / 10; // Result ~0 to 102
        uint8_t posY = adc_y / 20; // Result ~0 to 51

        // 3. Draw the Cross (using scaled coordinates)
        displayDrawHorizontalLine(posX, posY, 5);
        displayDrawVerticalLine(posX + 2, posY - 2, 5);

        // 4. DRAW BITMAP CORRECTLY
        // Get the struct from your header and pass it by pointer
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
