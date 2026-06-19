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
#define PLAYER_FRAMES 4
#define StartUpSFX 2
#define SEED 42

#define ADC_X 0
#define ADC_Y 1

#define X_Scale 8
#define Y_Scale 16
// The Y_Max value is used for inversion
#define Y_Max 1023

int main(void)
{
    uint16_t loop_counter = 0;

    //USART_B9600, USART_CONFIG_8N1 means 9600 baud, 8 data bits, no parity, 1 stop bit
    usartSetup(USART_B9600, USART_CONFIG_8N1);
    adcSetup();
    audio_init();

    mapInitRandomRooms(SEED);

    sei();
    adcSetupFreeRunning();
    displaySetup();

    mapDrawOverview();

    // Trigger the startup jingle once
    audio_trigger_sfx(StartUpSFX);

    uint16_t adc_x = 0;
    uint16_t adc_y = 0;
    uint8_t  index = 0;

    while (1) {
        displayClearBuffer();

        // Read ADC free-running values atomically to avoid race conditions
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_x = adcLastRead(ADC_X);
            adc_y = adcLastRead(ADC_Y);
        }

        // Advance the audio state machine every loop iteration
        // audio_trigger_sfx() is called on game EVENTS, not here
        audio_tick();
        loop_counter++;

        mapHandleInput(adc_x, adc_y, loop_counter);
        mapUpdateDisplay();

        _delay_ms(2*GAME_TICK_MS);//small delay to avoid too fast input handling

        // Scale ADC (0–1023) to display coordinates
        uint8_t posX = adc_x / X_Scale; // 1023/8
        uint8_t posY = (Y_Max/Y_Scale) - adc_y / Y_Scale; // Max Y-value minus Y_Pos to invert the Y-axis

        Bitmap player = Player(index);
        displayDrawBitmap(posX, posY, &player);

        displayUpdate();

        index++;
        if (index >= PLAYER_FRAMES) index = 0;

        _delay_ms(GAME_TICK_MS);
    }
    return 0;
}