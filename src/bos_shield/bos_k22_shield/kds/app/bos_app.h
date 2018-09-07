/*
 * boscloner.h
 *
 *  Created on: Jan 23, 2016
 *      Author: MJ
 */

#ifndef APP_BOS_APP_H_
#define APP_BOS_APP_H_

//global app defines
#include <stdint.h>
#include "ksdk.h"
#include "spi_bb_driver.h"	//bit bang master driver
#include "dspi.h"		//spi slave driver
#include "oled_app.h"
#include "GFX.h"
#include "printf.h"
#include "oled_app.h"
#include "bos_commands.h"
#include "bt_uart.h"
#include "Timers.h"
#include "bos_io.h"
#include "weigand.h"
#include "uart_reader_bm.h"
#include "hid.h"

extern uint8_t brute_mode;

uint8_t tmr1_dsp = 0;
#define DEF_DSP_UPDATE_TIME 2000	//basic updates of system status
#define CARD_ID_UPDATE_TIME 10000	//10 seconds

uint8_t tmr2_pm3_check = 0;
#define PM3_SPI_CHECK_TIME 1000	// 	time ms between checking for spi value from pm3



#define BOSCLONER_VERSION "1.0"

typedef enum bos_state_ {
	BOS_IDLE,	//waiting
	BOS_BRUTE,
	BOS_SIM,
} bos_state_t;
bos_state_t bos_state;

extern bos_state_t bos_state;

uint8_t clone_card(uint8_t clone_type);
uint8_t pm3_send_cmd(uint8_t cmd_type);

#endif /* APP_BOS_APP_H_ */

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
