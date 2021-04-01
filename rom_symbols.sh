#!/bin/sh
for i in ndx keyd fa vartab fnlen fnadr status sa; do
	echo "#define" `echo $i | tr '[:lower:]' '[:upper:]'` 0x`cat ../ChickenLips16-rom/build/ChickenLips16/kernal.sym ../ChickenLips16-rom/build/ChickenLips16/basic.sym | grep -w $i | head -n 1 | cut -d " " -f 2`;
done > rom_symbols.h
