#include <stdint.h>

uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);
uint8_t via2_read(uint8_t reg);
void via2_write(uint8_t reg, uint8_t value);

uint8_t via2_pb_get_out();
void via2_pb_set_in(uint8_t value);
void via2_sr_set(uint8_t value);
