#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "rfm12_config.h"
#include "rfm12.h"
#include "../global.h"

struct RFM12_stati {
    unsigned char Rx:1;
    unsigned char Tx:1;
    unsigned char New:1;
};

struct RFM12_stati RFM12_status;
volatile unsigned char RFM12_Index = 0;
unsigned char RFM12_Data[RFM12_DATA_LENGTH + 10]; // +10 == paket overhead

// -----------------------------------------------------------------------------

unsigned int crcUpdate(unsigned int crc, unsigned char data) {
    unsigned int tmp;
    unsigned char i;

    tmp = data << 8;
    for (i=0; i<8; i++) {
	if ((crc^tmp) & 0x8000) {
	    crc = (crc<<1) ^ 0x1021;
	} else {
	    crc = crc << 1;
	}
	tmp = tmp << 1;
    }
    return crc;
}

ISR(RFM12_IRQ_VECT) {
    if(RFM12_status.Rx) {
	if(RFM12_Index < RFM12_DATA_LENGTH) {
	    unsigned char c = rfm12_trans(0xB000) & 0x00FF;
	    if (RFM12_Index == 0 && c > RFM12_DATA_LENGTH) {
		c = RFM12_DATA_LENGTH;
	    }
	    RFM12_Data[RFM12_Index++] = c;
	} else {
	    rfm12_trans(0x8208);
	    RFM12_status.Rx = 0;
	}
	if(RFM12_Index >= RFM12_Data[0] + 3) { //EOT
	    rfm12_trans(0x8208);
	    RFM12_status.Rx = 0;
	    RFM12_status.New = 1;
	}
    } else if(RFM12_status.Tx) {
	rfm12_trans(0xB800 | RFM12_Data[RFM12_Index]);
	if(!RFM12_Index) {
	    RFM12_status.Tx = 0;
	    rfm12_trans(0x8208); // TX off
	} else {
	    RFM12_Index--;
	}
    } else {
	rfm12_trans(0x0000); //dummy read
    }
}

// -----------------------------------------------------------------------------

unsigned short rfm12_trans(unsigned short dataOut) {
    unsigned char i;
    unsigned short dataIn = 0;

    cbi(RFM12_SPI_PORT, RFM12_SPI_CS);
    for (i=0; i<16; i++) {
	if (dataOut & 32768) {
	    sbi(RFM12_SPI_PORT, RFM12_SPI_SDI);
	} else {
	    cbi(RFM12_SPI_PORT, RFM12_SPI_SDI);
	}
	dataIn<<=1;
	if (RFM12_SPI_PIN & (1<<RFM12_SPI_SDO)) {
	    dataIn |= 1;
	}
	sbi(RFM12_SPI_PORT, RFM12_SPI_SCK);
	dataOut <<= 1;
	asm("nop");
	cbi(RFM12_SPI_PORT, RFM12_SPI_SCK);
    }
    sbi(RFM12_SPI_PORT, RFM12_SPI_CS);

    return dataIn;
}

// -----------------------------------------------------------------------------

void rfm12_reset(void) {
    sbi(RFM12_RESET_PORT, RFM12_RESET);
    _delay_ms(200);

    cbi(RFM12_RESET_PORT, RFM12_RESET);
    _delay_ms(10);

    sbi(RFM12_RESET_PORT, RFM12_RESET);
    _delay_ms(200);
}

void rfm12_init(void) {
    RFM12_RESET_DDR |= (1<<RFM12_RESET);

    sbi(RFM12_SPI_DDR, RFM12_SPI_SDI);
    sbi(RFM12_SPI_DDR, RFM12_SPI_SCK);
    sbi(RFM12_SPI_DDR, RFM12_SPI_CS);
    cbi(RFM12_SPI_DDR, RFM12_SPI_SDO);
    sbi(RFM12_SPI_PORT, RFM12_SPI_CS);

    cbi(RFM12_IRQ_DDR, RFM12_IRQ);

    rfm12_reset();

    rfm12_trans(0x0000);
//    rfm12_trans(0x80D8); // Enable FIFO

    rfm12_trans(0xC0E0);                 // AVR CLK: 10MHz
    rfm12_trans(0x80D7);                 // Enable FIFO
    rfm12_trans(0xC2AB);                 // Data Filter: internal
    rfm12_trans(0xCA81);                 // Set FIFO mode
    rfm12_trans(0xE000);                 // disable wakeuptimer
    rfm12_trans(0xC800);                 // disable low duty cycle
    rfm12_trans(0xC4F7);                 // AFC settings: autotuning: -10kHz...+7,5kHz

    rfm12_trans(0x0000);

    RFM12_status.Rx = 0;
    RFM12_status.Tx = 0;
    RFM12_status.New = 0;

    sbi(MCUCR, RFM12_IRQ_SENSE);
    sbi(GICR, RFM12_IRQ_INT);
}

