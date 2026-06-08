/**
 * @author MIC Lab Team - Olaf Sassnick
 * @brief Font Specification
 *
 */

#ifndef _AVRHAL_FONTSPEC__H__
#define _AVRHAL_FONTSPEC__H__

#include <stdint.h>

typedef struct {
    const uint8_t* data;
    const uint8_t charSize;
    const uint8_t firstChar;
    const uint8_t lastChar;
} FontSpec;

#endif
