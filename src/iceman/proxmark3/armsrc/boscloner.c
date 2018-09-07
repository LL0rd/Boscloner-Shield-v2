/* Notes
 * Known issue.  The RX buffer is always 0 in first spi read.  Tried to eliminate by reading 2 time, etc.  still not working correctly
 * PM3 will crash if outputting data over USB and not connected to computer.
 */

#include "boscloner.h"
#include "util.h"
#include "proxmark3.h"
#include "at91sam7s512.h"
#include "boscloner_utils.h"
#include <stdint.h>
#include "boscloner_utils.h"

#include "apps.h"


void bos_load_id();
void bos_print_buffer();
void bos_print_txbuffer();
uint32_t bos_proc_cmds();
void bos_send_ack();
void bos_send_rxbuf();
void bos_update_id();
void boscloner_setupspi(void);
void clear_id_vals();
void clear_rxBuff(void);
void bos_process();

struct bos_ bos = { .clone_run = 0, .process = idle, .flag_send_data = 0,
		.bosclone_type = 0, .flag_brute_stop = 0 };

//#define DEBUG_OUTPUT_SPI 1

#define ENABLE_CLONE 1

#define SPI_TIMEOUT_COUNT 	1000
#define BOS_BRUTE_CYCLES_VAL 20000
#define BOS_BRUTE_CYCLES_DELAY 1000

#define BOS_EM410X_SIM_CYCLES 200000

//!MUST MATCH THE ENUM USED IN SHIELD FIRMWARE!
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

#define PM3_CLONE_OK  		10
#define PM3_CLONE_ERROR  	11

#define BOS_CMD_VAR_LOCATION 7
#define BOS_CMD_DEL_LOCATION 8

#define BOS_TRANSFER_SIZE 16
#define BOS_ID_OFFSET 2

uint32_t bos_delay = 0;  //cmd delay value recieved
uint32_t copy_count = 0;
uint8_t copy_run = 0;

bos_state_t bos_state = IDLE;

uint32_t cmd_vals[8] = { 0 };		//save recived cmd data into this array.
uint8_t txBuf[BOS_TRANSFER_SIZE] = { 0 };
uint8_t rxBuf[BOS_TRANSFER_SIZE] = { 0 };

uint8_t bos_clone_status = 0;	//1 when finished correctly
static uint32_t bos_hi2 = 0, bos_hi = 0, bos_lo = 0; //for 5 byte card (10 hex vals) l0 = bytes low = 3 bytes, hi = high 2 byte.
uint32_t _hi = 0, _lo = 0;

//#define BOS_PRINT_DEBUG

uint32_t get_id_hi() {
	return bos_hi;
}

uint32_t get_id_lo() {
	return bos_lo;
}

//boscloner periodic tasks
void bos_periodic_tasks(void) {

	WDT_HIT();

	pm3_load_tx_data();  //load current status of pm3
	send_clone_packet();


}

uint8_t test = 0;
void bos_process() {

	// - START BOSCLONER CODE
	switch (bos.process) {
	case hid_clone:
		_hi = get_id_hi();
		_lo = get_id_lo();
		Dbprintf("bosclone id: %x lo: %x", _hi, _lo);
		CopyHIDtoT55x7(0, _hi, _lo, 0);
		bos.clone_run = 1;	//set to 1 if only want to run once (debug).
		bos.process = idle;	//clear flag to run again
		break;
	case em4100_clone:
		_hi = get_id_hi();
		_lo = get_id_lo();
		Dbprintf("clone em4100 hi: %x lo: %x", _hi, _lo);
		WriteEM410x(1, _hi, _lo);	//card type = 1.
		bos.process = idle;	//clear flag to run again
		break;
	case hid_brute_f:		//blocking
		bos_brute_hid(1, bos_delay);	//1 = change facility
		bos.process = idle;	//clear flag to run again
		break;
	case hid_brute_c:		//blocking
		bos_brute_hid(2, bos_delay);	//use the flag id as paramter
		bos.process = idle;	//clear flag to run again
		break;
	case hid_sim:	//blocking
		_hi = get_id_hi();
		_lo = get_id_lo();
		Dbprintf("hid sim id: %x lo: %x del: %u", _hi, _lo, bos_delay);
		bos_hid_sim(_hi, _lo, bos_delay);
		bos.process = idle;	//clear flag to run again
		break;
	case em4100_sim:
		Dbprintf("em410x sim id:0x%X%X%X%X%X, del: %u", bos.id5[0], bos.id5[1],
				bos.id5[2], bos.id5[3], bos.id5[4], bos_delay);
		bos_em410x_sim();
		bos.process = idle;	//clear flag to run again
		break;
	default:
		break;

	}	//case
}

