#ifndef ring_buffer_h
#define ring_buffer_h

#include <stdbool.h>
#include <stdint.h>

#ifndef RING_BUFFER_CAPACITY
#define RING_BUFFER_CAPACITY 128
#endif

typedef struct {
    uint8_t buffer[RING_BUFFER_CAPACITY];
    volatile uint8_t head; // Schreib-Index
    volatile uint8_t tail; // Lese-Index
    volatile uint8_t count; // Aktuelle Anzahl der Elemente
} RingBuffer;

static inline void ringBufferInit(RingBuffer* ringBuffer)
{
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
    ringBuffer->count = 0;
}

static inline uint8_t ringBufferSize(const RingBuffer* ringBuffer)
{
    return ringBuffer->count;
}

static inline uint8_t ringBufferCapacity()
{
    return RING_BUFFER_CAPACITY;
}

static inline bool ringBufferEmpty(const RingBuffer* ringBuffer)
{
    return (ringBuffer->count == 0);
}

static inline bool ringBufferFull(const RingBuffer* ringBuffer)
{
    return (ringBuffer->count >= RING_BUFFER_CAPACITY);
}

static inline bool ringBufferPush(RingBuffer* ringBuffer, uint8_t value)
{
    if (ringBufferFull(ringBuffer)) {
        return false;
    }

    ringBuffer->buffer[ringBuffer->head] = value;
    ringBuffer->head = (ringBuffer->head + 1) % RING_BUFFER_CAPACITY;
    ringBuffer->count++;
    return true;
}

static inline bool ringBufferPeek(RingBuffer* ringBuffer, uint8_t* value)
{
    if (ringBufferEmpty(ringBuffer)) {
        return false;
    }

    *value = ringBuffer->buffer[ringBuffer->tail];
    return true;
}

static inline bool ringBufferPop(RingBuffer* ringBuffer, uint8_t* value)
{
    if (ringBufferEmpty(ringBuffer)) {
        return false;
    }

    *value = ringBuffer->buffer[ringBuffer->tail];
    ringBuffer->tail = (ringBuffer->tail + 1) % RING_BUFFER_CAPACITY;
    ringBuffer->count--;
    return true;
}

#endif