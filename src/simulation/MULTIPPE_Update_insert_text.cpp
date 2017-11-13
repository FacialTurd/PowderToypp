#include "simulation/Elements.h"
#include "simulation/MULTIPPE_Update.h"
#include <iostream>
#include "simplugin.h"

#define MY_MAX_BLOCK_SIZE (1<<18)
#define ALIGNSIZE 2

uint8_t * mytxt_buffer1 = NULL;
int mytxt_buffer1_offset = 0;

void MULTIPPE_Update::InsertText(Simulation *sim, int i, int x, int y, int ix, int iy)
{
#ifndef OGLI
	if (!mytxt_buffer1)
	{
		mytxt_buffer1 = (uint8_t*)malloc(MY_MAX_BLOCK_SIZE);
		if (!mytxt_buffer1)
			return;
		mytxt_buffer1_offset = 0;
	}
	/* 0x00: block size (2 byte)
	 * 0x02: start x (2 byte)
	 * 0x04: start y (2 byte)
	 * 0x06: null-terminated ASCII string
	 */
	int begin_block = mytxt_buffer1_offset;
	int block_cptr = begin_block + 6;
	int r;
	while (block_cptr < MY_MAX_BLOCK_SIZE)
	{
		x += ix; y += iy;
		if (!sim->InBounds(x, y)) break;
		r = sim->pmap[y][x];
		if ((r&0xFF) != ELEM_MULTIPP || sim->parts[r>>8].life != 10) break;
		
		mytxt_buffer1[block_cptr++] = sim->parts[r>>8].ctype;
	}
	if (block_cptr >= MY_MAX_BLOCK_SIZE) return;

	mytxt_buffer1[block_cptr] = 0; // add null
	block_cptr = (block_cptr & (-ALIGNSIZE)) + ALIGNSIZE; // align
	
	mytxt_buffer1_offset = block_cptr;

	int totalsize = block_cptr - begin_block;
	int position = sim->parts[i].ctype;
	int16_t * begin_block_ptr = (int16_t*)&mytxt_buffer1[begin_block];

	begin_block_ptr[0] = totalsize;
	begin_block_ptr[1] = position & 0xFFFF;
	begin_block_ptr[2] = (position >> 16) & 0xFFFF;
#endif
}

#undef MY_MAX_BLOCK_SIZE
#undef ALIGNSIZE
