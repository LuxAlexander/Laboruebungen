#include "avrhal/usart.h"
#include "utils/bit.h"
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
    if (txLength != 0) {
        return 0; /* busy */
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