void pm3_load_tx_data() {
	uint8_t i = 0;

	bos.flag_send_data = 1;
	txBuf[0] = PM3_CMD_STATUS;
	txBuf[1] = bos.process;
	//for(i=0; i<BOS_TRANSFER_SIZ ; i++){
	//}

}

void bos_print_txbuffer() {
	uint8_t i = 0;

	for (i = 0; i < 16; i++) {
		Dbprintf("rxbuf[%u]: %x", i, txBuf[i]);
		//Dbprintf("loopi:i %u , %x",i,i);
	}
}

//get bos shield cmd/data and return ack if valid
int8_t send_clone_packet() {
	uint8_t i = 0;
	uint8_t count = 0;

	WDT_HIT();
	boscloner_setupspi();
	clear_rxBuff();

	//RECEIVE THE PACKET OF DATA
	for (i = 0; i < BOS_TRANSFER_SIZE; i++) {
		while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY) == 0) {	//TDRE
			count++;	// wait for the transfer to complete
			if (count >= SPI_TIMEOUT_COUNT) {
				Dbprintf("SPI TIMEOUT - SPI_TXEMPTY!");
				return 1;
			}
		}
		//AT91C_BASE_SPI->SPI_TDR = i;	//load data to tx  DUMMY DATA
		//load data to send
		if (bos.flag_send_data) {
			AT91C_BASE_SPI->SPI_TDR = txBuf[i];
		} else {
			AT91C_BASE_SPI->SPI_TDR = 0xff;
		}
		count = 0;
		while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX) == 0) {//ENDTX wait for the transfer to complete
			count++;
			if (count >= SPI_TIMEOUT_COUNT) {
				//Dbprintf("SPI TIMEOUT - spi endtx!");
				return 2;
			}
		}
		while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDRX) == 0)
			; //CAN DO A CHECK TO SEE IF THE RX DATA IS VALID = CHECK THE RXBUFF OR ENDRX
		rxBuf[i] = AT91C_BASE_SPI->SPI_RDR;	//read the data recieved
		if (i == 0) // the first byte read from buffer is junk data.  needs to second byte after this things are synced = didn't help
			//rxBuf[i] = AT91C_BASE_SPI->SPI_RDR;	//read the data recieved
			WDT_HIT();
	}			//FOR
	SpinDelayUs(100);	//let slave prepare for next packet.
	bos.flag_send_data = 0;
	//process commands.
	bos_proc_cmds();

	WDT_HIT();
	return (0);	//correct exit

}

