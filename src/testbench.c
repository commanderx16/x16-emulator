// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "cpu/cpu.h"
#include "glue.h"
#include "testbench.h"

int hex_to_int8(char* str);
int hex_to_int16(char* str);
bool hex_validate(char* str);
void invalid();
void ready();

size_t get_testline(char **lineptr, size_t *n, FILE *stream)
{
    char *cptr;
    size_t maxlen;
    size_t slen;
    int c;

    if(*lineptr == NULL) {
        *lineptr = malloc(256);
        *n = 256;
    }

    cptr = *lineptr;
    maxlen = *n;
    slen = 0;

    do {
        c = fgetc(stream);

        if (c == EOF) {
            break;
        }

        *cptr = c;
        ++cptr;
        ++slen;

        if (slen >= maxlen) {
            maxlen = maxlen << 1;
            *lineptr = realloc(*lineptr, maxlen);
            cptr = *lineptr + slen;
        }
    } while(c != '\n');

    *cptr = '\0';
    *n = maxlen;
    return slen;
}

void testbench_init()
{
    char* line = NULL;

    char addr[5] = "0000";
    char addr2[5] = "0000";
    char val[3] = "00";

    int iaddr, iaddr2, ival;
    size_t slen = 0;

    bool init_done = false;

    ready();

    while (!init_done) {
        size_t len = get_testline(&line, &slen, stdin);     //Read command from stdin

        if (strncmp(line, "RAM", 3) == 0) {                 //Set RAM bank
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    memory_set_ram_bank((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "ROM", 3) == 0) {            //Set ROM bank
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    memory_set_rom_bank((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "STM", 3) == 0) {            //Set memory address value
            if (len < 12) {
                invalid();
            } else {
                strncpy(addr, line + 4, 4);
                strncpy(val, line + 9, 2);

                iaddr = hex_to_int16(addr);
                ival = hex_to_int8(val);

                if (iaddr == -1 || ival == -1) {
                    invalid();
                } else {
                    write6502((uint16_t)iaddr, (uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "FLM", 3) == 0) {            //Fill memory address range with value
            if (len < 17) {
                invalid();
            } else {
                strncpy(addr, line + 4, 4);
                strncpy(addr2, line + 9, 4);
                strncpy(val, line + 14, 2);

                iaddr = hex_to_int16(addr);
                iaddr2 = hex_to_int16(addr2);
                ival = hex_to_int8(val);

                if (iaddr == -1 || iaddr2 == -1 || ival == -1) {
                    invalid();
                } else {
                    for (; iaddr <= iaddr2; iaddr++) {
                        write6502((uint16_t)iaddr, (uint8_t)ival);
                    }
                    ready();
                }
            }
        }

        else if (strncmp(line, "STA", 3) == 0) {            //Set accumulator value
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    cpu_set_a((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "STX", 3) == 0) {            //Set X register value
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    cpu_set_x((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "STY", 3) == 0) {            //Set Y register value
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    cpu_set_y((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "SST", 3) == 0) {            //Set status register value
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    cpu_set_status((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "SSP", 3) == 0) {            //Set stack pointer value
            if (len < 7) {
                invalid();
            } else {
                strncpy(val, line + 4, 2);
                ival = hex_to_int8(val);
                if (ival == -1) {
                    invalid();
                } else {
                    cpu_set_sp((uint8_t)ival);
                    ready();
                }
            }
        }

        else if (strncmp(line, "RUN", 3) == 0) {            //Run code at address
            if (len < 9) {
                invalid();
            } else {
                strncpy(addr, line + 4, 4);

                iaddr = hex_to_int16(addr);
                if (iaddr == -1) {
                    invalid();
                } else {
                    uint8_t sp = cpu_get_sp();
                    write6502(0x0100+sp, (0xfffd-1)>>8);
                    sp--;
                    write6502(0x0100+sp, (0xfffd-1) & 255);
                    sp--;
                    cpu_set_sp(sp);
                    cpu_set_pc((uint16_t)iaddr);
                
                    init_done=true;
                }
            }
        }

        else if(strncmp(line, "RQM", 3) == 0) {             //Request memory address value
            if (len < 9) {
                invalid();
            } else {
                strncpy(addr, line + 4, 4);
                iaddr = hex_to_int16(addr);
                if (iaddr == -1) {
                    invalid();
                } else {
                    printf("%lx\n", (long)read6502((uint16_t)iaddr));
                    fflush(stdout);
                }
            }
        }

        else if(strncmp(line, "RQA", 3) == 0) {             //Request accumulator value
            printf("%lx\n", (long)cpu_get_a());
            fflush(stdout);
        }

        else if(strncmp(line, "RQX", 3) == 0) {             //Request X register value
            printf("%lx\n", (long)cpu_get_x());
            fflush(stdout);
        }

        else if(strncmp(line, "RQY", 3) == 0) {             //Request Y register value
            printf("%lx\n", (long)cpu_get_y());
            fflush(stdout);
        }

        else if(strncmp(line, "RST", 3) == 0) {             //Request status register value
            printf("%lx\n", (long)cpu_get_status());
            fflush(stdout);
        }

        else if(strncmp(line, "RSP", 3) == 0) {             //Request stack pointer value
            printf("%lx\n", (long)cpu_get_sp());
            fflush(stdout);
        }

        else {
            printf("ERR Unknown command\n");
            fflush(stdout);
        }
    }
    free(line);
}

int hex_to_int8(char* str)
{
    int val;

    if (strlen(str) != 2 || !hex_validate(str)) {
        return -1;
    } else {
        val = (int)strtol(str, NULL, 16);
        if (val < 0 || val > 0xff) return -1; else return val;
    }
}

int hex_to_int16(char* str)
{
    int val;

    if (strlen(str) != 4 || !hex_validate(str)) {
        return -1;
    } else {
        val = (int)strtol(str, NULL, 16);
        if (val < 0 || val > 0xffff) return -1; else return val;
    }
}

bool hex_validate(char* str)
{
    int i=0;
    char c;

    c = str[i];
    while (c != 0) {
        if ( (c>= '0' && c<= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            i++;
        } else {
            return false;
        }
        c = str[i];
    }
    return true;
}

void invalid()
{
    printf("ERR Invalid command\n");
    fflush(stdout);
}

void ready()
{
    printf("RDY\n");
    fflush(stdout);
}