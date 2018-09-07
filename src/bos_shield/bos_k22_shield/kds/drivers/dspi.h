/*
 * dspi.h
 *
 *  Created on: Jan 22, 2016
 *      Author: MJ
 */

#ifndef APP_DSPI_H_
#define APP_DSPI_H_
#include "stdint.h"

extern void SPI1_IRQHandler(void);

#define TRANSFER_SIZE               (16)                /*! Transfer size */
struct spi_cmd_ {
	uint8_t f_rx;	//flag rx data availabe - don't clear buffer until processed
	uint8_t f_tx;	//flag tx data availabe - don't clear until data transmitted.
	uint8_t rxBuf[TRANSFER_SIZE];
	uint8_t txBuf[TRANSFER_SIZE];
};
extern struct spi_cmd_ spi_cmd;

int8_t test_dspi(void);
uint8_t f_pm3_data = 0;  //recieved pm3 data from spi
//void send_clone_cmd_pm3(uint8_t clone_type);

#endif /* APP_DSPI_H_ */
