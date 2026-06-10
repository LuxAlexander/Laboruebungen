/**
 * @author MIC Lab Team - Olaf Sassnick
 * @brief OLED Display Driver with framebuffer
 *
 */

#include "avrhal/display.h"
#include "avrhal/font8x8.h"
#include "avrhal/font8x8vertical.h"
#include "avrhal/spi.h"

#include "utils/bit.h"
#include "utils/math.h"
#include <avr/io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#define RESET_PIN PB3
#define DATA_CMD_PIN PB4

/* Display RAM has 132 columns, while only 128 are visible. Therefore skip the first 2 non-visible columns   */
#define DISPLAY_NONVISIBLE_BORDER_OFFSET 2

#define DISPLAY_PRINT_TEXT_BUFFER_SIZE 64

#define DISPLAY_DEFAULT_CONTRAST 128

typedef enum {
    SH1106_SET_CONTRAST = 0x81,
    SH1106_ENTIRE_DISPLAY_ON_DISABLE = 0xA4,
    SH1106_ENTIRE_DISPLAY_ON = 0xA5,
    SH1106_NORMAL_DISPLAY = 0xA6,
    SH1106_REVERSE_DISPLAY = 0xA7,
    SH1106_SET_DISPLAY_OFF = 0xAE,
    SH1106_SET_DISPLAY_ON = 0xAF,

    SH1106_SET_DISPLAY_OFFSET = 0xD3,
    SH1106_SET_COMPINS = 0xDA,

    SH1106_SETVCOMDETECT = 0xDB,

    SH1106_SETDISPLAYCLOCKDIV = 0xD5,
    SH1106_SETPRECHARGE = 0xD9,

    SH1106_SET_MULTIPLEX_RATIO = 0xA8,

    SH1106_SET_PAGE_COLUMN_MASK = 0x0F,
    SH1106_SET_PAGE_COLUMN_H = 0x10,
    SH1106_SET_PAGE_COLUMN_L = 0x00,
    SH1106_SET_PAGE = 0xB0,
    SH1106_SETSTARTLINE = 0x40,

    SH1106_SET_ADDRESSING_MODE = 0x20,
    SH1106_SET_COLUMN_ADDR = 0x21,
    SH1106_SET_PAGE_ADDR = 0x22,

    SH1106_COMSCANINC = 0xC0,
    SH1106_COMSCANDEC = 0xC8,

    SH1106_SEGMENT_REMAP = 0xA0,

    SH1106_CHARGEPUMP = 0x8D,

    SH1106_EXTERNALVCC = 0x1,
    SH1106_SWITCHCAPVCC = 0x2
} DisplaySH1106Command;

typedef enum {
    SH1106_ADDRESSING_MODE_PAGE = 0b10
} DisplaySH1106AddressingMode;

/** Signal, that the to be transmitted byte is data. 
 * This is accomplished by setting th D/C pin HIGH. */
void displaySetDataIndicator()
{
    BIT_SET(PORTB, DATA_CMD_PIN);
}
/** Signal, that the to be transmitted byte is a command. 
 * This is accomplished by setting th D/C pin LOW. */
void displaySetCommandIndicator()
{
    BIT_CLR(PORTB, DATA_CMD_PIN);
}
/** Send a single-byte command. */
void displaySendCommand(DisplaySH1106Command cmd)
{
    displaySetCommandIndicator();
    spiTransferByte((uint8_t)cmd);
}
/** Send a single data byte. */
void displaySendData(uint8_t data)
{
    displaySetDataIndicator();
    spiTransferByte(data);
}

/** Select the addressing mode. For the SH1106 there is only the page-based addressing mode available.
 * The initial SSD1306 also supports a vertical and horizontal addressing mode.
 *
 * @param[in] addressingMode 
 */
void displaySetAddressingMode(DisplaySH1106AddressingMode addressingMode)
{
    spiTransferByte(SH1106_SET_ADDRESSING_MODE | (uint8_t)addressingMode);
}

/** Before writing to the display RAM, the page and column need to be selected.
 * Each page has a size of 132x8 pixel. In total there are 8 pages stacked 
 * on top of each other, realizing 64 pixel display height.
 * A column represents an 8 pixel vertical slice of a page.
 *
 * @param[in] page (value between 0-7)
 * @param[in] startColumn (value between 0-127)
 */
