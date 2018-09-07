/*
 * bos_io.c
 *
 *  Created on: Feb 12, 2016
 *      Author: MJ
 */
#include "bos_io.h"

#define PB_DEBOUNCE_DEL	50
//#define PB_COUNT_DEB_DEL 10000
#define PB_COUNT_DEL	30000
#define PB_COUNT_TEST	6000000  //

//gpio for boscloner board - init
void init_bosio() {

	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;

	//PB1
	GPIOC_PDDR |= 0 << PB1_LOC;  //input=0 output=1
	PORTC_PCR7 |= PORT_PCR_MUX(1); //mux 1 = gpio, drive strength on
	//LED0
	GPIOC_PDDR |= 1 << LED0_LOC;  //input=0 output=1
	PORTC_PCR8 |= PORT_PCR_MUX(1) | PORT_PCR_DSE_MASK; //mux 1 = gpio, drive strength on
	//LED0
	GPIOC_PDDR |= 1 << LED1_LOC;  //input=0 output=1
	PORTC_PCR9 |= PORT_PCR_MUX(1) | PORT_PCR_DSE_MASK; //mux 1 = gpio, drive strength on

}

//debounce for pushbutton,  check if going into MFG test mode
uint8_t pb1_check2() {
	uint64_t count = 0;
	if ((!PB1_READ)) {
		while (1) { //loop while switch is pushed
			count++;

			/*
			 if ((PB1_READ)){
			 __asm("NOP");
			 }
			 */

			if ((PB1_READ && count > PB_DEBOUNCE_DEL)
					&& (count < PB_COUNT_TEST)) {	//wait until goes high again
				return 1;
			} else if (PB1_READ && count > PB_COUNT_TEST) {
				return 2;
			}

		}//while
	}//if

	return 0;
}

//debounce for pushbutton,  return1 if switch pushed.
uint8_t pb1_check() {
	if (!PB1_READ) {
		DelayMS(PB_DEBOUNCE_DEL);
		if (!PB1_READ) {
			while (!PB1_READ)
				;	//wait until released.
			return 1;
		}
	}
	return 0;
}
