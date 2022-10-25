// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

/*
TESTBENCH
---------

1. General

The testbench option is intended to be used for unit testing where the test runner is
made in a high level language. The test runner starts the emulator with the
-testbench option. During the test, the test runner communicates with the emulator
over stdin/stdout.


2. Messages from the emulator to the test runner (stdout)

RDY                 Sent when the emulator is ready to receive commands from the test runner
<hex value>         Emulator response to get requests
ERR <message>       Invalid arguments
STP                 Sent when STP instruction is executed


3. Messages from the test runner to the emulator (stdin)

RAM                 Set RAM bank
ROM                 Set ROM bank
STM nnnn nn         Set value of memory address nnnn to nn
FLM nnnn nnnn nn    Fill memory address range nnnn to nnnn with value nn
STA nn              Set accumulator value to nn
STX nn              Set X register value to nn
STY nn              Set Y register value to nn
SST nn              Set status register value to nn
SSP nn              Set stack pointer value to nn
RUN nnnn            Execute code at address nnnn, when done emulator respond with RDY
RQM nnnn            Request value of memory address nnnn
RQA                 Request value of accumulator
RQX                 Request value of X register
RQY                 Request value of Y register
RST                 Request value of status register
RSP                 Request value of stack pointer


4. Message formatting

Commands sent to the emulator must be formatted precisely as follows:

- One command per line written to stdin
- The first three characters is the command in upper case
- 16 bit values must be written with exactly four digits in hex format
- 8 bit values must be written with exactly two digits in hex format
- There is exactly one character separating the command field and each value field; it doesn't matter what character is used as separator

*/


#ifndef TEST_H
#define TEST_H

void testbench_init();

#endif