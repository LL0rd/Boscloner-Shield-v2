/*
 * uart_bm.h
 *
 *  Created on: Dec 21, 2015
 *      Author: MJ
 */

#ifndef SOURCES_BOS_COMMANDS_H_
#define SOURCES_BOS_COMMANDS_H_

extern volatile uint8_t clone_status;

extern volatile uint8_t rx_done;
extern volatile uint8_t cmd_data[];

void uart_putc(uint8_t c);
int8_t bt_uart_putString(uint8_t *ptr);
//!wrapper to allow printf to output to the lcd without modifying the function structure.
void bt_putc_uart_printf(void* p, char c);
uint32_t get_card_lo_app();
uint32_t get_card_hi_app();
void send_app_card_clone();
void send_app_card_scan();
void bt_cmd_process();
void init_UART();
void reset_cmd_state();
uint32_t * get_cmddata2();


//cmds to send to pm3
typedef enum {
	PM3_CMD_CLONE_HID_ID = 1,
	PM3_CMD_CLONE_EM4100,
	PM3_CMD_HID_SIM,
	PM3_CMD_EM4100_SIM,
	PM3_CMD_BRUTE,
	PM3_CMD_PM3_STOP,	//stop pm3 process
	PM3_CMD_STATUS,
	PM3_ACK
} pm3_cmds_;


//processes on the pm3
typedef enum {
	idle = 1,
	hid_clone,
	em4100_clone,
	hid_brute_f,
	hid_brute_c,
	hid_sim,
	em4100_sim
} pm3_process_;
pm3_process_ pm3_state;



#define BOS_PACKET_SIZE 16
#define CLONE_ID_LEN 10
/*
#define PM3_CMD_CLONE_HID_ID 		(0Xa1)		//will use for HID command flag
#define PM3_CMD_CLONE_EM4100 		(0Xa2)
#define PM3_CMD_HID_SIM				(0XA3)
#define PM3_CMD_EM4100_SIM			(0XA4)
#define PM3_CMD_BRUTE 				(0Xa5)
#define PM3_CMD_BRUTE_STOP			(0XA6)
#define PM3_CMD_SIM_STOP			(0XA7)

#define PM3_ACK 					(0XB1)
*/
#endif /* SOURCES_BOS_COMMANDS_H_ */
