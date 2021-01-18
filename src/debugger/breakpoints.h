#ifndef _BREAKPOINTS_H
#define _BREAKPOINTS_H

typedef enum {
	BPT_PC= 0x10,
	BPT_MEM= 0x03,
	BPT_RMEM= 0x01,
	BPT_WMEM= 0x02
} TBreakpointType;

typedef struct {
	TBreakpointType type;
	int addr;
	int length;
} TBreakpoint;

#endif
