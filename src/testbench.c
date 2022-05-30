#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "cpu/fake6502.h"
#include "glue.h"
#include "testbench.h"

int hex_to_int8(char* str);
int hex_to_int16(char* str);
bool hex_validate(char* str);
void invalid();

size_t getline(char **lineptr, size_t *n, FILE *stream)
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

        if(c == EOF) {
            break;
        }

        *cptr = c;
        ++cptr;
        ++slen;

        if(slen >= maxlen) {
            maxlen = maxlen << 1;
            *lineptr = realloc(*lineptr, maxlen);
            cptr = *lineptr + slen;
        }
    } while(c != '\n');

    *cptr = '\0';
    ++cptr;
    ++slen;

    *n = maxlen;
    return slen;
}

void testbench_init(){
    char* line = NULL;

    char addr[5] = "0000";
    char addr2[5] = "0000";
    char val[3] = "00";

    int iaddr, iaddr2, ival;
    size_t len = 0;

    bool init_done=false;

    //Send READY message
    printf("RDY\n");
    fflush(stdout);

    while(!init_done){
        getline(&line, &len, stdin);                        //Read command from stdin

        if (strncmp(line, "RAM", 3)==0){                    //Set RAM bank
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                memory_set_ram_bank((uint8_t)ival);
            }
        }

        else if (strncmp(line, "ROM", 3)==0){               //Set ROM bank
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                memory_set_rom_bank((uint8_t)ival);
            }
        }

        else if (strncmp(line, "STM", 3)==0){               //Set memory address value
            strncpy(addr, line+4, 4);
            strncpy(val, line+9, 2);

            iaddr = hex_to_int16(addr);
            ival = hex_to_int8(val);

            if (iaddr==-1 || ival==-1){
                invalid();
            }
            else{
                write6502((uint16_t)iaddr, (uint8_t)ival);
            }
        }

        else if (strncmp(line, "FLM", 3)==0){               //Fill memory address range with value
            strncpy(addr, line+4, 4);
            strncpy(addr2, line+9, 4);
            strncpy(val, line+14, 2);

            iaddr = hex_to_int16(addr);
            iaddr2 = hex_to_int16(addr2);
            ival = hex_to_int8(val);

            if (iaddr==-1 || iaddr2==-1 || ival==-1){
                invalid();
            }
            else{
                for (;iaddr<=iaddr2;iaddr++){
                    write6502((uint16_t)iaddr, (uint8_t)ival);
                }
            }
        }

        else if (strncmp(line, "STA", 3)==0){               //Set accumulator value
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                a = (uint8_t)ival;
            }
        }

        else if (strncmp(line, "STX", 3)==0){               //Set X register value
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                x = (uint8_t)ival;
            }
        }

        else if (strncmp(line, "STY", 3)==0){               //Set Y register value
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                y = (uint8_t)ival;
            }
        }

        else if (strncmp(line, "SST", 3)==0){               //Set status register value
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                status = (uint8_t)ival;
            }
        }

        else if (strncmp(line, "SSP", 3)==0){               //Set stack pointer value
            strncpy(val, line+4, 2);
            ival = hex_to_int8(val);
            if (ival==-1){
                invalid();
            }
            else{
                sp = (uint8_t)ival;
            }
        }

        else if (strncmp(line, "RUN", 3)==0){               //Run code at address
            strncpy(addr, line+4, 4);

            iaddr = hex_to_int16(addr);
            if (iaddr==-1){
                invalid();
            }
            else{
                write6502(0x0100+sp, (0xfffd-1)>>8);
                sp--;
                write6502(0x0100+sp, (0xfffd-1) & 255);
                sp--;
                pc = (uint16_t)iaddr;
            
                init_done=true;
            }
        }

        else if(strncmp(line, "RQM", 3)==0){                //Request memory address value
            strncpy(addr, line+4, 4);
            iaddr = hex_to_int16(addr);
            if (iaddr==-1){
                invalid();
            }
            else{
                printf("%lx\n", (long)read6502((uint16_t)iaddr));
                fflush(stdout);
            }
        }

        else if(strncmp(line, "RQA", 3)==0){                //Request accumulator value
            printf("%lx\n", (long)a);
            fflush(stdout);
        }

        else if(strncmp(line, "RQX", 3)==0){                //Request X register value
            printf("%lx\n", (long)x);
            fflush(stdout);
        }

        else if(strncmp(line, "RQY", 3)==0){                //Request Y register value
            printf("%lx\n", (long)y);
            fflush(stdout);
        }

        else if(strncmp(line, "RST", 3)==0){                //Request status register value
            printf("%lx\n", (long)status);
            fflush(stdout);
        }

        else if(strncmp(line, "RSP", 3)==0){                //Request stack pointer value
            printf("%lx\n", (long)sp);
            fflush(stdout);
        }
    }
    free(line);
}

int hex_to_int8(char* str){
    int val;

    if (strlen(str)!=2 || !hex_validate(str)){
        return -1;
    }
    else{
        val = (int)strtol(str, NULL, 16);
        if (val < 0 || val > 0xff) return -1; else return val;
    }
}

int hex_to_int16(char* str){
    int val;

    if (strlen(str)!=4 || !hex_validate(str)){
        return -1;
    }
    else{
        val = (int)strtol(str, NULL, 16);
        if (val < 0 || val > 0xffff) return -1; else return val;
    }
}

bool hex_validate(char* str){
    int i=0;
    char c;

    c = str[i];
    while (c!=0){
        if ( (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F') ){
            i++;
        }
        else{
            return false;
        }
        c = str[i];
    }
    return true;
}

void invalid(){
    printf("ERR Invalid command\n");
    fflush(stdout);
}