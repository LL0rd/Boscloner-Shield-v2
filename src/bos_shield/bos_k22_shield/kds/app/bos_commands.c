/*
 /*
 * uart_hal.c
 *
 *  Created on: Dec 21, 2015
 *      Author: MJ
 */

#include "MK22F51212.h"
#include "bos_app.h"
#include "string.h"
#include "printf.h"

void clear_data();
void bt_cmd_process();
uint16_t atohex(uint8_t c);
void ascii_array_tohex(uint8_t * pArray);
uint32_t get_card_hi_app();
uint32_t get_card_lo_app();
uint8_t charToHexDigit_test(uint8_t c);
unsigned char stringToByte(char c[2]);
void delay(int del);
void reset_cmd_state();
void clear_hex_vals();
void clear_cmd_val();
void clear_data();
void init_UART();
void bt_putc_uart_printf(void* p, char c);

#define APP_CARD_CLONE_STRING "APP CLONE"
#define APP_CLONE_STATUS_STRING "APP Change Status"

//bluetooth ascii commands
#define BT_CMD_CLONE		"CLONE_HID"
#define BT_CMD_CLONE_DIS	"DISABLE_CLONE"	//sent from android app
#define BT_CMD_CLONE_EN		"ENABLE_CLONE"	//sent from android app
#define BT_CMD_SCAN			"SCAN"
#define BT_CMD_SYNC			"SYNC"
#define BT_CMD_BRUTE_HID	"BRUTE_HID"
#define BT_CMD_HID_SIM		"SIM_HID"
#define BT_CMD_SIM_STOP		"SIM_STP"
#define BT_CMD_BOS_STOP		"PM3_STP"		//stop any bos processes on the Pm3
#define BT_CMD_CLONE_EM4100	"CLONE_EM4100"
#define BT_CMD_SIM_EM4100	"SIM_EM4100"

//UART COMMANDS
#define CMD_BRUTE_STATUS	"BRUTE_STAT"
#define CMD_START		'$'
#define CMD_SEPERATOR 	':'
#define CMD_DATA_SIZE	32
#define CMD_DATA2_SIZE	16
#define CMD_VAL_SIZE 	16

#define PM3_RX_CMD 0X51

static volatile uint32_t cmd_val[CMD_VAL_SIZE] = { 0 };	//hold the parse command values

volatile uint8_t clone_status = 1; //global flag to enable disable auto cloning auto enable
uint8_t charToHexDigit_test(uint8_t c);
uint32_t hex_to_ascii(uint8_t hex);

//$!CLONE_HID,2006878900?$	//receive from android $!CLONE,2006878900?$
//$!BRUTE_HID,2006e2309f,2,1001?$   //2 or 3 for second cmd data, 3 for changing facility, 2 for changing card, 4 delay time.
//$!BRUTE_STP?$   //1 or 2 for second cmd data,1 for changing card, 2 for changing facility.
//$!SIM_HID,2006e2309f,1001?$  //ID,DELAY
//$!SIM_STP?$
//$!CLONE_EM4100,3B0052BE68?$  //lf em 410x_sim 3B0052BE68
//$!SIM_EM4100,3B0052BE68,1001?$

void test_strtok() {
	char str[] = "this, is the string - I want to parse";
	char delim[] = " ,-";
	char* token;
	for (token = strtok(str, delim); token; token = strtok(NULL, delim)) {
		//printf("token=%s\n", token);
		__asm("NOP");
	}
}

