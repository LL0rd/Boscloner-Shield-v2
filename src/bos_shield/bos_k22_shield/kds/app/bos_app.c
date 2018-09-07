/*
 * boscloner.c
 *
 *  Created on: Jan 23, 2016
 *      Author: MJ
 *      states:
 *      bosidle - no background functions running on pm3
 *      - can run any clone commands as they are detected from wiegand/uart
 *      bosbrute
 *      - brute is running in background, cannot clone cards, but can display
 *      detected weigand/uart data on lcd if incoming.
 *      - pushbutton detect.
 *
 */

#include "bos_app.h"

uint8_t pm3_send_cmd(uint8_t cmd_type);
uint8_t brute_mode = 0;
uint8_t f_brute = 0;

void Init_App();

void app_tasks_generic() {
	uint8_t temp;

	temp = pb1_check2();
	if (temp == 1) {	//short push button
		//clone_status ^= 1;	//toggle the clone status.
		//print_clone_status(NULL);
		bos_print_status();
	} else if (temp == 2) { //long push button enter test mode
		//MFG TEST MODE

		//! blink leds.
		test_leds();
		test_bluetooth();
	}

	//BOSLONER UTILITY PROCESSES
	if (TimerIsDone(tmr1_dsp)) { //check is display update time is done
		disp_revert_buf();
		display();
		TimerStop(tmr1_dsp);
	}

}

void app_init_timers() {

	tmr1_dsp = RegisterTimer();
	tmr2_pm3_check = RegisterTimer();

	//start tmr2
	TimerStart(tmr2_pm3_check, PM3_SPI_CHECK_TIME);
}

int main(void) {
	Init_App();

	init_disp_update();
	printf("Startup......\n");
	printf("Boscloner Verion:%s\n", BOSCLONER_VERSION);
	display();

	uint8_t bcount = 0;
	bos_state = BOS_IDLE;

	for (;;) // Forever loop
			{

		app_tasks_generic();

		//periodic check for pm3 status/cmd
		if (TimerIsDone(tmr2_pm3_check)) {

			send_pm3_packet(); //get latest data

			if (spi_cmd.f_rx) {
				spi_cmd.f_rx = 0;
				clearRxBuffer();
			}
			TimerStart(tmr2_pm3_check, PM3_SPI_CHECK_TIME); //restart timer
			//if() data valid data was recieved process it
		}

		//uart em4100 commands
		if (reader_done) {
			print_received_card_em4100();
			void bt_send_app_em4100();
			em4100_process();	//resets uart state machine for next bytes
		}

		//bt uart commands
		if (rx_done) {	//rx uart command
			bt_cmd_process();	//processed the rx uart cmd
			reset_rx_state();
		}

		//hid weigand receive
		if (WiegandDone()) {	//decoded wiegand packet
			wiegand_process();	//process card data received from wiegand
			printRecivedCardWiegand();	//print card data to lcd
			bt_send_app_card_clone();	//send card data to the app
			resetWeigand();		//reset weigand state machine

		}

		if (bos_state == BOS_BRUTE) {
			if (rx_done) {	//rx uart command
				__asm("NOP");
				bt_cmd_process();	//processed the rx uart cmd
				reset_rx_state();
				bt_uart_putString("$!STATUS,MCU ACK?$\r");	//MUST SEND \R IN ORDER FOR THE BT TO SEND THE DATA.
			}
			if (f_brute) {
				f_brute = 0;
				pm3_send_cmd(PM3_CMD_PM3_STOP);
				bos_state = BOS_IDLE;
				clearTxBuffer();

			}
		} //if

	} //for

} //main

//send clone command to the pm3
uint8_t pm3_send_cmd(uint8_t cmd_type) {
	uint8_t ret;

	ret = pm3_process(cmd_type);	//setup and send the command packet.
	bt_send_app_card_clone();	//send data to bluetooth.  //!

	return (ret);
}

void test_send_packets() {
//test_send_packet();	//WORKS!
//test_send2_packet();	//does not work....!
//test_send_packet3();	//works
}

void Init_App() {
	uint32_t val1;

	init_bosio();
	LED1_HIGH;
	LED0_HIGH;

	InitSysTimers();
	DelayMS(100);

	spi_bb_init();	//init spi bit bang pins pins
	LCD_Init();
	init_printf(NULL, putCharLCD);	//add the lcd write() as the callback
	DelayMS(10);
	display();	//enable display
	bosclone_display();

	InitWeigand();
	init_bt_uart();
	init_bt_io();
	init_dspi();

//CHECK THE CLOCK VALUES
	val1 = CLOCK_SYS_GetSystemClockFreq();

//SETUP THE LCD
//LCD_Init();
//bosclone_display();
//display();	//enable display
	init_UART_reader();

	InitWeigandPins();

	app_init_timers();

	LED1_LOW;
	LED0_LOW;

}

uint8_t update_being_displayed() {
	return (TimerIsOn(tmr1_dsp));

}



void delay(int del) {

	for (int i = 0; i < del; i++) {
		for (int j = 0; j < 2000; j++) {
			__asm("NOP");
		}
	}

}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
