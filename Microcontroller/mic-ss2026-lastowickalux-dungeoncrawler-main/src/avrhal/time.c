#include "avrhal/time.h"
#include "utils/bit.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

/** Enables timer 0 compare match interrupt. */
static inline void enableTimerInterrupt()
{
    BIT_SET(TIMSK, OCIE0);
}

/** Disables timer 0 compare match interrupt. */
static inline void disableTimerInterrupt()
{
    BIT_CLR(TIMSK, OCIE0);
}

static volatile uint32_t counter;

void timeSetup()
{
}

uint32_t timeUSec()
{
    return 0;
}