void bt_cmd_process() {
	uint8_t temp;
	uint8_t * pbuf = 0, *ptoken = 0;

	//exmaple buf: "BRUTE_HID,2006e2309f,2,1000?$"  //removed $! in isr
	pbuf = get_rx_buf();
	ptoken = strtok(pbuf, ",?$");

	if (!strcmp(ptoken, BT_CMD_BRUTE_HID)) {
		ptoken = strtok(NULL, ",");	//get next token
		hid_asciitohex(ptoken);
		ptoken = strtok(NULL, ",?");  //get the brute type command
		cmd_val[0] = atohex(*(ptoken));
		ptoken = strtok(NULL, ",?");  //get the delay time
		cmd_val[1] = atoi(ptoken);	//save the converted value
		pm3_format_packet(PM3_CMD_BRUTE);
		pm3_process();
		bos_state = BOS_BRUTE;
	} else if (!strcmp(ptoken, BT_CMD_BOS_STOP)) {	//
		pm3_format_packet(PM3_CMD_PM3_STOP);
		pm3_process();
		//pm3_send_cmd(PM3_CMD_BRUTE_STOP);
		clearTxBuffer();
		bos_state = BOS_IDLE;
	} else if (!strcmp(ptoken, BT_CMD_CLONE)) {	//clone hid card
		ptoken = strtok(NULL, ",");		//get next token
		hid_asciitohex(ptoken);	//10 hex ascii values
		if (bos_state == BOS_IDLE) {
			pm3_format_packet(PM3_CMD_CLONE_HID_ID);
			pm3_process();	//send the clone command
		}
		printRecivedCardApp(APP_CARD_CLONE_STRING);
	}

	else if (!strcmp(ptoken, BT_CMD_SIM_EM4100)) {//reveived from app hex ascii data
		ptoken = strtok(NULL, ",");		//get next token
		hid_asciitohex(ptoken);		//10 hex ascii values, or byte values stored
		ptoken = strtok(NULL, ",?");  //get the delay time
		cmd_val[0] = atoi(ptoken);	//save the converted value
		pm3_load_sim_em4100_packet();
		pm3_process();	//send the clone command

	} else if (!strcmp(ptoken, BT_CMD_CLONE_EM4100)) {//reveived from app hex ascii data
		ptoken = strtok(NULL, ",");		//get next token
		hid_asciitohex(ptoken);		//10 hex ascii values, or byte values stored
		pm3_load_clone_em4100_packet();
		pm3_process();	//send the clone command
	}

	else if (!strcmp(BT_CMD_CLONE_DIS, ptoken)) {
		clone_status = 0;
		print_clone_status(APP_CLONE_STATUS_STRING);
	} else if (!strcmp(BT_CMD_CLONE_EN, ptoken)) {
		clone_status = 1;
		print_clone_status(APP_CLONE_STATUS_STRING);
	} else if (!strcmp(BT_CMD_SYNC, ptoken)) {
		clone_status = 1;
		print_clone_status(APP_CLONE_STATUS_STRING);
	} else if (!strcmp(CMD_BRUTE_STATUS, ptoken)) {
		//bos_state = BOS_BRUTE;
		//return status to the app.
		//get the status of brute process
	} else if (!strcmp(BT_CMD_HID_SIM, ptoken)) {
		ptoken = strtok(NULL, ",");	//get next token
		hid_asciitohex(ptoken);	//convert id value to hex
		ptoken = strtok(NULL, ",?");	//get delay
		cmd_val[0] = atoi(ptoken);	//save the converted value
		pm3_load_hid_sim_packet();
		pm3_process();	//send command to pm3
		bos_state = BOS_SIM;
	} else if (!strcmp(BT_CMD_SIM_STOP, ptoken)) {
		spi_cmd.txBuf[0] = PM3_CMD_PM3_STOP;
		spi_cmd.f_tx = 1;
		pm3_process();	//send command to pm3
		bos_state = BOS_IDLE;
	}

//uart_putString("$!STATUS,MCU ACK?$\r");	//MUST SEND \R IN ORDER FOR THE BT TO SEND THE DATA.

}

//format and write the pm3 spi tx buffer packet
void pm3_format_packet(uint8_t clone_type) {
	uint8_t ret;
	uint32_t * pcmd2;
	uint32_t cmd2;

	switch (clone_type) {
	case PM3_CMD_CLONE_HID_ID:
		pm3_load_hid_packet(clone_type);
		spi_cmd.f_tx = 1;
		break;
	case PM3_CMD_CLONE_EM4100:
		pm3_load_em4100_packet(clone_type);
		spi_cmd.f_tx = 1;
		break;
	case PM3_CMD_BRUTE:
		pm3_load_brute_packet(PM3_CMD_BRUTE);//load the hid card value same as clone, but with differet command
		spi_cmd.f_tx = 1;
		;
		break;
	case PM3_CMD_PM3_STOP:
		spi_cmd.txBuf[0] = clone_type;
		spi_cmd.f_tx = 1;
		break;
	default:
		break;
	}
}

//comes from teh app, not the reader,  in 5byte values not 10 hex ascii values
void pm3_load_sim_em4100_packet() {
	uint8_t i = 0; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;
	uint8_t temp[2] = { 0 };
	uint8_t temp2 = 0;

	pcard_id = get_card_id5();	//hid.bytes stored previous to this call.
	spi_cmd.txBuf[i++] = PM3_CMD_EM4100_SIM;
	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			spi_cmd.txBuf[i] = *pcard_id++;
		} else
			spi_cmd.txBuf[i] = 0;
	}
	spi_cmd.txBuf[6] = cmd_val[0] >> 8;  //load high byte of delay
	spi_cmd.txBuf[7] = cmd_val[0] & 0xff; //load low byte

}