//process bos commands
uint32_t bos_proc_cmds() {
	uint32_t i = 0;
	uint8_t temp1 = 0, temp2 = 0;

#ifdef DEBUG_OUTPUT_SPI
	bos_print_buffer();
#endif
	if (rxBuf[1] == PM3_CMD_PM3_STOP) {
		bos.flag_brute_stop = 1;
		bos.flag_stop = 1;
	}

	bos.bosclone_type = rxBuf[1];
	if (bos.bosclone_type != 0) {
		//Dbprintf("cmd: %x", bos.bosclone_type);
		if (bos.bosclone_type == PM3_CMD_CLONE_HID_ID) {
			bos_load_id();	//loads id from rx buf, updates globals
			bos.process = hid_clone;
			bos_state = CLONE;
		} else if (bos.bosclone_type == PM3_CMD_CLONE_EM4100) {
			bos_load_id();	//loads id from rx buf, updates globals
			bos_state = CLONE;
			bos.process = em4100_clone;
		} else if (bos.bosclone_type == PM3_CMD_BRUTE) {	//BOSBRUTE COMMAND
			bos_state = BRUTE;
			bos_load_id();	//loads id from rx buf, updates globals
			if (rxBuf[BOS_CMD_VAR_LOCATION] == 1) {
				bos.process = hid_brute_f;
			} else if (rxBuf[BOS_CMD_VAR_LOCATION] == 2) {
				bos.process = hid_brute_c;
			}
			temp1 = rxBuf[BOS_CMD_DEL_LOCATION];	//high byte
			temp2 = rxBuf[BOS_CMD_DEL_LOCATION + 1];	//low byte
			bos_delay = temp1 << 8 | temp2;
			if (bos_delay <= 0) {	//not valid load default
				bos_delay = BOS_BRUTE_CYCLES_DELAY;
			}
		} else if (bos.bosclone_type == PM3_CMD_HID_SIM) {
			bos_load_id();	//loads id from rx buf, updates globals
			//Dbprintf("hid sim");
			temp1 = rxBuf[7];	//high byte
			temp2 = rxBuf[7 + 1];	//low byte
			cmd_vals[0] = temp1 << 8 | temp2;
			if (cmd_vals[0] <= 0) {	//not valid load default
				cmd_vals[0] = BOS_BRUTE_CYCLES_DELAY;
			}
			bos_delay = cmd_vals[0];
			bos.process = hid_sim;
		} else if (bos.bosclone_type == PM3_CMD_EM4100_SIM) {
			bos_load_id();	//loads id from rx buf, updates globals
			//Dbprintf("hid sim");
			temp1 = rxBuf[7];	//high byte
			temp2 = rxBuf[7 + 1];	//low byte
			cmd_vals[0] = temp1 << 8 | temp2;
			if (cmd_vals[0] <= 0) {	//not valid load default
				cmd_vals[0] = BOS_BRUTE_CYCLES_DELAY;
			}
			bos_delay = cmd_vals[0];
			bos.process = em4100_sim;
		}
	}

	return (0);
}

//load id from the rx buf
void bos_load_id() {
	uint8_t i = 0;

	clear_id_vals();
	for (i = 0; i < (BOS_TRANSFER_SIZE - BOS_ID_OFFSET); i++) { //16-2
		bos.id5[i] = rxBuf[i + BOS_ID_OFFSET]; //!bos.id5 is only length 5,  throwing compile error due to optimiations,  will change to 16 which is max loop value.
	}
	bos_update_id();

}

//write new hi lo values based from newly rx byte data
void bos_update_id() {
	uint8_t i = 0;
	//NOW PACKEAGE HI,LO VALUES FOR CLONING THE CARD
	//hi=0x20  lo=0x06e2309f
	bos_hi = 0;
	bos_lo = 0;
	for (i = 0; i < CLONE_ID_LEN; i++) {
		if (i == 0) {
			bos_hi = bos.id5[i];
		} else if (i == 1) {
			bos_lo |= bos.id5[i] << 24;
		} else if (i == 2) {
			bos_lo |= bos.id5[i] << 16;
		} else if (i == 3) {
			bos_lo |= bos.id5[i] << 8;
		} else if (i == 4) {
			bos_lo |= bos.id5[i];
		}
	}		//for

}

