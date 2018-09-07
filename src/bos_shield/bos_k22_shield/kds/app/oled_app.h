/*
 * oled_app.h
 *
 *  Created on: Dec 15, 2015
 *      Author: MJ
 */

#ifndef SOURCES_OLED_APP_H_
#define SOURCES_OLED_APP_H_

void print_received_card_em4100();
void printRecivedCardApp();
void print_clone_status(uint8_t * pArray);
void printRecivedCardWiegand();


void init_disp_update();

void oled_test();
void digitalWrite(uint8_t pin, uint8_t dir);
void pinMode(uint8_t pin, uint8_t dir);


#endif /* SOURCES_OLED_APP_H_ */
