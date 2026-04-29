/**
 * @author MIC Lab Team
 * @brief System time
 *
 */

#ifndef _AVRHAL_TIME__H__
#define _AVRHAL_TIME__H__

#include <stdint.h>

/** Setup hardware timer 0 as system time
 *
*/
void timeSetup();

/** Elapsed time in microseconds since setup.
*
* Time granularity: 8 microseconds
* @note Elapsed time since invocation of timeSetup() and sei()
*
* @return elapsed time in microseconds
*/
uint32_t timeUSec();

#endif