void bos_hid_sim(uint32_t fhi, uint32_t flo, uint32_t delay_ms) {

	pm3_load_tx_data();
	bos.flag_send_data = 1;

	while (1) {
		LED(LED_RED, 50);
		CmdHIDsimTAGEx(fhi, flo, 0, BOS_BRUTE_CYCLES_VAL);
		pm3_load_tx_data();
		send_clone_packet();	//check for new data
		//SpinDelay(delay_ms); //was not working properly
		bos_del_ms(delay_ms);

		if (BUTTON_PRESS() == 1)
			break;
		else if (bos.flag_stop == 1) {
			bos.flag_stop = 0;
			break;
		}
	} //while
	DbpString("Done...");

}

void bos_em410x_sim() {
	FpgaDownloadAndGo(FPGA_BITSTREAM_LF);
	bos_em4100_sim_init(); //write the bigbuf data

	LED_A_ON();
	while (1) {
		SimulateTagLowFrequencyEx(4096, 0, 0, BOS_EM410X_SIM_CYCLES); //leds must be off or will mess up timing
		bos_periodic_tasks();
		if (BUTTON_PRESS() == 1)
			break;
		else if (bos.flag_stop == 1) {
			bos.flag_stop = 0;
			break;
		}

	}
	Dbprintf("Done...");
	bos.flag_stop = 0;
	FpgaWriteConfWord(FPGA_MAJOR_MODE_OFF);
	LED_A_OFF();

}

