/*
 * boscloner_utils.h
 *
 *  Created on: Aug 14, 2018
 *      Author: MJ
 */

#ifndef APP_BOS_UTILS_H_
#define APP_BOS_UTILS_H_

void bos_encode_hid26(uint32_t cardnum, uint32_t fc, uint32_t * hi, uint32_t * lo);
void bos_encode_hid35(uint32_t cardnum, uint32_t fc, uint32_t * hi, uint32_t * lo);

#endif /* APP_BOS_UTILS_H_ */
