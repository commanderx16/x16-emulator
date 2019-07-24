#include <stdint.h>

#define NUM_RAM_BANKS 256
#define NUM_ROM_BANKS 8

#define RAM_SIZE (0xa000 + NUM_RAM_BANKS * 8192) /* $0000-$9FFF + banks at $C000-$DFFF */
#define ROM_SIZE (NUM_ROM_BANKS * 8192)          /* banks at $A000-$BFFF */

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[];
extern uint8_t ROM[];
