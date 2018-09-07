/*
 * hid.h
 *
 *  Created on: Aug 24, 2018
 *      Author: MJ
 *      //storage and routines for hid data
 */

#ifndef APP_HID_H_
#define APP_HID_H_


#include "bos_app.h"


#define CARD_ID_VALS_SIZE 10  //number of hex values




//use for em4100 as well
struct hid_data_ {
 uint8_t 	hex[CARD_ID_VALS_SIZE];
 uint8_t 	bytes[5];
 uint32_t 	hi2;
 uint32_t 	hi;
 uint32_t 	lo;
};
extern struct hid_data_ hid;


uint8_t * get_card_id5();






#endif /* APP_HID_H_ */
