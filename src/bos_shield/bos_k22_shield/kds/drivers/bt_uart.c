/*
 * bluetooth.c
 *
 *  Created on: Feb 1, 2016
 *      Author: MJ
 */
#include "bt_uart.h"
#include "bos_app.h"




//UART PINS
//#define MK_UART_TXO PTE0
//MK_uart_RXI PTE1
#define MK_UART_TXO_LOC 	0
#define MK_UART_RXI_LOC 	0

#define CMD_DATA_SIZE	32
#define CMD_DATA2_SIZE	16
#define CMD_VAL_SIZE 	16


#define UART_LOOPBACK

#define FRDM_BRD
#define UART_INTERRUPT
//FREEDOM BOARD
//RXI = PTE1  ; TXO = PTE0

#define BAUD_RATE 9600

static volatile uint8_t c, flag;
uint8_t uart_rx_buf[CMD_DATA_SIZE] = { 0 };


#define RX_STATE_WAIT 	0
#define RX_STATE_START	1
#define RX_STATE_DATA	2

volatile uint8_t rx_state = RX_STATE_WAIT;
volatile uint8_t rx_done = 0;
static volatile uint8_t cmd_val_over = 0;




void bt_send_app_em4100() {
	uint8_t * pid5;
	uint8_t temp, a, i;
	//example  uart_putString("$!EM4100,XXXXXXXXXX?$\r");
	pid5 = get_card_id5();
	init_printf(NULL, bt_putc_uart_printf);	//setup to print to uartputc
	bt_uart_putString("$!EM4100,");	//start command
	for (i = 0; i < 5; i++) {
		temp = *pid5++;
		//print nibble at a time
		printf("%1x", temp >> 4);
		printf("%1x", temp & 0xf);
	}
	bt_uart_putString("?$\r");  //end command

	//re-enable print for lcd
	init_printf(NULL, putCharLCD);
}



void bt_send_app_card_scan() {
	uint8_t * pid5;
	uint8_t temp, a, i;
	//example  uart_putString("$!SCAN,a9:78:65:43:21?$\r");
	pid5 = get_card_id5();
	init_printf(NULL, bt_putc_uart_printf);	//setup to print to uartputc
	bt_uart_putString("$!SCAN,");	//start command
	for (i = 0; i < 5; i++) {
		temp = *pid5++;
		//print nibble at a time
		printf("%1x", temp >> 4);
		printf("%1x", temp & 0xf);
		if (i != 4) {
			printf(":");
		}
	}
	bt_uart_putString("?$\r");  //end command

	//re-enable print for lcd
	init_printf(NULL, putCharLCD);

}

void bt_send_app_status() {
	bt_uart_putString("$!STATUS,Clone Done!\r");
}

void bt_send_app_card_clone() {	//output data to the bluetooth.
	uint8_t * pid5;
	uint8_t temp, a, i;
	//example  uart_putString("$!SCAN,a9:78:65:43:21?$\r");
	pid5 = get_card_id5();
	init_printf(NULL, bt_putc_uart_printf);	//setup to print to uartputc
	bt_uart_putString("$!CLONE,");	//start command
	for (i = 0; i < 5; i++) {
		temp = *pid5++;
		//print nibble at a time
		printf("%1x", temp >> 4);
		printf("%1x", temp & 0xf);
		if (i != 4) {
			printf(":");
		}
	}
	bt_uart_putString("?$\r");  //end command

	//re-enable print for lcd
	init_printf(NULL, putCharLCD);

}




volatile uint8_t i_isr = 0;
void UART1_RX_TX_IRQHandler(void) {

	if (UART1_S1 & UART_S1_RDRF_MASK) {
		c = UART1_D;
		flag = 1;
#ifdef UART_LOOPBACK
		UART1_D = c;
#endif
	}

	//decode state machine
	switch (rx_state) {
	case (RX_STATE_WAIT):
		if (c == '$' && rx_done == 0) { //don't start unles the cmd_done is processed (cleared).
			rx_state = RX_STATE_START;
		}
		break;
	case (RX_STATE_START):
		if (c == '!') {
			rx_state = RX_STATE_DATA;
		} else {
			rx_state = RX_STATE_WAIT;
		}

		break;
		//GET THE COMMAND
	case (RX_STATE_DATA):
		if (i_isr > CMD_DATA_SIZE) {
			rx_state = RX_STATE_WAIT;	//reset state machine
			cmd_val_over = 1;	//overflow
			rx_done = 1;
			i_isr = 0;
		} else {
			if (c == '\r') {
				rx_state = RX_STATE_WAIT;	//reset state machine
				rx_done = 1;
			}
			uart_rx_buf[i_isr++] = c;
			__asm("NOP");
		}
		break;
		//GET THE DATA

	default:
		rx_state = RX_STATE_WAIT;
		break;

	}

	//! if

	__asm("NOP");
}

uint8_t * get_rx_buf(){
	return (&uart_rx_buf);
}


