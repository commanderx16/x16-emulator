#!/bin/sh
for i in ndx keyd fa vartab fnlen fnadr status sa; do
	echo "#define" `echo $i | tr '[:lower:]' '[:upper:]'` 0x`cat ../x16-rom/build/x16/kernal.sym ../x16-rom/build/x16/basic.sym | grep -w $i | head -n 1 | cut -d " " -f 2`;
done > src/rom_symbols.h