void displaySelectPageAndStartColumn(uint8_t page, uint8_t startColumn)
{
    const uint8_t nibbleMask = 0b1111;
    displaySendCommand(SH1106_SET_PAGE | (page & nibbleMask));
    displaySendCommand(SH1106_SET_PAGE_COLUMN_L | (startColumn & nibbleMask));
    displaySendCommand(SH1106_SET_PAGE_COLUMN_H | ((startColumn >> 4) & nibbleMask));
}
/** By setting the start line (i.e. the upmost pixel row), the first RAM data row can be mapped to a lower pixel row
 *
 * @param[in] start line - the physical pixel row on which the upmost RAM data should appear on
 */
void displaySetStartLine(uint8_t line)
{
    const uint8_t lineMask = 64 - 1;
    displaySendCommand(SH1106_SETSTARTLINE | (line & lineMask));
}

/** Set the display brightness/contrast. Default value ist 128.
 *
 * @param[in] contrast value (value range 0 - 255)
 */
void displaySetContrast(uint8_t contrast)
{
    displaySendCommand(SH1106_SET_CONTRAST);
    displaySendCommand(contrast);
}

/** Carry out a display hardware reset, by toggling the RESET_PIN */
void displayReset()
{
    /* The SSD1306 manual states a required delay of 3 us - we are a bit more generous */
    BIT_SET(PORTB, RESET_PIN);
    _delay_ms(1);
    BIT_CLR(PORTB, RESET_PIN);
    _delay_ms(1);
    BIT_SET(PORTB, RESET_PIN);
    _delay_ms(1);
}

/** Carry out a display hardware initialization, including a hardware reset. */
void displaySetup()
{
    spiSetup();
    DDRB |= (1 << RESET_PIN) | (1 << DATA_CMD_PIN);
    displayReset();
    displaySendCommand(SH1106_SET_DISPLAY_OFF);
    displaySetStartLine(0);
    displaySendCommand(SH1106_SET_CONTRAST);
    displaySendCommand(SH1106_NORMAL_DISPLAY);
    displayUpdate();
    displaySendCommand(SH1106_SET_DISPLAY_ON);
}

static uint64_t frameBuffer[DISPLAY_WIDTH]; /*One 64 bit value represents a vertical line of the display */

uint64_t* displayFrameBuffer()
{
    return frameBuffer;
}

void displayClearBuffer()
{
    for (uint8_t i = 0; i < DISPLAY_WIDTH; ++i) {
        frameBuffer[i] = 0;
    }
}

void displayUpdate()
{
    /* The display memory is organized in pages. For the SH1106 there exist 8 pages.
        Each page has a size of 132x8 pixel. In total there are 8 pages stacked 
        on top of each other, realizing 64 pixel display height.
        A column represents an 8 pixel vertical slice of a page. */

    /* The SSD1306 additionally supports a vertical addressing mode, 
     which should give a small performance advantage, 
     unfortunately it is unavailable on the SH1106 (the clone we are using). */
    displaySetAddressingMode(SH1106_ADDRESSING_MODE_PAGE);

    for (uint8_t page = 0; page < DISPLAY_PAGES; ++page) {
        displaySelectPageAndStartColumn(page, DISPLAY_NONVISIBLE_BORDER_OFFSET);

        displaySetDataIndicator();
        uint8_t* firstPageColumnPtr = ((uint8_t*)frameBuffer) + page;
        for (uint8_t i = 0; i < DISPLAY_WIDTH; ++i) {
            const uint8_t pageColumn = *(firstPageColumnPtr + DISPLAY_PAGES * i);
            spiTransferByte(pageColumn);
        }
    }
}

void displayDrawVerticalLine(uint8_t x, uint8_t y, uint8_t length)
{
    if (x >= DISPLAY_WIDTH) {
        return;
    }
    const uint64_t val = (1ull << length) - 1; /* (2 ^ length) -1 */

    frameBuffer[x] |= val << y;
}

void displayDrawHorizontalLine(uint8_t x, uint8_t y, uint8_t length)
{
    const uint64_t val = 1ull << y;

    for (uint8_t i = x; i < x + length && i < DISPLAY_WIDTH; ++i) {
        frameBuffer[i] |= val;
    }
}

void displayDrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    const int8_t xDelta = x2 - x1;
    const int8_t xSign = signInt8(xDelta);
    const int8_t yDelta = y2 - y1;
    const int8_t ySign = signInt8(yDelta);

    const bool horizontalDrawingMode = absInt8(xDelta) > absInt8(yDelta);

    if (horizontalDrawingMode) {
        for (uint8_t y = y1, x = x1; x < DISPLAY_WIDTH; x += xSign) {
            const int8_t yOffset = ((x - x1) * yDelta) / xDelta;
            y = y1 + yOffset;
            frameBuffer[x] |= 1ull << (y);
            if (x == x2) {
                break;
            }
        }
    } else {
        for (uint8_t y = y1, x = x1; x < DISPLAY_WIDTH; y += ySign) {
            const int8_t xOffset = ((y - y1) * xDelta) / yDelta;
            x = x1 + xOffset;
            frameBuffer[x] |= 1ull << (y);
            if (y == y2) {
                break;
            }
        }
    }
}

void displayDrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    displayDrawVerticalLine(x, y, h);
    displayDrawVerticalLine(x + w - 1, y, h);

    const uint64_t val = (1ull << y) | (1ull << (y + h - 1));

    for (uint8_t i = x; i < x + w && i < DISPLAY_WIDTH; ++i) {
        frameBuffer[i] |= val;
    }
}

void displayDrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = x; i < x + w && i < DISPLAY_WIDTH; ++i) {
        displayDrawVerticalLine(i, y, h);
    }
}

void displayDrawPixel(uint8_t x, uint8_t y)
{
    frameBuffer[x] |= 1ull << y;
}

void displayDrawBitmap(uint8_t x, uint8_t y, const Bitmap* bmp)
{
    const uint16_t len = bmp->height * bmp->width / bmp->dataSize;
    uint8_t col = x;
    uint8_t row = 0;
    for (uint16_t i = 0; i < len && col < DISPLAY_WIDTH; ++i) {
        const uint8_t val = pgm_read_byte(&bmp->data[i]);
        frameBuffer[col] |= ((uint64_t)val) << (bmp->height - row - bmp->dataSize + y);

        row += (bmp->dataSize);
        if (row >= (bmp->height)) {
            col += 1;
            row = 0;
        }
    }
}

void displayRenderText(uint8_t x, uint8_t y, __attribute__((unused)) const char* str)
{
    const uint8_t xStart = x;
    const FontSpec* fontSpec = font8x8();

    for (uint8_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] < fontSpec->firstChar || str[i] > fontSpec->lastChar) {
            if (str[i] == '\n') {
                /* Start next line */
                x = xStart;
                y += fontSpec->charSize;
            } else {
                /* Character not available - skip */
                x += fontSpec->charSize;
            }
            continue;
        }
        const uint16_t charDataStartIdx = fontSpec->charSize * (str[i] - fontSpec->firstChar);
        const uint8_t* character = &(fontSpec->data[charDataStartIdx]);

        for (uint8_t j = 0; j < fontSpec->charSize && j + x < DISPLAY_WIDTH; ++j) {
            const uint64_t charColumn = pgm_read_byte(&character[j]);
            frameBuffer[x + j] |= charColumn << y;
        }
        x += fontSpec->charSize;
    }
}

void displayRenderTextVertical(uint8_t x, uint8_t y, __attribute__((unused)) const char* str)
{
    const uint8_t yStart = y;
    const FontSpec* fontSpec = font8x8vertical();

    for (uint8_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] < fontSpec->firstChar || str[i] > fontSpec->lastChar) {
            if (str[i] == '\n') {
                /* Start next line */
                y = yStart;
                x -= fontSpec->charSize;
            } else {
                /* Character not available - skip */
                y += fontSpec->charSize;
            }
            continue;
        }
        const uint16_t charDataStartIdx = fontSpec->charSize * (str[i] - fontSpec->firstChar);
        const uint8_t* character = &(fontSpec->data[charDataStartIdx]);

        for (uint8_t j = 0; j < fontSpec->charSize && j + x < DISPLAY_WIDTH; ++j) {
            const uint64_t charColumn = pgm_read_byte(&character[j]);
            frameBuffer[x + j] |= charColumn << y;
        }
        y += fontSpec->charSize;
    }
}

void displayPrintVertical(uint8_t x, uint8_t y, const char* format, ...)
{
    char buffer[DISPLAY_PRINT_TEXT_BUFFER_SIZE];

    /* Initialize buffer with zero-length string, in case vsnprintf() fails */
    buffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, DISPLAY_PRINT_TEXT_BUFFER_SIZE, format, args);
    va_end(args);

    displayRenderTextVertical(x, y, buffer);
}

void displayPrint(uint8_t x, uint8_t y, const char* format, ...)
{
    char buffer[DISPLAY_PRINT_TEXT_BUFFER_SIZE];

    /* Initialize buffer with zero-length string, in case vsnprintf() fails */
    buffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, DISPLAY_PRINT_TEXT_BUFFER_SIZE, format, args);
    va_end(args);

    displayRenderText(x, y, buffer);
}