void bos_brute_hid(uint8_t mode, uint32_t delay_ms) { //flag = mode  3,4,5 = 1,2,3 from mcu
	uint32_t hi = 0, lo = 0, hi2 = 0, lo2 = 0, loop_hi = 0, loop_lo = 0;
	uint32_t test;
	uint8_t fmtLen = 0;
	uint32_t fc = 0;
	uint32_t cardnum = 0;
	uint64_t hi_lo = { 0 };
	uint8_t hi_lo_array[10] = { 0 };
	uint32_t i = 0, j = 0;
	uint32_t max_val = 0;
	uint8_t up_done = 0, down_done = 0;
	uint32_t updated_val = 0;
	uint8_t fbreak = 0;

	Dbprintf("bosBrute hid mode: %u, del(ms): %u", mode, delay_ms);
	hi = bos_hi;
	lo = bos_lo;
	Dbprintf("hi: 0x%x lo: 0x%08x", hi, lo);
	//figure out card format, start with 26 bit.

	if (hi2 != 0) { //extra large HID tags
		//PrintAndLogEx(NORMAL, "HID Prox TAG ID: %x%08x%08x (%u)", hi2, hi, lo, (lo>>1) & 0xFFFF);   //!what is the (%u)  looks like card num, is on some cards.
	} else {  //standard HID tags <38 bits

		if (((hi >> 5) & 1) == 1) { //if bit 38 is set then < 37 bit format is used  //!less than 37 bit used.
			uint32_t lo2 = 0;
			lo2 = (((hi & 31) << 12) | (lo >> 20)); //get bits 21-37 to check for format len bit
			uint8_t idx3 = 1;
			while (lo2 > 1) { //find last bit set to 1 (format len bit)
				lo2 >>= 1;		//!lo2 = lo2>>1;
				idx3++;			//!increment idx3
			}

			fmtLen = idx3 + 19; //!idx3 = 16
			fc = 0;
			cardnum = 0;

			if (fmtLen == 26) {
				cardnum = (lo >> 1) & 0xFFFF;
				fc = (lo >> 17) & 0xFF;
			}
			if (fmtLen == 34) {
				cardnum = (lo >> 1) & 0xFFFF;
				fc = ((hi & 1) << 15) | (lo >> 17);
			}
			if (fmtLen == 35) {

				cardnum = (lo >> 1) & 0xFFFFF;
				fc = ((hi & 1) << 11) | (lo >> 21);

			}
		} //<37bit check
		else { //if bit 38 is not set then 37 bit format is used
			fmtLen = 37;
			fc = 0;
			cardnum = 0;
			if (fmtLen == 37) {
				cardnum = (lo >> 1) & 0x7FFFF;
				fc = ((hi & 0xF) << 12) | (lo >> 20);
			}
		}
		//! this is the only printout for all formats.
		Dbprintf(
				"HID Prox TAG ID: %x%08x (%u) - Format Len: %ubit - FC: %u - Card: %u",
				hi, lo, (lo >> 1) & 0xFFFF, fmtLen, fc, cardnum);
	}

	//BRUTE LOOP
	switch (mode) {
	case 1: //card increment mode, facility fixed
		pm3_load_tx_data();
		bos.flag_send_data = 1;

		if (fmtLen == 26) {
			max_val = 65536;	//2^16 possiblities for card num
		} else if (fmtLen == 35) {
			max_val = 1048580;	//2^20 possiblities for card num
		}
		i = 0;
		while (up_done == 0 || down_done == 0) {
			if (BUTTON_PRESS() == 1) {	//break on button press
				DbpString("[-] told to stop");
				break;
			} else if (bos.flag_brute_stop == 1) {
				bos.flag_brute_stop = 0;
				DbpString("brute stop cmd rx");
				fbreak = 1;
				break;
			}
			i++;  //CARDNUM +- I values

			//INCREMENT CARD VALUE *************************
			updated_val = cardnum + i;
			if (updated_val > max_val) {
				up_done = 1;
			}

			//encode the current values
			if (!up_done) {
				LED(LED_RED, 50);
				if (fmtLen == 26) {
					bos_encode_hid26(updated_val, fc, &loop_hi, &loop_lo);
				} else if (fmtLen == 35) {
					bos_encode_hid35(updated_val, fc, &loop_hi, &loop_lo);
				} else {
					Dbprintf("Card format: %u not supported", fmtLen);
					break;
				}
				Dbprintf("trying Facility = %u card %u", fc, updated_val);
				Dbprintf("updated TAG ID: 0x%x%08x", loop_hi, loop_lo);
				CmdHIDsimTAGEx(loop_hi, loop_lo, 0, BOS_BRUTE_CYCLES_VAL);
				if (!down_done) {
					bos_del_ms(delay_ms / 2);
				} else {
					bos_del_ms(delay_ms);
				};
			}

			//DECREMENT  VALUE  **************************
			updated_val = cardnum - i;
			if (updated_val <= 0) {
				down_done = 1;
			}

			//encode the current values
			if (!down_done) {
				LED(LED_RED, 50);
				if (fmtLen == 26) {
					bos_encode_hid26(updated_val, fc, &loop_hi, &loop_lo);
				} else if (fmtLen == 35) {
					bos_encode_hid35(updated_val, fc, &loop_hi, &loop_lo);
				} else {
					Dbprintf("Card format: %u not supported", fmtLen);
					break;
				}
				Dbprintf("trying Facility = %u card %u", fc, updated_val);
				Dbprintf("updated TAG ID: 0x%x%08x", loop_hi, loop_lo);
				CmdHIDsimTAGEx(loop_hi, loop_lo, 0, BOS_BRUTE_CYCLES_VAL);
				if (!up_done) {
					bos_del_ms(delay_ms / 2);
				} else {
					bos_del_ms(delay_ms);
				}
			}

			pm3_load_tx_data();
			send_clone_packet();	//check for new data
		}	//while

		Dbprintf("BOSBRUTE Done!");
		break;
	case 2:	//facility code (fc) increment mode
		if (fmtLen == 26) {
			max_val = 256;	//2^8 possiblities forfc
		} else if (fmtLen == 35) {
			max_val = 4096;	//2^12 possiblities for fc
		}
		i = 0;
		while (up_done == 0 || down_done == 0) {
			if (BUTTON_PRESS()) {	//break on button press
				DbpString("[-] told to stop");
				break;
			} else if (bos.flag_brute_stop == 1) {
				bos.flag_brute_stop = 0;
				DbpString("brute stop cmd rx");
				break;
			}

			i++;  //fc +- I values
			//INCREMENT fc VALUE *************************
			updated_val = fc + i;
			if (updated_val > max_val) {
				up_done = 1;
			}

			//encode the current values
			if (!up_done) {
				LED(LED_RED, 50);
				if (fmtLen == 26) {
					bos_encode_hid26(cardnum, updated_val, &loop_hi, &loop_lo);
				} else if (fmtLen == 35) {
					bos_encode_hid35(cardnum, updated_val, &loop_hi, &loop_lo);
				} else {
					Dbprintf("Card format: %u not supported", fmtLen);
					break;
				}
				Dbprintf("trying Facility = %u card %u", updated_val, cardnum);
				Dbprintf("updated TAG ID: 0x%x%08x", loop_hi, loop_lo);
				CmdHIDsimTAGEx(loop_hi, loop_lo, 0, BOS_BRUTE_CYCLES_VAL);
				if (!down_done) {
					bos_del_ms(delay_ms / 2);
				} else {
					bos_del_ms(delay_ms);
				}
			}

			//DECREMENT  VALUE  **************************
			updated_val = fc - i;
			if (updated_val <= 0) {
				down_done = 1;
			}

			//encode the current values
			if (!down_done) {
				LED(LED_RED, 50);
				if (fmtLen == 26) {
					bos_encode_hid26(cardnum, updated_val, &loop_hi, &loop_lo);
				} else if (fmtLen == 35) {
					bos_encode_hid35(cardnum, updated_val, &loop_hi, &loop_lo);
				} else {
					Dbprintf("Card format: %u not supported", fmtLen);
					break;
				}
				Dbprintf("trying Facility = %u card %u", updated_val, cardnum);
				Dbprintf("updated TAG ID: 0x%x%08x", loop_hi, loop_lo);
				CmdHIDsimTAGEx(loop_hi, loop_lo, 0, BOS_BRUTE_CYCLES_VAL);
				if (!up_done) {
					bos_del_ms(delay_ms / 2);
				} else {
					bos_del_ms(delay_ms);
				}
			}

			//Dbprintf("loop: %u", i);
			pm3_load_tx_data();
			send_clone_packet();	//check for new data
		}	//while

		Dbprintf("BOSBRUTE Done", loop_hi, loop_lo);
		break;

	default:
		Dbprintf("mode not recognized....");
		break;
	}  //switch

}

