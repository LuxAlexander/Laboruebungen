/**
 * @author MIC Lab Team - Olaf Sassnick
 * @brief OLED Display Driver with framebuffer
 *
 */

#ifndef _AVRHAL_DISPLAY__H__
#define _AVRHAL_DISPLAY__H__

#include "avrhal/bitmap.h"
#include <stdint.h>

#define DISPLAY_DEFAULT_CONTRAST 128
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_PAGES 8
#define DISPLAY_BITS_PER_PAGE_COLUMN 8

/**
 * @brief Perform a hardware reset of the display controller.
 */
void displayReset();

/** Carry out a display hardware initialization, including a hardware reset. */
/**
 * @brief Initialize the display and perform a hardware reset.
 *
 * This sets up SPI communication and performs the power-on display routine.
 */
void displaySetup();

/**
 * @brief Clear the internal framebuffer  (not the physical display).
 */
void displayClearBuffer();

/**
 * @brief Get a pointer to the internal framebuffer.
 *
 * @return Pointer to a framebuffer array of size DISPLAY_WIDTH.
 */
uint64_t* displayFrameBuffer();

/**
 * @brief Send the entire framebuffer to the display hardware via the SPI bus.
 */
void displayUpdate();

/**
 * @brief Draw a vertical line.
 *
 * @param x Starting X coordinate.
 * @param y Starting Y coordinate.
 * @param length Number of pixels downward to draw.
 */
void displayDrawVerticalLine(uint8_t x, uint8_t y, uint8_t length);

/**
 * @brief Draw a horizontal line.
 *
 * @param x Starting X coordinate.
 * @param y Starting Y coordinate.
 * @param length Number of pixels to the right to draw.
 */
void displayDrawHorizontalLine(uint8_t x, uint8_t y, uint8_t length);

/**
 * @brief Draw a general line with start- and end coordinates.
 *
 * @param x1 Starting X coordinate.
 * @param y1 Starting Y coordinate.
 * @param x2 Ending X coordinate.
 * @param y2 Ending Y coordinate.
 */
void displayDrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

/**
 * @brief Draw an unfilled rectangle.
 *
 * @param x Left coordinate.
 * @param y Top coordinate.
 * @param w Width of the rectangle.
 * @param h Height of the rectangle.
 */
void displayDrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Draw a filled rectangle.
 *
 * @param x Left coordinate.
 * @param y Top coordinate.
 * @param w Width of the rectangle.
 * @param h Height of the rectangle.
 */
void displayDrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Draw a single pixel in the framebuffer.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 */
void displayDrawPixel(uint8_t x, uint8_t y);

/**
 * @brief Render a bitmap image.
 *
 * @param x Left coordinate on screen.
 * @param y Top coordinate on screen.
 * @param bmp Pointer to a bitmap structure.
 */
void displayDrawBitmap(uint8_t x, uint8_t y, const Bitmap* bmp);

/**
 * @brief Render a character string horizontally.
 *
 * @param x Starting X position.
 * @param y Starting Y position.
 * @param str Null-terminated c-string to render.
 */
void displayRenderText(uint8_t x, uint8_t y, const char* str);

/**
 * @brief Render a character string vertically (top→bottom).
 *
 * @param x Starting X position.
 * @param y Starting Y position.
 * @param str Null-terminated c-string to render vertically.
 */
void displayRenderTextVertical(uint8_t x, uint8_t y, const char* str);

/**
 * @brief Print a formatted string horizontally (printf-style).
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param format Format string (printf-compatible).
 * @param ... Format arguments.
 */
void displayPrint(uint8_t x, uint8_t y, const char* format, ...);

/**
 * @brief Print a formatted string vertically (printf-style).
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param format Format string (printf-compatible).
 * @param ... Format arguments.
 */
void displayPrintVertical(uint8_t x, uint8_t y, const char* format, ...);

#endif
