/* Notes
 * Known issue.  The RX buffer is always 0 in first spi read.  Tried to eliminate by reading 2 time, etc.  still not working correctly
 * - fix: must offset the rx buffer by +1 in order to read correct data.
 */

#include "boscloner.h"
#include "util.h"
#include "proxmark3.h"
#include "at91sam7s512.h"
#include "boscloner_utils.h"
#include <stdint.h>
#include "boscloner_utils.h"

static int _em410x_simTraceLen = 0;
static uint32_t pack_i = 0;
static uint32_t pack_val = 0;
static uint8_t * pBigBuf;


//ascii to hex routine
uint8_t bos_atohex(uint8_t c) {

	if (c >= 'A')
		return (c - 'A' + 10) & 0x0f;	//if caps hex
	else
		return (c - '0') & 0x0f;
}

uint8_t em410x_uida[11] = "3B0052BE68";
uint8_t em410x_uid[5] = { 0x3B, 0x00, 0x52, 0xbe, 0x68 };

void bos_em4100_sim_init() {
	uint8_t i = 0;
	BigBuf_Clear();
	pBigBuf = BigBuf_get_addr();
	//!ascii  ConstructEM410xEmul(em410x_uida, 64);
	ConstructEM410xEmul(&em410x_uid, 64);
}

// clear out our _em410x_sim window
int Clear_em410x_sim(int redraw) {
	int gtl = _em410x_simTraceLen;
	//memset(BigBuf, 0x00, _em410x_simTraceLen);
	BigBuf_Clear();
	_em410x_simTraceLen = 0;
}

//-----------------------------------------------------------------------------
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// This function only - based upon original by iZsh
//-----------------------------------------------------------------------------
/* write a manchester bit to the _em410x_sim */
void Append_em410x_sim(int redraw, int clock, int bit) {
	int i;

	//set first half the clock bit (all 1's or 0's for a 0 or 1 bit)
	for (i = 0; i < (int) (clock / 2); ++i) {
		*pBigBuf++ = bit;
	}
	//set second half of the clock bit (all 0's or 1's for a 0 or 1 bit)
	for (i = (int) (clock / 2); i < clock; ++i) {
		//BigBuf[_em410x_simTraceLen++] = bit ^ 1;
		*pBigBuf++ = bit ^ 1;
	}
}


//-----------------------------------------------------------------------------
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// This function only - based upon original by iZsh
//-----------------------------------------------------------------------------
//pass in binary byte array value.
void ConstructEM410xEmul(uint8_t *puid, const uint8_t clock) {
	int i, j, binary[4], parity[4];
	uint32_t n;
	uint8_t hex_i = 0;
	uint8_t temp_hex = 0;
	/* clear our _em410x_sim */
	//Clear_em410x_sim(0);
	uint8_t * ptemp = puid;
	/*
	for (i = 0; i < 5; i++) {
		Dbprintf("bos.id5: %x", bos.id5[i]);
	}
	*/

	/* write 9 start bits */
	for (i = 0; i < 9; i++)
		Append_em410x_sim(0, clock, 1);

	uint8_t temp[2] = { 0 };

	/* for each hex char */
	hex_i = 0;
	parity[0] = parity[1] = parity[2] = parity[3] = 0;
	for (i = 0; i < 10; i++) {
		/* read each hex char */
		if (hex_i > 0) {	//if one then the high nibble was already used
			n = (temp_hex) & 0x0f;
			//Dbprintf("hexi: %u %x %x", hex_i, n, temp_hex);
			hex_i = 0;
			puid++;	//go to next byte
		} else {
			temp_hex = bos.id5[(i / 2)];
			n = (temp_hex >> 4) & 0x0f;	//use low nibble
			//Dbprintf("hexi: %u %x %x", hex_i, n, temp_hex);
			hex_i++;
		}

		for (j = 3; j >= 0; j--, n /= 2)
			binary[j] = n % 2;

		/* append each bit */
		Append_em410x_sim(0, clock, binary[0]);
		Append_em410x_sim(0, clock, binary[1]);
		Append_em410x_sim(0, clock, binary[2]);
		Append_em410x_sim(0, clock, binary[3]);

		/* append parity bit */
		Append_em410x_sim(0, clock,
				binary[0] ^ binary[1] ^ binary[2] ^ binary[3]);

		/* keep track of column parity */
		parity[0] ^= binary[0];
		parity[1] ^= binary[1];
		parity[2] ^= binary[2];
		parity[3] ^= binary[3];
	}

	/* parity columns */
	Append_em410x_sim(0, clock, parity[0]);
	Append_em410x_sim(0, clock, parity[1]);
	Append_em410x_sim(0, clock, parity[2]);
	Append_em410x_sim(0, clock, parity[3]);

	/* stop bit */
	Append_em410x_sim(1, clock, 0);
}

