#include "usart.h"
#include "../utils/bit.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


#define TX_BUF_SIZE 255

static uint8_t txBuffer[TX_BUF_SIZE] = { 0 };
static volatile uint8_t txLength = 0;
static volatile uint8_t txPos = 0;

static inline void enableTransmitBufferEmptyInterrupt() /* set UDRIE 1= enable data Register empty interrupt */
{
    BIT_SET(UCSRB, UDRIE);
}

static inline void disableTransmitBufferEmptyInterrupt() /* set UDRIE 0= disable data Register empty interrupt */
{
    BIT_CLR(UCSRB, UDRIE);
}

uint8_t usartWriteString_P(const char* str_pgm)
{
    // 1. Wait until any ongoing interrupt-driven RAM transmission finishes
    while (txLength != 0) {
        // Wait loop
    }

    uint8_t count = 0;
    char c;

    // 2. Read characters directly out of Flash memory until the null terminator
    while ((c = pgm_read_byte(str_pgm + count)) != '\0') {
        
        // 3. Wait until the hardware Data Register is completely empty (UDRE bit is set)
        while (!(UCSRA & BIT(UDRE))) {
            // Wait for hardware register to clear
        }
        
        // 4. Shove the character straight into the hardware transmission register
        UDR = c;
        count++;
    }

    return count;
}

void usartResetTransmission()
{
    disableTransmitBufferEmptyInterrupt();
    txLength = 0;
    txPos = 0;
}

ISR(USART_UDRE_vect)
{
    if (txPos < txLength) {
        UDR = txBuffer[txPos];
        txPos++;

    } else {
        usartResetTransmission();
    }
}

void usartSetup(UsartBaudrate baud, UsartConfig config)
{
    if (config != USART_CONFIG_8N1) {
        return; /* Unsupported */
    }
    UCSRC = BIT(URSEL) | BIT(UCSZ0) | BIT(UCSZ1);

    uint16_t ubrrValue = (F_CPU / (16UL * baud)) - 1;
    UBRRL = ubrrValue & 0xFF;
    //Changed from << to >>
    UBRRH = (ubrrValue >> 8) & 0xFF;

    BIT_SET(UCSRB, TXEN);
}

uint8_t usartWriteString(const char* str)
{

    // Warten, bis die vorherige Übertragung vollständig abgeschlossen ist
    while (txLength != 0) {
        // Warteschleife (Interrupts müssen aktiv sein, damit txLength sinkt)
    }
    
     ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
     {
        uint16_t i = 0;
        for (i = 0; i < TX_BUF_SIZE && str[i] != '\0'; i++) {

            txBuffer[i] = str[i];
        }
        txPos = 0;
        txLength = i;
        enableTransmitBufferEmptyInterrupt();
    } 
    return txLength;
}

uint8_t usartPrint(const char* format, ...)
{
    const uint8_t bufferSize = 255;
    char buffer[bufferSize];

    /* Initialize buffer with zero-length string, in case vsnprintf() fails */
    buffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, bufferSize, format, args);
    va_end(args);

    return usartWriteString(buffer);
}
