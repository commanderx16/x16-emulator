#!/bin/bash
set -e

# build ROM
(cd ../x16-rom/; make clean all)

# copy ROM labels & LST
cp ../x16-rom/build/x16/rom_labels.h src
cp ../x16-rom/build/x16/rom_lst.h src

# always force re-build of main.c because of updated rom_labels.h
rm -f main.o

# build emulator with `-trace` support
TRACE=1 make