// HID Prox TAG ID: 0x2803156a22 (46353) - Format Len: 35bit - FC: 24 - Card: 701713
//#db# HID Prox TAG ID: 0x2006e2309f (6223) - Format Len: 26bit - FC: 113 - Card: 6223 //(26,6223,113,&hi,&lo);

//-----------------------------------------------------------------------------
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// This function only - based upon original by iZsh
//-----------------------------------------------------------------------------
void ConstructEM410xEmul_working(const char *uid, const uint8_t clock) {

	int i, j, binary[4], parity[4];
	uint32_t n;
	/* clear our _em410x_sim */
	Clear_em410x_sim(0);

	/* write 9 start bits */
	for (i = 0; i < 9; i++)
		Append_em410x_sim(0, clock, 1);

	uint8_t temp[2] = { 0 };

	/* for each hex char */
	parity[0] = parity[1] = parity[2] = parity[3] = 0;
	for (i = 0; i < 10; i++) {
		/* read each hex char */
		//sscanf(&uid[i], "%1x", &n);	//converts ascii to integers 1 hex value at a time
		//temp[0] = uid[i];
		n = bos_atohex(uid[i]);
		for (j = 3; j >= 0; j--, n /= 2)
			binary[j] = n % 2;

		/* append each bit */
		Append_em410x_sim(0, clock, binary[0]);
		Append_em410x_sim(0, clock, binary[1]);
		Append_em410x_sim(0, clock, binary[2]);
		Append_em410x_sim(0, clock, binary[3]);

		/* append parity bit */
		Append_em410x_sim(0, clock,
				binary[0] ^ binary[1] ^ binary[2] ^ binary[3]);

		/* keep track of column parity */
		parity[0] ^= binary[0];
		parity[1] ^= binary[1];
		parity[2] ^= binary[2];
		parity[3] ^= binary[3];
	}

	/* parity columns */
	Append_em410x_sim(0, clock, parity[0]);
	Append_em410x_sim(0, clock, parity[1]);
	Append_em410x_sim(0, clock, parity[2]);
	Append_em410x_sim(0, clock, parity[3]);

	/* stop bit */
	Append_em410x_sim(1, clock, 0);
}

// HID Prox TAG ID: 0x2803156a22 (46353) - Format Len: 35bit - FC: 24 - Card: 701713
//#db# HID Prox TAG ID: 0x2006e2309f (6223) - Format Len: 26bit - FC: 113 - Card: 6223 //(26,6223,113,&hi,&lo);

void bos_del_ms(uint32_t del) {
	uint32_t i = 0, j = 0;

	for (i = 0; i < del; i++) {
		for (j = 0; j < 5000; j++) {
			WDT_HIT();
		}
	}
}

void test_format() {
	uint32_t hi, lo;
	uint64_t original26, original35;

	original26 = 0x2006e2309f;
	original35 = 0x2803156a22;

	bos_encode26(6223, 113, &hi, &lo);
	bos_encode35(701713, 24, &hi, &lo);
}

