/**
 * @author MIC Lab Team - Olaf Sassnick
 * @brief Small math helper functions
 *
 */

#ifndef _UTILS_MATH__H__
#define _UTILS_MATH__H__

static inline uint8_t absInt8(int8_t val)
{
    if (val < 0) {
        return -val;
    }
    return val;
}

static inline int8_t signInt8(int8_t val)
{
    if (val < 0) {
        return -1;
    }
    return 1;
}

#endif
