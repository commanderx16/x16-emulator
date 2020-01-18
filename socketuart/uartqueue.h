//
//  uartqueue.h
//  CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD//
//

#ifndef uartqueue_h
#define uartqueue_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_ITEMS 16384
int get_outgoing_queue_length(void);
int get_incoming_queue_length(void);
uint8_t get_outgoing_value(void);
bool insert_outgoing_value(uint8_t item);
uint8_t get_incoming_value(void);
bool insert_incoming_value(uint8_t item);

#endif /* uartqueue_h */
