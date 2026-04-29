/**
 * @author MIC Lab Team
 * @brief SPI Master 
 *
 */

#ifndef _AVRHAL_SPI__H__
#define _AVRHAL_SPI__H__

#include <stdint.h>

#define SPI_DDR DDRB

#define SPI_CS PB4
#define SPI_MOSI PB5
#define SPI_MISO PB6
#define SPI_SCK PB7

/** Setup the hardware spi interface as master */
void spiSetup();

/** Transfer a byte on the spi interface
 *  @param[in] to be transmitted byte
 *  
 *  @return Received byte from slave
 */
uint8_t spiTransferByte(uint8_t data);

#endif