void bos_print_buffer() {
	uint8_t i = 0;

	for (i = 0; i < 16; i++) {
		Dbprintf("rxbuf[%u]: %x", i, rxBuf[i]);
		//Dbprintf("loopi:i %u , %x",i,i);
	}
}

void bos_send_ack() {
	uint8_t i = 0;

	//boscloner_setupspi();
	for (i = 0; i < BOS_TRANSFER_SIZE; i++) {
		while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY) == 0)
			;	// wait for the transfer to complete
		if (i == 0) {
			AT91C_BASE_SPI->SPI_TDR = PM3_ACK;	//send the ack byte
		} else if (i > 0) {
			AT91C_BASE_SPI->SPI_TDR = rxBuf[i + 1];	//offset by 1 error + skip the cmd value = the recieved data values, +1 effectively is +2 here
		}
	}	//for
}	//function

//this will show the issue of index0=0, then ok data.
void bos_send_rxbuf() {
	uint8_t i = 0;

//boscloner_setupspi();
	for (i = 0; i < BOS_TRANSFER_SIZE; i++) {
		while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY) == 0)
			;	// wait for the transfer to complete
		AT91C_BASE_SPI->SPI_TDR = rxBuf[i];	//send the rx packet back
	}	//for
}	//function

void clear_id_vals() {
	uint8_t i = 0;

	bos_hi2 = 0;
	bos_hi = 0;
	bos_lo = 0;
	for (i = 0; i < CLONE_ID_LEN; i++) {
		bos.id5[i] = 0;
	}

}

void clear_rxBuff(void) {
	uint8_t i;
	for (i = 0; i < BOS_TRANSFER_SIZE; i++) {
		rxBuf[i] = 0;
	}

}	//function