// -----------------------------------------------------------------------------

void rfm12_setbandwidth(unsigned char bandwidth, unsigned char gain, unsigned char drssi) {
    rfm12_trans(0x9400|((bandwidth&7)<<5)|((gain&3)<<3)|(drssi&7));
}

void rfm12_setfreq(unsigned short freq) {
    if (freq<96) { // 430,2400MHz
	freq=96;
    }
    if (freq>3903) { // 439,7575MHz
	freq=3903;
    }
    rfm12_trans(0xA000|freq);
}

void rfm12_setbaud(unsigned short baud) {
    if (baud<663) {
	return;
    }
    if (baud<5400) { // Baudrate= 344827,58621/(R+1)/(1+CS*7)
	rfm12_trans(0xC680|((43104/baud)-1));
    } else {
	rfm12_trans(0xC600|((344828UL/baud)-1));
    }
}

void rfm12_setpower(unsigned char power, unsigned char mod) {
    rfm12_trans(0x9800|(power&7)|((mod&15)<<4));
}

// -----------------------------------------------------------------------------

unsigned char rfm12_rxstart(void) {
    if(RFM12_status.New)  {
	return(1); // buffer not yet empty
    }
    if(RFM12_status.Tx) {
	return(2); // tx in action
    }
    if(RFM12_status.Rx) {
	return(3); // rx already in action
    }

    rfm12_trans(0x82C8); // RX on
    rfm12_trans(0xCA81); // set FIFO mode
    rfm12_trans(0xCA83); // enable FIFO

    RFM12_Index = 0;
    RFM12_status.Rx = 1;

    return(0);
}

unsigned char rfm12_rxfinish(unsigned char *data) {
    unsigned int crc, crc_chk = 0;
    unsigned char i;

    if(RFM12_status.Rx) {
	return(255); // not finished yet
    }
    if(!RFM12_status.New) {
	return(254); // old buffer
    }

    for(i=0; i<RFM12_Data[0] + 1; i++) {
	crc_chk = crcUpdate(crc_chk, RFM12_Data[i]);
    }

    crc = RFM12_Data[i++];
    crc |= RFM12_Data[i] << 8;

    RFM12_status.New = 0;

    if(crc != crc_chk) {
	return(0); //crc err -or- strsize
    }

    for(i=0; i<RFM12_Data[0]; i++) {
	data[i] = RFM12_Data[i+1];
    }

    return(RFM12_Data[0]); // strsize
}

// -----------------------------------------------------------------------------

unsigned char rfm12_txstart(unsigned char *data, unsigned char size) {
    unsigned char i, l;
    unsigned int crc;

    if(RFM12_status.Tx) {
	return(2); // tx in action
    }
    if(RFM12_status.Rx) {
	return(3); // rx already in action
    }
    if(size > RFM12_DATA_LENGTH) {
	return(4); // str to big to transmit
    }

    RFM12_status.Tx = 1;
    RFM12_Index = size + 9; // act -10 

    i = RFM12_Index;
    RFM12_Data[i--] = 0xAA;
    RFM12_Data[i--] = 0xAA;
    RFM12_Data[i--] = 0xAA;
    RFM12_Data[i--] = 0x2D;
    RFM12_Data[i--] = 0xD4;
    RFM12_Data[i--] = size;
    crc = crcUpdate(0, size);
    for(l=0; l<size; l++) {
	RFM12_Data[i--] = data[l];
	crc = crcUpdate(crc, data[l]);
    }
    RFM12_Data[i--] = (crc & 0x00FF);
    RFM12_Data[i--] = (crc >> 8);
    RFM12_Data[i--] = 0xAA;
    RFM12_Data[i--] = 0xAA;

    rfm12_trans(0x8238); // TX on

    return(0);
}

unsigned char rfm12_txfinished(void) {
    if(RFM12_status.Tx) {
	return(255); // not yet finished
    }
    return(0);
}

// -----------------------------------------------------------------------------

void rfm12_allstop(void) {
    rfm12_trans(0x8208); // shutdown all
    RFM12_status.Rx = 0;
    RFM12_status.Tx = 0;
    RFM12_status.New = 0;
    rfm12_trans(0x0000); // dummy read
}

// -----------------------------------------------------------------------------
