/*
 * test.c
 *
 *  Created on: Mar 3, 2017
 *      Author: MJ
 */

#include "test.h"

#define TEST_BLINK_COUNT 4
#define RX_LEN 2

const uint8_t RX2[3] = {0xff,0xff,0};


void test_leds() {
	uint8_t count = 0;
	uint8_t i = 0;

	init_disp_update(); //saves buffer, clears scrren
	TimerStart(tmr1_dsp, CARD_ID_UPDATE_TIME);//set custom time to timeout, not default time used
	setTextSize(2);
	printf("Blink LEDs..\n");
	display();

	for (i = 0; i < TEST_BLINK_COUNT; i++) {
		LED1_HIGH;
		LED0_HIGH;
		DelayMS(200);
		LED1_LOW;
		LED0_LOW;
		DelayMS(200);
	}

}


int8_t test_bluetooth() {
	uint8_t rx[RX_LEN+1] = { 0 };
	uint8_t i = 0, count = 0, timeout = 0;


	uart_disable_rxint();
	bt_uart_putString("AT");  //should receive "OK"

	//get the response:
	for (i = 0; i < RX_LEN; i++) {
		rx[i]  = bt_uart_getc_timeout(1000);
		if (rx[i] == -1) {  //timeout
			timeout = 1;
			break;
		}
	}
	uart_enable_rxint();



	init_disp_update(); //saves buffer, clears scrren
	TimerStart(tmr1_dsp, CARD_ID_UPDATE_TIME);//set custom time to timeout, not default time used
	setTextSize(2);


	if (!strcmp("OK", rx) || !strcmp(RX2, rx)) {
		__asm("NOP");
		printf("BT Success: \n%s", rx);
		//success.
		bt_uart_putString("BT Send Data OK?$\r");
	}
	else
		printf("BT failed: \n%s", rx);

	display();
}





