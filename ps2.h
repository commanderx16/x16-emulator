#include <stdint.h>

#define PS2_DATA_MASK 1
#define PS2_CLK_MASK 2

extern int ps2_clk_out, ps2_data_out;
extern int ps2_clk_in, ps2_data_in;

void kbd_buffer_add(uint8_t code);
uint8_t kbd_buffer_remove();
void ps2_step();
