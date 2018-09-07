/*
 * hid.c
 *
 *  Created on: Aug 24, 2018
 *      Author: MJ
 */

#include "bos_app.h"


struct hid_data_ hid;

uint8_t * get_card_id5() {
	return &hid.bytes;
}

uint32_t get_card_hi_app() {
	return (hid.hi);  //hi
}
uint32_t get_card_lo_app() {
	return (hid.lo);
}


void clear_hex_vals() {
	uint8_t i;
	for (i = 0; i < CARD_ID_VALS_SIZE; i++) {
		hid.hex[i] = 0;
	}
}


//convert from ascii array to hex binary data byte array
void hid_asciitohex(uint8_t * pArray) {
	uint8_t i;
	uint8_t size = 0, size2, temp1, temp2, temp3;

	size = strlen(pArray);
	size = 10;  //10 hex values

	for (int i = 0; i < size; i++) {
		temp1 = *pArray++;	//ascii data
		temp2 = atohex(temp1);
		hid.hex[i] = temp2;
	}

	//we have the single 10 hex values - now pack, to 5 bytes - MSB first
	for (i = 0; i < size; i++) {
		if (i == 0) {
			hid.bytes[0] = (hid.hex[0] << 4) | (hid.hex[1]);
		} else if (i == 2) {
			hid.bytes[1] = (hid.hex[2] << 4) | (hid.hex[3]);
		} else if (i == 4) {
			hid.bytes[2] = (hid.hex[4] << 4) | (hid.hex[5]);
		} else if (i == 6) {
			hid.bytes[3] = (hid.hex[6] << 4) | (hid.hex[7]);
		} else if (i == 8) {
			hid.bytes[4] = (hid.hex[8] << 4) | (hid.hex[9]);
		}
		//card_hex |= (uint64_t) temp2 << (4 * i); //shift each hex up 4 bits -- //for some reason 9<<7 converting to 0xffffffff90000000  must cast 64 bit!!!

	}

	//can store in hi lo format as well.
	hid.hi = hid.bytes[0];
	hid.lo = hid.bytes[1] << 24 | hid.bytes[2] << 16 | hid.bytes[3] << 8 | hid.bytes[4];

}
