#pragma once
// extern char sdl_ignore_quit;
#include <stdint.h>
#include "PowderToy.h"

// void DelayOperation1(Simulation * sim, int ms);
// extern int sdl_my_extra_args[4];
extern uint8_t * mytxt_buffer1;
extern int mytxt_buffer1_parts;

#define ARG0_NO_DLL_PLUGIN		0x1
#define ARG0_NO_QUIT_SHORTCUT	0x2