void bos_encode_hid26(uint32_t cardnum, uint32_t fc, uint32_t * hi,
		uint32_t * lo) {

	uint32_t hi_temp1 = 0;
	uint32_t lo_temp1 = 0;
	uint8_t count = 0;
	uint8_t ep = 0;
	uint8_t op = 0;

	lo_temp1 = (fc << 17) | (cardnum << 1);  //add card number

	//calculate odd parity = bit0 = for bit(1 base) location 2-13. 0 based = bits 1-12.
	for (uint8_t i = 0; i < 12; i++) {
		if ((lo_temp1 >> (i + 1)) & 0x01 == 1) { //1 bit check
			count++;
		}
	}
	//if (count & 1) { //odd number of bits//! on even number the odd parity is 1 for some reason
	//if(count%2 == 0){ //even parity count
	if (!(count % 2)) {
		op = 1;
	}
	//calculate even parity = bit26 (1 base) = location 14-25. = 12 bits to check
	count = 0;
	for (uint8_t i = 0; i < 12; i++) {
		if ((lo_temp1 >> (i + 13)) & 0x01 == 1) { //1 bit check
			count++;
		}
	}
	//if(count%2 == 0){ //even parity count  //! if odd number of bits then 1
	//if (count %2 == 1){ //works
	if (count % 2) {
		ep = 1;
	}

	//SET PARITY BITS
	lo_temp1 |= (ep << 25) | (op); //set the even parity and odd parity bits.

	//SET STATIC BITSs = set bit 27,38
	lo_temp1 |= (1 << 26);

	hi_temp1 = (1 << 5); //set bit 37 of final val

	*hi = hi_temp1;
	*lo = lo_temp1;

}

//http://www.proxmark.org/forum/viewtopic.php?id=1767
void bos_encode_hid35(uint32_t cardnum, uint32_t fc, uint32_t * hi,
		uint32_t * lo) {

	uint32_t hi_temp = 0;
	uint32_t lo_temp = 0;
	uint8_t count = 0;
	uint32_t i = 0;

	lo_temp = (fc << 21) | (cardnum << 1);
	hi_temp = 0x28 | ((fc >> 11) & 1);

	//calculate parity bits based upon required bits using bit masks
	//proper order of parity checks,  34,1,35 (1 based).
	//See http://www.proxmark.org/forum/viewtopic.php?id=1767 for proper bit selection for check.

	// Select only bit used for parity bit 34 in low number (10110110110110110110110110110110)
	uint32_t parity_bit_34_low = lo_temp & 0xB6DB6DB6;

	// Calculate number of ones in low number
	for (i = 1; i != 0; i <<= 1) { //shift the 1 bit through 32 bit number and or with  parity mask.
		if (parity_bit_34_low & i)
			count++;
	}
	// Calculate number of ones in high number
	if (hi_temp & 1) {
		count++;
	}
	// Set parity bit (Even parity)
	if (count % 2) {
		hi_temp |= 0x2;
	}

	// Calculating and setting parity bit 1
	// Select only bit used for parity bit 1 in low number (01101101101101101101101101101100)
	uint32_t parity_bit_1_low = lo_temp & 0x6DB6DB6C;
	count = 0;

	// Calculate number of ones in low number
	for (i = 1; i != 0; i <<= 1) {
		if (parity_bit_1_low & i)
			count++;
	}
	// Calculate number of ones in high number
	if (hi_temp & 0x1)
		count++;

	if (hi_temp & 0x2)
		count++;

	// Set parity bit (Odd parity)
	if (!(count % 2))
		lo_temp = lo_temp | 0x1;

	// Calculating and setting parity bit 35
	count = 0;
	// Calculate number of ones in low number (all bit of low, bitmask unnecessary)
	for (i = 1; i != 0; i <<= 1) {
		if (lo_temp & i)
			count++;
	}
	// Calculate number of ones in high number
	if (hi_temp & 0x1)
		count++;

	if (hi_temp & 0x2)
		count++;

	// Set parity bit (Odd parity)
	if (!(count % 2))
		hi_temp = hi_temp | 0x4;

	*hi = hi_temp;
	*lo = lo_temp;

}
