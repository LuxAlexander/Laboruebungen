#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include "util/atomic.h"
#include "avrhal/adc.h"
#include <avr/io.h>



static volatile uint16_t adc_result_ch_0 = 0;
static volatile uint16_t adc_result_ch_1 = 0;


ISR(ADC_vect)
{
    uint8_t current_channel = ADMUX & 0x0F;

    uint16_t value = ADC;

    if (current_channel == 0)
    {
        adc_result_ch_0 = value;

        ADMUX = (ADMUX & 0xF0) | 1;
    }
    else
    {
        adc_result_ch_1 = value;
        ADMUX = (ADMUX & 0xF0) | 0;
    }
}

void adcSetup()
{
    // einschalten, Referenzspannung = AVCC, benötigten Prescaler (atm32 8MHz)
    // Taktfrequenz zwischen 50kHz und 200kHz
    // PA0
    // ADCSRA: ADEN=1 enable, ADPS2=1, ADPS1=1, ADPS0=1 -> Prescaler 128
    // Prescaler 64: APS2=1, ADPS1=1, ADPS0=0
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    // Other Options: REFS1=0, REFS0=1 -> AVCC with external capacitor at AREF pin
    // REFS1=0, REFS0=0 -> AREF, Internal Vref turned off
    // REFS1=1, REFS0=0 -> Internal 2.56V Voltage Reference with external capacitor at AREF pin
    ADMUX |= (1 << REFS0); // Referenzspannung = AVCC
    // PA0 als Eingang, ADC0 als Kanal
    // ADMUX: MUX3=0, MUX2=0, MUX1=0, MUX0=0 -> ADC0 (PA0)
}

uint16_t adcRead()
{
    // Start single conversion on channel

    // ADSC = 1 -> Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion to complete (ADSC becomes '0' again)
    while (ADCSRA & (1 << ADSC))
    {
        // busy wait
    }

    /*Read ADC value (ADCL must be read first)
    uint16_t adc_value = ADCL;
    adc_value |= (ADCH << 8);*/

    return ADCW; // ADCW is a combined register for ADCL and ADCH
}

void adcSetChannel(uint8_t channel)
{
    // Set the ADC channel (MUX bits in ADMUX register)
    // Clear the MUX bits (MUX3:0) and set the desired channel
    if (channel < 8)
    {
        ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    }
}

void adcSetupFreeRunning()
{
    // Free running mode: ADSC=1, ADATE=1, ADTS2=0, ADTS1=0, ADTS0=0
    // ADEN=1 to enable ADC, ADATE=1 to enable auto-triggering ADSC=1 to start conversion
    // ADTS2=0, ADTS1=0, ADTS0=0 for free running mode
    ADCSRA |= (1 << ADEN) | (1 << ADATE);
    SFIOR &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0)); // Free running mode

    // ADC Interrupt Enable
    ADCSRA |= (1 << ADIE);

    // start conversion
    ADCSRA |= (1 << ADSC);
    
}

int16_t adcLastRead(uint8_t channel)
{
    if (channel == 0)
    {
        return adc_result_ch_0;
    }
    else if (channel == 1)
    {
        return adc_result_ch_1;
    }
    else
    {
        return -1; // Invalid channel
    }
}