/*
 void bos_blink(int count) {
 int i = 0;
 for (i = 0; i < count; i++) {
 LED_B_ON();
 SpinDelay(100);
 LED_B_OFF();
 SpinDelay(100);
 }
 }
 */

/*
 void boscloner_send_spi() {

 uint8_t i, temp = 1;
 uint8_t array_match = 0;

 temp = temp + 1;
 boscloner_setupspi();

 for (i = 0; i < BOS_TRANSFER_SIZE; i++) {

 while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY) == 0)
 // wait for the transfer to complete
 ;
 AT91C_BASE_SPI->SPI_TDR =;	//tx data register
 while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF) == 0)
 ;	//read register - wait for data
 rxBuf[i] = AT91C_BASE_SPI->SPI_RDR;

 LED_C_INV(); //toggle ledc
 }

 //FAILS
 //test the recieve array for match of sent
 for (i = 0; i < BOS_TRANSFER_SIZE; i++) {
 if (rxBuf[i] == i) {
 array_match = 1;
 } else {
 array_match = 0;
 break;
 }
 }

 if (array_match == 1) {
 //bos_blink(2);
 LED_B_ON();
 } else {
 //bos_blink(3);
 LED_B_OFF();
 }

 }
 */

void boscloner_setupspi(void) {

// PA10 -> SPI_NCS2 chip select (LCD)
// PA11 -> SPI_NCS0 chip select (FPGA)
// PA12 -> SPI_MISO Master-In Slave-Out
// PA13 -> SPI_MOSI Master-Out Slave-In
// PA14 -> SPI_SPCK Serial Clock

// Disable PIO control of the following pins, allows use by the SPI peripheral
	AT91C_BASE_PIOA->PIO_PDR = GPIO_NCS0 | GPIO_NCS2 | GPIO_MISO | GPIO_MOSI
			| GPIO_SPCK;

	AT91C_BASE_PIOA->PIO_ASR = GPIO_NCS0 | GPIO_MISO | GPIO_MOSI | GPIO_SPCK;

	AT91C_BASE_PIOA->PIO_BSR = GPIO_NCS2;

//enable the SPI Peripheral clock
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI);
// Enable SPI
	AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIEN;

	AT91C_BASE_SPI->SPI_MR = (0 << 24) | // Delay between chip selects (take default: 6 MCK periods)
			(11 << 16) | // Peripheral Chip Select (selects LCD SPI_NCS2 or PA10)
			(0 << 7) |	// Local Loopback Disabled
			(1 << 4) |	// Mode Fault Detection disabled
			(0 << 2) |	// Chip selects connected directly to peripheral
			(0 << 1) |	// Fixed Peripheral Select
			(1 << 0);		// Master Mode
	AT91C_BASE_SPI->SPI_CSR[2] = (1 << 24) |// Delay between Consecutive Transfers (32 MCK periods)
			(1 << 16) |	// Delay Before SPCK (1 MCK period)
			//(6 << 8) |	// Serial Clock Baud Rate (baudrate = MCK/6 = 24Mhz/6 = 4M baud = 8 bits - 500khz = 24Mhz/48
			(49 << 8) |// Serial Clock Baud Rate (baudrate = MCK/6 = 24Mhz/6 = 4M baud = 8 bits - 500khz = (24Mhz/48)+1
			//(16 << 8) |	// Serial Clock Baud Rate (baudrate = MCK/6 = 24Mhz/6 = 4M baud = 8 bits - 500khz = 24Mhz/48
			//( 1 << 4)	|	// Bits per Transfer (9 bits)
			(0 << 4) |  // 8 BITS PER TRANSFER (THE FIELD TO 0)
			(0 << 3) |	// Chip Select inactive after transfer
			(1 << 1) |	// Clock Phase data captured on leading edge, changes on following edge
			(0 << 0);		// Clock Polarity inactive state is logic 0

}
