#ifndef __BOSCLONER__UTILS_H
#define __BOSCLONER__UTILS_H
#include "stdint.h"


void bos_em4100_sim_init();
void bos_encode_hid26(uint32_t cardnum, uint32_t fc, uint32_t * hi, uint32_t * lo);
void bos_encode_hid35(uint32_t cardnum, uint32_t fc, uint32_t * hi, uint32_t * lo);
void bos_del_ms(uint32_t del);
uint8_t bos_atohex(uint8_t c);
void ConstructEM410xEmulGraph(const char *uid, const uint8_t clock);
void AppendGraph(int redraw, int clock, int bit);
int ClearGraph(int redraw);
#endif