void pm3_load_clone_em4100_packet() {
	uint8_t i = 0; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;
	uint8_t temp[2] = { 0 };
	uint8_t temp2 = 0;

	pcard_id = get_card_id5();
	spi_cmd.txBuf[i++] = PM3_CMD_CLONE_EM4100;
	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			spi_cmd.txBuf[i] = *pcard_id++;
		} else
			spi_cmd.txBuf[i] = 0;
	}
}

void pm3_load_hid_sim_packet() {
	uint8_t i = 0; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;

	pcard_id = get_card_id5();	//
	spi_cmd.txBuf[i++] = PM3_CMD_HID_SIM;

	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			spi_cmd.txBuf[i] = *pcard_id++;
		} else
			spi_cmd.txBuf[i] = 0;
	}
	spi_cmd.txBuf[6] = cmd_val[0] >> 8;  //load high byte of delay
	spi_cmd.txBuf[7] = cmd_val[0] & 0xff; //load low byte
}

void pm3_load_hid_packet() {
	uint8_t i = 0; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;

	pcard_id = get_card_id5();
	spi_cmd.txBuf[i++] = PM3_CMD_CLONE_HID_ID;

	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			spi_cmd.txBuf[i] = *pcard_id++;
		}

		else
			spi_cmd.txBuf[i] = 0;
	}
}

void pm3_load_brute_packet() {
	uint8_t i = 0; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;

	pcard_id = get_card_id5();
	spi_cmd.txBuf[i++] = PM3_CMD_BRUTE;

	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			spi_cmd.txBuf[i] = *pcard_id++;
		} else
			spi_cmd.txBuf[i] = 0;
	}
	spi_cmd.txBuf[6] = cmd_val[0];	//load the brute type value
	spi_cmd.txBuf[7] = cmd_val[1] >> 8;  //load high byte of delay
	spi_cmd.txBuf[8] = cmd_val[1] & 0xff; //load low byte

}

void pm3_load_em4100_packet() {
	uint8_t i = 0, temp[3] = { 0 }, temp2; //array stores hex data in high bytes in lows array values.
	uint8_t * pcard_id;

	pcard_id = &reader_rx;

	spi_cmd.txBuf[i++] = PM3_CMD_CLONE_EM4100;
	for (i = 1; i < TRANSFER_SIZE; i++) {
		if (i < 6) {
			temp[0] = *pcard_id++;	//split full value into bytes
			temp[1] = *pcard_id++;
			temp2 = (int) strtol(&temp, 0, 16);
			spi_cmd.txBuf[i] = temp2;
		}

		else
			spi_cmd.txBuf[i] = 0;
	}
}

//loads the hid card id into spi tx buffer
//0 = CMD, 1 = CARD ID FOR HID/EM4100,  6 =
void init_send_clone_packet(uint8_t * pcard_id, uint8_t clone_type) {
	uint8_t i, temp[3] = { 0 }, temp2;//array stores hex data in high bytes in lows array values.
	for (i = 0; i < TRANSFER_SIZE; i++) {
		if (i == 0)
			spi_cmd.txBuf[i] = clone_type;
		else if (i < 6) {
			if (clone_type == PM3_CMD_CLONE_HID_ID
					|| clone_type == PM3_CMD_BRUTE) {
				spi_cmd.txBuf[i] = *pcard_id++;
			}
			if (clone_type == PM3_CMD_CLONE_EM4100) {
				temp[0] = *pcard_id++;	//split full value into bytes
				temp[1] = *pcard_id++;
				temp2 = (int) strtol(&temp, 0, 16);
				spi_cmd.txBuf[i] = temp2;
			}
		} else
			spi_cmd.txBuf[i] = 0;
	}	//for
	asm("NOP");
}

//ascii to hex routine
uint16_t atohex(uint8_t c) {

	if (c >= 'A')
		return (c - 'A' + 10) & 0x0f;	//if caps hex
	else
		return (c - '0') & 0x0f;
}

//http://stackoverflow.com/questions/6933039/convert-two-ascii-bytes-in-one-hexadecimal-byte
unsigned char stringToByte(char c[2]) {
	return charToHexDigit(c[0]) * 16 + charToHexDigit(c[1]);
}

void clear_cmd_val() {
	uint8_t i;
	for (i = 0; i < CMD_VAL_SIZE; i++) {
		cmd_val[i] = 0;
	}
}



