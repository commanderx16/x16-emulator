//
//  socketclient.h
//  CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD//

#ifndef socketclient_h
#define socketclient_h

#include <stdio.h>
#include <stdint.h>

void socket_connect(void);
size_t socket_write(uint8_t in_value);
uint8_t socket_read(void);

#endif /* socketclient_h */