void init_bt_io(){
	//bluetooth enable =pta4
	//state = pta2

		SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
		//enable = pta5
		GPIOA_PDDR |= 1 << 5;  //input=0 output=1
		PORTA_PCR5 |= PORT_PCR_MUX(0X01) | PORT_PCR_DSE_MASK; //
		BT_EN();

		//state = pta2
		GPIOA_PDDR |= 0 << 2;  //input=0 output=1
		PORTA_PCR2 = 0;		//for some reason had to write a 0 before it would accept the new values...
		PORTA_PCR2 |= PORT_PCR_MUX(0X01) | PORT_PCR_DSE_MASK; //


}


//init uart1
void init_bt_uart() {
	int ubd, temp;

	SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;	//enable clock to uart

#ifdef FRDM_BRD
//RXI = PTE1  ; TXO = PTE0
	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
//TXO
	GPIOE_PDDR |= 1 << 0;  //input=0 output=1
	PORTE_PCR0 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK; //mux 3 = UART1_TX, drive strength on
//RXI
	GPIOE_PDDR |= 0 << 1;  //input=0 output=1
	PORTE_PCR1 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK; //mux 3 = UART1_RX, drive strength on
#else
			SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
			//TXO
			GPIOE_PDDR |= 1 << 0;//input=0 output=1
			PORTE_PCR0 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK;//mux 3 = UART1_TX, drive strength on
			//RXI
			GPIOE_PDDR |= 0 << 1;//input=0 output=1
			PORTE_PCR1 |= PORT_PCR_MUX(0X03) | PORT_PCR_DSE_MASK;//mux 3 = UART1_RX, drive strength on
#endif

// Calculate baud settings
	ubd = (uint16_t) ((SystemCoreClock) / (BAUD_RATE * 16));
// Shave off the current value of the UARTx_BDH except for the SBR
	UART1_BDH |= UART_BDH_SBR((ubd & 0x1F00) >> 8);
	UART1_BDL = (uint8_t) (ubd & UART_BDL_SBR_MASK);

//SETUP CONTROL REGSITERS - 8N1, RX_ISR
//UART1_C2 |= UART_C2_TCIE_MASK | UART_C2_RIE_MASK; //tx/rx intenrrupt enable

//can setup fifo of size up to 128 words.....uart_pfifo

#ifdef UART_INTERRUPT
	UART1_C2 |= UART_C2_RIE_MASK;
	NVIC_EnableIRQ(UART1_RX_TX_IRQn);		//UART1_IRQn for smaller devices
#endif

//enable transmitter//receiver
	UART1_C2 |= UART_C2_TE_MASK | UART_C2_RE_MASK;

}

void clear_data() {
	uint8_t i;
	for (i = 0; i < CMD_DATA_SIZE; i++) {
		uart_rx_buf[i] = 0;
	}
}


void reset_rx_state() {
	rx_done = 0;
	i_isr = 0;
	clear_cmd_val();
	clear_data();
	clear_hex_vals();
	UART1_C2 |= UART_C2_RE_MASK | UART_C2_TE_MASK;

}

void uart_disable_rxint() {
	UART1_C2 &= ~UART_C2_RIE_MASK;
}

void uart_enable_rxint() {
	UART1_C2 |= UART_C2_RIE_MASK;
}


//!wrapper to allow printf to output to the lcd without modifying the function structure.
void bt_putc_uart_printf(void* p, char c) {
	uart_putc(c);
}

void uart_putc(uint8_t c) {
	while (!(UART1_S1 & UART_S1_TC_MASK))
		;
	UART1_D = c;
}

void bt_uart_putc(uint8_t c) {
	while (!(UART1_S1 & UART_S1_TC_MASK))
		;
	UART1_D = c;
}

int8_t bt_uart_putString(uint8_t *ptr) {
	uint32_t count = 0;
	while (*ptr != 0) {
		while (!(UART1_S1 & UART_S1_TDRE_MASK)) {
			count++;
			if (count > 10000) {
				return (-1);
			}
		}
		uart_putc(*ptr++);
	}
	return (1);
}

int16_t bt_uart_getc_timeout(uint32_t ms) {
	uint32_t j = 0;
	uint32_t count_ms;
	uint64_t temp, i = 0;
	uint8_t c = 0;
	count_ms = SystemCoreClock / (1000); //clock/(#ticks per s of Us)
	temp = count_ms * ms;  //not working.. tooo long!
	while (!(UART1->S1 & UART_S1_RDRF_MASK)) {
		i++;
		if (i > (temp / 100)) {
			return -1;
		}
	}
	c = UART1->D;
	return (c);
}





#define BT_DEL 0
/*
void bt_test_messages() {

	uart_putString("$!SCAN,a9:78:65:43:21?$\r");
	DelayMS(BT_DEL);
	uart_putString("$!CLONE,a9:78:65:43:22?$\r");
	DelayMS(BT_DEL);
	uart_putString("$!STATUS,MCU STARTUP?$\r");
	DelayMS(BT_DEL);
	uart_putString("$!SCAN,a9:78:65:43:23?$\r");
	DelayMS(BT_DEL);
	uart_putString("$!CLONE,a9:78:65:43:24?$\r");
	DelayMS(BT_DEL);
	uart_putString("$!STATUS,Done!?$\r");
	DelayMS(2000);
}
*/
