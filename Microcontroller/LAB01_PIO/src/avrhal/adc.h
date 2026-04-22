/**
 * @author MIC Lab Team
 * @brief avrhal adc functions
 *
 */
#ifndef _AVRHAL_ADC__H__
#define _AVRHAL_ADC__H__

#include <stdint.h>

/** Setup the ADC.
 *  @note Must be invoked once before adcRead().
*/
void adcSetup();

/** Carry out a single AD Conversion.
 *
 * This call busy-waits until the conversion is completed.
 * 
 * @return The 10-bit conversion result
*/
uint16_t adcRead();


/** Set the channel for a single AD Conversion.
 *
 * @param[in] the AD channel
*/
void adcSetChannel(uint8_t channel);


/** Configures the AD Converter as free running.
 *
 * Configure the AD Converter to continuously sample and convert analog inputs (free-running mode).
 * The ISR alternately switches between channels 0 and 1, performing conversions on each channel. 
 * From the main execution path, the most recent conversion result  can be obtained via adcLastRead().
*/
void adcSetupFreeRunning();

/** Obtain last AD result
 *
 * When configured via analogReadFreeRunning() this
 * function may be used to obtain the last reading.
 *
 * The value "-1" is returned when no result is available
 * or the pin was not configured via analogReadFreeRunning().
 *
 * @param[in] pin avrhal pin number
 * @return last conversion result or -1
*/
int16_t adcLastRead(uint8_t channel);

#endif
