// Commander X16 Emulator
// Copyright (c) 2019, 2020 Michael Steil
// All rights reserved. License: 2-clause BSD

#define VER "37"
#define VER_NAME "Geneva"
#define VER_INFO "### Release 37 (\"Geneva\")\n" \
"* VERA 0.9 register layout [Frank van den Hoef]\n" \
"* audio [Frank van den Hoef]\n" \
"    * VERA PCM and PSG audio support\n" \
"    * YM2151 support is now enabled by default\n" \
"    * added `-abufs` to specify number of audio buffers\n" \
"* removed UART [Frank van den Hoef]\n" \
"* added window icon [Nigel Stewart]\n" \
"* fixed access to paths with non-ASCII characters on Windows [Serentty]\n" \
"* SDL HiDPI hint to fix mouse scaling [Edward Kmett]\n" \
"### Release 36 (\"Berlin\")\n" \
"* added VERA UART emulation (`-uart-in`, `-uart-out`)\n" \
"* correctly emulate missing SD card\n" \
"* moved host filesystem interface from device 1 to device 8, only available if no SD card is attached\n"\
"* require numeric argument for `-test` to auto-run test\n" \
"* fixed JMP (a,x) for 65c02\n" \
"* Fixed ESC as RUN/STOP [Ingo Hinterding]\n\n"
