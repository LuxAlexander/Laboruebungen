#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define LED_ON_TIME_MS 200
#define LED_OFF_TIME_MS LED_ON_TIME_MS

/* * Returns a word with only bit - th bit set . Mind the p a r e n t h e s e s ! */
# define BIT ( bit ) (1 ull << ( bit ))
/* * Raise bit - th bit in word . */
# define BIT_SET ( word , bit ) (( word ) |= BIT ( bit ))
/* * Clear bit - th bit in word . */
# define BIT_CLR ( word , bit ) (( word ) &= ˜ BIT ( bit ))
/* * Returns BIT ( bit ) if bit - th bit of word is set and 0 otherwise . */
# define MASK_BIT ( word , bit ) (( word ) & BIT ( bit ))
/* * Returns 1 if bit - th bit of word is set and 0 otherwise . */
# define BIT_IS_SET ( word , bit ) (!! MASK_BIT ( word , bit ))

int led(void)
{
    /* Replace with your application code */
	DDRA=0xFF;
	DDRA=DDRA|(1<<DDA6);
	//#warning this is warning
    while (1) 
    {
		PORTA= 0xFF;
		_delay_ms(LED_ON_TIME_MS);
		PORTA=0x00;
		_delay_ms(LED_OFF_TIME_MS);
		printf("Hello World");
		//Toggle
		PORTA ^=0xFF;
    }
}

