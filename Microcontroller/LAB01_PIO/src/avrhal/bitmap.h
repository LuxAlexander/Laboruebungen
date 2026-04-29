/**
 * @author MIC Lab Team
 * @brief Bitmap data struct
 *
 */

#ifndef BITMAP_H_
#define BITMAP_H_

#include <stdint.h>

typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
    uint8_t dataSize;
} Bitmap;

#endif /* BITMAP_H_ */