/*
 /*
 * uart_hal.c
 *
 *  Created on: Dec 21, 2015
 *      Author: MJ
 */

#include "MK22F51212.h"
#include "bos_app.h"
#include "uart_reader_bm.h"
#include "string.h"
#include "printf.h"
#include "weigand.h"

//prototypes
void ascii_array_tohex(uint8_t * pArray);
uint32_t get_card_hi_app();
uint32_t get_card_lo_app();
uint8_t charToHexDigit_test(uint8_t c);
unsigned char stringToByte(char c[2]);
void init_UART_reader();
void putc_uart_reader_printf(void* p, char c);

//varaiables
//#define UART_LOOPBACK
uint8_t reader_rx[READER_BUF_LEN] = { 0 };	//reader rx buffer
#define BAUD_RATE 9600
uint8_t reader_done = 0;
volatile uint8_t buf_i = 0, c = 0, started = 0;

//data saved in ascii, hex data, 10 bytes
uint8_t * get_em4100_buf() {
	return (&reader_rx);
}

void em4100_store() {
	//store in the hid storage.
	uint8_t i;
	uint64_t id = 0;

	//id = strtol(reader_rx);
	//store msb low in array
	for (i = 0; i < 5; i++) {
		hid.bytes[i]  = (reader_rx[(2*i)+1]<<8) | reader_rx[2*i]  ; //0
	}

}

void em4100_process() {
	//em4100_store();
	reader_buf_restart();
	//send data out bt uart

	//display data on the lcd
}

void reader_buf_restart() {
	uint8_t i = 0;

	//restart buffer fill vars
	reader_done = 0;
	buf_i = 0;
	started = 0;

	for (i = 0; i < READER_BUF_LEN; i++) {
		reader_rx[i] = 0;
	}

	//c = UART0_D;//clear buffer
	//c = UART0_D;
	//c = UART0_D;
	//UART0_D = 0;
	UART0_C2 |= UART_C2_RE_MASK; //enable interrupt again
}

//interrupt
void UART0_RX_TX_IRQHandler(void) {

	if (UART0_S1 & UART_S1_RDRF_MASK) {
		c = UART0_D;
#ifdef UART_LOOPBACK
		UART0_D = c;
#endif
	}

	if (started == 0) {	//start collecting the data - first time in loop
		started = 1;
		//ignore first byte = 0x02 start is em4100
	} else {
		if (c == '\n') {
			reader_done = 1;
			UART0_C2 &= ~UART_C2_RE_MASK;  //disable receiver until processed
		} else if (buf_i >= 16) {  //buffer overrun
			//error
			__asm("NOP");

		} else {
			if (reader_done == 0) { //in case interrupt occurrs while not processed, dont write to buf
				reader_rx[buf_i++] = c;
			}
		}

	}

	__asm("NOP");
}

void uart_reader_disable_rxint() {
	UART0_C2 &= ~UART_C2_RIE_MASK;
}

void uart_reader_enable_rxint() {
	UART0_C2 |= UART_C2_RIE_MASK;
}

//init uart0  //PTB16=RX   PTB17=TX
void init_UART_reader() {
	int ubd, temp;

	SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;	//enable clock to uart

//RXI = PTB16  ; TXO = PTB17
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
//TXO
	GPIOB_PDDR |= 1 << 17;  //input=0 output=1
	PORTB_PCR17 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK; //mux 3 = UART0_TX, drive strength on
//RXI
	GPIOB_PDDR |= 0 << 16;  //input=0 output=1
	PORTB_PCR16 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK; //mux 3 = UART0_RX, drive strength on

// Calculate baud settings
	ubd = (uint16_t) ((SystemCoreClock) / (BAUD_RATE * 16));
// Shave off the current value of the UARTx_BDH except for the SBR
	UART0_BDH |= UART_BDH_SBR((ubd & 0x1F00) >> 8);
	UART0_BDL = (uint8_t) (ubd & UART_BDL_SBR_MASK);

//SETUP CONTROL REGSITERS - 8N1, RX_ISR
//UART0_C2 |= UART_C2_TCIE_MASK | UART_C2_RIE_MASK; //tx/rx intenrrupt enable

//can setup fifo of size up to 128 words.....uart_pfifo

//#ifdef UART_INTERRUPT
	UART0_C2 |= UART_C2_RIE_MASK;
	NVIC_EnableIRQ(UART0_RX_TX_IRQn);		//UART0_IRQn for smaller devices
//#endif

//enable transmitter//receiver
	UART0_C2 |= UART_C2_TE_MASK | UART_C2_RE_MASK;

}

//!wrapper to allow printf to output to the lcd without modifying the function structure.
void bt_putc_uart_printf(void* p, char c) {
	uart_putc(c);
}

void uart_putc(uint8_t c) {
	while (!(UART0_S1 & UART_S1_TC_MASK))
		;
	UART0_D = c;
}

int8_t uart_reader_putString(uint8_t *ptr) {
	uint32_t count = 0;
	while (*ptr != 0) {
		while (!(UART0_S1 & UART_S1_TDRE_MASK)) {
			count++;
			if (count > 10000) {
				return (-1);
			}
		}
		uart_putc(*ptr++);
	}
	return (1);
}

int16_t uart_reader_getc_timeout(uint32_t ms) {
	uint32_t j = 0;
	uint32_t count_ms;
	uint64_t temp, i = 0;
	uint8_t c = 0;
	count_ms = SystemCoreClock / (1000); //clock/(#ticks per s of Us)
	temp = count_ms * ms;  //not working.. tooo long!
	while (!(UART0->S1 & UART_S1_RDRF_MASK)) {
		i++;
		if (i > (temp / 100)) {
			return -1;
		}
	}
	c = UART0->D;
	return (c);
}

