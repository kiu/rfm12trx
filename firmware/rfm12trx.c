/*
 * rfm12trx http://schoar.de/tinkering/rfm12trx
*/

#include "rfm12trx.h"

#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "lib/global.h"
#include "lib/uart/uart.h"
#include "lib/rfm12/rfm12.h"

unsigned char tx_buf[RFM12_DATA_LENGTH];
uint8_t tx_pos = 0;

uint8_t tx_high;
uint8_t tx_low;
uint8_t tx_toggle;

uint8_t tx_ready = 0;

uint8_t hexconvert(uint8_t ascii) {
    if (ascii >= '0' && ascii <= '9') {
	return ascii - 48;
    }

    if (ascii >= 'A' && ascii <= 'F') {
	return ascii - 55;
    }

    if (ascii >= 'a' && ascii <= 'f') {
	return ascii - 87;
    }

    return 255;
}

void read(void) {
    if (tx_ready == 1) {
	return;
    }

    while(1) {

	unsigned int ci = uart_getc();

	if (ci & UART_NO_DATA) {
	    return;
	}

	uint8_t cc = (uint8_t)ci;

	if (hexconvert(cc) != 255) {
	    if (tx_toggle == 0) {
		tx_high = hexconvert(cc);
	    } else {
		tx_low = hexconvert(cc);
		tx_buf[tx_pos++] = tx_high * 16 + tx_low;
	    }
	    tx_toggle ^= 1;
	}

	if (cc == '\r' || cc == '\n' || tx_pos >= sizeof(tx_buf) - 1) {
		tx_ready = 1;
		tx_toggle = 0;
		return;
	}

    }
}

void rx(void) {
    uint8_t state;

    state = rfm12_rxstart();

    unsigned char tmp[RFM12_DATA_LENGTH];
    state = rfm12_rxfinish(tmp);
    if (state == 0) {
	return;
    }
    if (state == 255) {
	return;
    }
    if (state == 254) {
	rfm12_allstop();
	return;
    }

    char out[16];
    sprintf(out, "RX: [%02x] ", state);
    uart_puts(out);
    for (uint8_t i=0; i < state; i++) {
	sprintf(out,"%02x", tmp[i]);
	uart_puts(out);
    }
    uart_puts("\r\n");
}

void tx(void) {
    if (tx_ready == 0) {
	return;
    }
    tx_ready = 0;

    uint8_t len = tx_pos;
    tx_pos = 0;
    if (len == 0) {
	return;
    }

    rfm12_allstop();
    if (rfm12_txstart(tx_buf, len) != 0) {
	return;
    }

    while (rfm12_txfinished() != 0) {
    }

    char out[16];
    sprintf(out, "TX: [%02x] ", len);
    uart_puts(out);
    for (uint8_t i=0; i < len; i++) {
	sprintf(out,"%02x", tx_buf[i]);
	uart_puts(out);
    }
    uart_puts("\r\n");
}

int main (void) {

    _delay_ms(2000);

    sei();

    uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE, F_CPU)); 
    uart_puts("\r\nrfm12trx http://schoar.de/tinkering/rfm12trx\r\n");

    rfm12_init();

    rfm12_setfreq(RFM12_FREQ(434.32));
    rfm12_setbandwidth(RxBW200, LNA_6, RSSI_79);
    rfm12_setbaud(19200);
    rfm12_setpower(PWRdB_0, TxBW105);

    while(1) {
	read();

	if (tx_ready != 0) {
	    tx();
	} else {
	    rx();
	}
    }

}
