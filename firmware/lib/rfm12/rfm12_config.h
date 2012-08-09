#ifndef __RFM12_CONFIG_H
#define __RFM12_CONFIG_H

#define RFM12_DATA_LENGTH	244 // Max 254 - 10

#define RFM12_SPI_PORT		PORTB
#define RFM12_SPI_PIN		PINB
#define RFM12_SPI_DDR		DDRB
#define RFM12_SPI_SDI		3
#define RFM12_SPI_SCK		5
#define RFM12_SPI_CS		2
#define RFM12_SPI_SDO		4

#define RFM12_RESET_PORT	PORTD
#define RFM12_RESET_DDR		DDRD
#define RFM12_RESET		PD3

#define RFM12_IRQ_PORT		PORTD
#define RFM12_IRQ_DDR		DDRD
#define RFM12_IRQ		2
#define RFM12_IRQ_INT		INT0
#define RFM12_IRQ_VECT		INT0_vect
#define RFM12_IRQ_SENSE		ISC01 // falling edge

#endif
