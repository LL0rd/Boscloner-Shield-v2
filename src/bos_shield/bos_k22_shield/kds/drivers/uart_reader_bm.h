/*
 * uart_bm.h
 *
 *  Created on: Dec 21, 2015
 *      Author: MJ
 */

#ifndef SOURCES_UART__READER_BM_H_
#define SOURCES_UART_READER_BM_H_


extern volatile uint8_t clone_status;

extern volatile uint8_t rx_done;
extern volatile uint8_t cmd_data[];


extern uint8_t reader_done;
void em4100_process();
void uart_reader_putc(uint8_t c);
int8_t uart_reader_putString(uint8_t *ptr);
void putc_uart_reader_printf ( void* p, char c);
void init_UART_reader();

#define READER_BUF_LEN 16
extern uint8_t reader_rx[READER_BUF_LEN];	//reader rx buffer

#endif /* SOURCES_UART_BM_H_ */
