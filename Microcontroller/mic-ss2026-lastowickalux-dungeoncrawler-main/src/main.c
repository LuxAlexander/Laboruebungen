#include "avrhal/adc.h"
#include "avrhal/display.h"
#include "avrhal/time.h"
#include "avrhal/usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "utils/sprite.h"
#include "utils/dungeon.h"
#include "utils/audio.h"

#define GAME_TICK_MS 30

int main(void)
{
    uint16_t loop_counter = 0;

    usartSetup(USART_B9600, USART_CONFIG_8N1);
    adcSetup();
    audio_init();

    mapInitRandomRooms(42);

    sei();
    adcSetupFreeRunning();
    displaySetup();

    mapDrawOverview();

    // Trigger the startup jingle once
    audio_trigger_sfx(1);

    uint16_t adc_x = 0;
    uint16_t adc_y = 0;
    uint8_t  index = 0;

    while (1) {
        displayClearBuffer();

        // Read ADC safely
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_x = adcLastRead(0);
            adc_y = adcLastRead(1);
        }

        // Advance the audio state machine every loop iteration
        // audio_trigger_sfx() is called on game EVENTS, not here
        audio_tick();
        loop_counter++;

        mapHandleInput(adc_x, adc_y, loop_counter);
        mapUpdateDisplay();

        _delay_ms(50); // debounce / CPU relief

        // Scale ADC (0–1023) to display coordinates
        uint8_t posX = adc_x / 10; // ~0–102
        uint8_t posY = adc_y / 20; // ~0–51

        Bitmap player = Player(index);
        displayDrawBitmap(posX, posY, &player);

        displayUpdate();

        index++;
        if (index >= 4) index = 0;

        _delay_ms(GAME_TICK_MS);
    }
    return 0;
}