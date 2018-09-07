/*
 * bluetooth.h
 *
 *  Created on: Feb 1, 2016
 *      Author: MJ
 */

#ifndef DRIVERS_BT_UART_H_
#define DRIVERS_BT_UART_H_
#include "bos_app.h"

//bluetooth enable =pta4
//state = pta2

#define BT_EN_LOC  		5
#define BT_STATE_LOC 	 2

#define BT_EN()  		GPIOA_PSOR |= 1<<BT_EN_LOC
#define BT_DIS()  		GPIOA_PCOR |= 1<<BT_EN_LOC

void init_bt();
void bt_uart_putc(uint8_t c);

void bt_send_app_card_scan();
void bt_send_app_status();
void bt_send_app_card_clone();



int8_t bt_uart_putString(uint8_t *ptr);
void bt_putc_uart_printf(void* p, char c);

#endif /* DRIVERS_BT_UART_H_ */
