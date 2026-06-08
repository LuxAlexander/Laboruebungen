#ifndef ring_buffer_h
#define ring_buffer_h

#include <stdbool.h>
#include <stdint.h>

#ifndef RING_BUFFER_CAPACITY
/* Define size of ring buffer with a default value. */
#define RING_BUFFER_CAPACITY 128
#endif

typedef struct {
    /* Define an uint8_t array and additional variables for storing the ring buffer state. */
} RingBuffer;

/** Initialize the given RingBuffer struct.
 * @param ringBuffer The RingBuffer instance
 * @note Must be called before the ring buffer is used.
*/
static inline void ringBufferInit(RingBuffer* ringBuffer)
{
}

/** Returns the number of elements in the ring buffer.
 * @param ringBuffer The RingBuffer instance
 * @return Number of elements
*/
static inline uint8_t ringBufferSize(const RingBuffer* ringBuffer)
{
    return 0;
}

/** Returns the number of elements which can be stored at maximum.
 * @return Number of elements
*/
static inline uint8_t ringBufferCapacity()
{
    return 0;
}

/** Checks if the ring buffer has no elements
 * @param ringBuffer The RingBuffer instance
 * @return  True if empty, false otherwise
*/
static inline bool ringBufferEmpty(const RingBuffer* ringBuffer)
{
    return true;
}

/** Checks if the ring buffer is full.
 * @param ringBuffer The RingBuffer instance
 * @return  True if full, false otherwise
*/
static inline bool ringBufferFull(const RingBuffer* ringBuffer)
{
    return true;
}

/** Pushes the given value to the ring buffer.
 * @param ringBuffer The RingBuffer instance
 * @param[in] value The element to be added
 * @return  True if successful, false otherwise
*/
static inline bool ringBufferPush(RingBuffer* ringBuffer, uint8_t value)
{
    return false;
}

/** Returns the oldest value from the ring buffer, without removing it.
 * @param ringBuffer The RingBuffer instance
 * @param[out] value The oldest value
 * @return  True if successful, false otherwise
*/
static inline bool ringBufferPeek(RingBuffer* ringBuffer, uint8_t* value)
{
    return false;
}

/** Removes the oldest value from the ring buffer.
 * @param ringBuffer The RingBuffer instance
 * @param[out] value The removed value
 * @return  True if successful, false otherwise
*/
static inline bool ringBufferPop(RingBuffer* ringBuffer, uint8_t* value)
{
    return false;
}

#endif
