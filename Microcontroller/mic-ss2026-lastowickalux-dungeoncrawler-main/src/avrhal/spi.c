#include "avrhal/spi.h"
#include "utils/bit.h"

#include <avr/io.h>


void spiSetup()
{
    BIT_SET(SPI_DDR, SPI_MOSI);
    BIT_SET(SPI_DDR, SPI_SCK);
    BIT_CLR(SPI_DDR, SPI_MISO);
    SPSR = (1 << SPI2X); //set Clockrate to 4MHz
    
    SPCR &= ~((1 << DORD) | (1 << CPOL) | (1 << CPHA)); //Data Order MSB first, Mode 0
    SPCR = (1 << SPE) | (1 << MSTR); //enable SPI, Master and Interrupt enable
}

uint8_t spiTransferByte(uint8_t data)
{
    /* Write data byte to register, initiating the transmission */
    SPDR = data;
    /* Wait until the flag-bit is set, indicating a completed data byte transfer */
    while(!BIT_IS_SET(SPSR, SPIF)); //!BIT_SET
    /* Return the byte received from the slave IC */
    return SPDR;
}
