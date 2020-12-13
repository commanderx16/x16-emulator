#ifndef _BREAKPOINTS_H
#define _BREAKPOINTS_H

typedef enum {
	BPT_PC,
	BPT_MEM,
	BPT_RMEM,
	BPT_WMEM
} TBreakpointType;

typedef struct {
	TBreakpointType type;
	int addr;
	int length;
} TBreakpoint;

#endif
