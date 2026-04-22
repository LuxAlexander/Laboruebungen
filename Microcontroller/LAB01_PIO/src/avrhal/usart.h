#ifndef usart_h_zidaSSgah23o7ier
#define usart_h_zidaSSgah23o7ier

#include <stdbool.h>
#include <stdint.h>

typedef enum { USART_B9600 = 9600 } UsartBaudrate;
typedef enum { USART_CONFIG_8N1 } UsartConfig;

/** Setup and enable the usart interface
 * @param[in] baud The desired baudrate
 * @param[in] config The interface configuration (example: 8 data bits, no parity, 1 stop bit)
*/
void usartSetup(UsartBaudrate baud, UsartConfig config);

/** Write a string to the usart interface.
 * @note The input string must be zero-terminated (`\0`). The trailing `\0` is not transmitted.
 * @param[in] str A zero-terminated string
 * @return The number of characters written
*/
uint8_t usartWriteString(const char* str);

/** Print a formatted string to the usart interface.
 * A trailing \r\n sequence is NOT appended (also known as DOS/Windows line ending).
 * @note Internally va_list(), va_start(), vsnprintf() and va_end() are used.
 * @param[in] format A printf-style format string
 * @return The number of characters written of the resulting string
*/
uint8_t usartPrint(const char* format, ...);

#endif
