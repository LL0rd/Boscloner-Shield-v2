#ifndef __BOSCLONER_H
#define __BOSCLONER_H
#include "stdint.h"

#define BOS_LOOP_CNT_VAL 200000  //aprox 1000ms
//#define BOS_LOOP_CNT_VAL 20000  //aprox 100ms
//#define BOS_LOOP_CNT_VAL 500  //aprox
//#define BOS_LOOP_CNT_VAL 50
//aprox

//extern uint8_t bos_clone_id[];
extern uint8_t bos_clone_status;
extern uint32_t bos_delay;

typedef enum {
	IDLE, CLONE, BRUTE, BRUTE_ERROR,
} bos_state_t;
extern bos_state_t bos_state;

//processes on the pm3

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




//#define CLONE_ID_LEN 	5		//10 BYTES OF CLONE DATA
#define CLONE_ID_LEN 	5		//!to avoid compile error
struct bos_ {
	uint8_t clone_run;
	pm3_process_ process;		//keeps track of current pm3 process
	uint8_t flag_send_data;
	uint8_t bosclone_type;
	uint8_t flag_brute_stop;
	uint8_t flag_stop;
	uint8_t id5[CLONE_ID_LEN];
//5 bytes = 10 hex values
};
extern struct bos_ bos;

extern uint32_t copy_count;
extern uint8_t copy_run;

void bos_brute_hid(uint8_t mode, uint32_t delay_ms);
void bos_em410x_sim();
void bos_hid_sim(uint32_t fhi, uint32_t flo, uint32_t delay_ms);
void bos_periodic_tasks(void);
uint32_t get_id_hi();
uint32_t get_id_lo();
void pm3_load_tx_data();
int8_t send_clone_packet();
void bos_process();

#endif
