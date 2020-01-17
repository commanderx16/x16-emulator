//
//  uartqueue.h
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 16/01/2020.
//

#ifndef uartqueue_h
#define uartqueue_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

int get_outgoing_queue_length(void);
int get_incoming_queue_length(void);
uint8_t get_outgoing_value(void);
bool insert_outgoing_value(uint8_t item);
uint8_t get_incoming_value(void);
bool insert_incoming_value(uint8_t item);

#endif /* uartqueue_h */
