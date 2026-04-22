/*#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/atomic.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#define TX_BUFFER_SIZE 32

// ==============================
// Ring Buffer
// ==============================
static volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;

// ==============================
// Interrupt Control Helpers
// ==============================
static inline void enableUDREInterrupt(void)
{
    UCSRB |= (1 << UDRIE);
}

static inline void disableUDREInterrupt(void)
{
    UCSRB &= ~(1 << UDRIE);
}

// ==============================
// USART Data Register Empty ISR
// ==============================
ISR(USART_UDRE_vect)
{
    if (head == tail) {
        // Buffer empty → stop interrupt
        disableUDREInterrupt();
        return;
    }

    // Send next byte
    UDR = tx_buffer[tail];
    tail = (tail + 1) % TX_BUFFER_SIZE;
}

// ==============================
// USART Setup (ATmega32)
// ==============================
void usartSetup(uint32_t baud)
{
    uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;

    UBRRH = (uint8_t)(ubrr >> 8);
    UBRRL = (uint8_t)ubrr;

    // 8N1 frame format
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);

    // Enable transmitter only
    UCSRB = (1 << TXEN);

    // Set TX pin (PD1) as output
    DDRD |= (1 << PD1);

    sei(); // Enable global interrupts
}

// ==============================
// Non-blocking single byte write
// ==============================
bool usartWrite(uint8_t data)
{
    uint8_t next_head = (head + 1) % TX_BUFFER_SIZE;

    // Buffer full → fail immediately (non-blocking)
    if (next_head == tail) {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        tx_buffer[head] = data;
        head = next_head;
    }

    enableUDREInterrupt();
    return true;
}

// ==============================
// Non-blocking string write
// ==============================
bool usartWriteString(const char *str)
{
    for (uint16_t i = 0; str[i] != '\0'; i++) {
        if (!usartWrite((uint8_t)str[i])) {
            return false; // buffer full → caller must retry
        }
    }
    return true;
}

// ==============================
// Optional: Check if busy
// ==============================
bool usartIsBusy(void)
{
    return (head != tail);
}*/
