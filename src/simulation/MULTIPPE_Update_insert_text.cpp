#include "simulation/Elements.h"
#include "simulation/MULTIPPE_Update.h"
#include <iostream>
#include <zlib.h>
#include "simplugin.h"

#define MAIN_BLOCK_SIZE (3<<17)
#define TOTAL_BLOCK_SIZE (MAIN_BLOCK_SIZE + 16)
#define MAIN_ZIP_BLOCK_SIZE ((XRES + YRES) * 4)
#define TOTAL_ZIP_BLOCK_SIZE (MAIN_ZIP_BLOCK_SIZE + 16)
#define ALIGNSIZE 2

uint8_t * mytxt_buffer1 = NULL;
uint8_t * my_deflate_buffer = NULL;
int mytxt_buffer1_parts = 0;

void MULTIPPE_Update::InsertText(Simulation *sim, int i, int x, int y, int ix, int iy)
{
#ifndef OGLI
	static z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	uint8_t *buffer_ptr1, *buffer_ptr2;
	static uint8_t *mytxt_buffer1_curr, *mytxt_buffer1_end;

	if (!mytxt_buffer1)
	{
		mytxt_buffer1_curr = mytxt_buffer1 = (uint8_t*)malloc(TOTAL_BLOCK_SIZE);
		if (!mytxt_buffer1)
			return;
		mytxt_buffer1_end = mytxt_buffer1 + MAIN_BLOCK_SIZE;
		mytxt_buffer1_parts = 0;
	}

newseg_0:
	int r, v, f = sim->parts[i].tmp2 & 1, rept = 1;

	if (f)
	{
		r = inflateInit2(&strm, -10);
		if (r != Z_OK) return;
		if (!my_deflate_buffer)
		{
			my_deflate_buffer = (uint8_t*)malloc(TOTAL_ZIP_BLOCK_SIZE);
			if (my_deflate_buffer == NULL) return;
		}

		buffer_ptr1 = my_deflate_buffer;
		buffer_ptr2 = (uint8_t*)(my_deflate_buffer + MAIN_ZIP_BLOCK_SIZE);
		/* compressedData */			strm.next_in	= (z_const Bytef *)(buffer_ptr1);
		/* compressedBufferSize */		strm.avail_in	= MAIN_ZIP_BLOCK_SIZE;
		/* uncompressedData */			strm.next_out	= (Bytef*)(mytxt_buffer1_curr + 6);
		/* uncompressedBufferSize */	strm.avail_out	= (uInt)(mytxt_buffer1_end - strm.next_out);
		if ((int)strm.avail_out < 0) return;
	}
	else
	{
		buffer_ptr1 = (uint8_t*)(mytxt_buffer1_curr + 6);
		buffer_ptr2 = mytxt_buffer1_end;
	}
	/* 0x00: block size (2 byte)
	 * 0x02: start x (2 byte)
	 * 0x04: start y (2 byte)
	 * 0x06: null-terminated ASCII string
	 */
	while (buffer_ptr1 < buffer_ptr2)
	{
		x += ix; y += iy;
		if (!sim->InBounds(x, y)) break;
		r = sim->pmap[y][x];
		if (TYP(r) != ELEM_MULTIPP) break;
		int ri = part_ID(r);
		if (sim->parts[ri].life == 10)
		{
			v = sim->parts[ri].ctype;
			intptr_t fin;
			if (f & 2)
				fin = (intptr_t)(buffer_ptr1 + 4*rept);
			else
				fin = (intptr_t)(buffer_ptr1 + rept),
				v = (v & 0xFF) * 0x01010101;
			if (fin > (intptr_t)buffer_ptr2) fin = (intptr_t)buffer_ptr2;
			for (; (intptr_t)buffer_ptr1 < fin; buffer_ptr1 += 4)
				*(int32_t*)buffer_ptr1 = v;
			buffer_ptr1 = (uint8_t*)fin;
			rept = 1;
		}
		else if (sim->parts[ri].life == 16)
		{
			v = ~sim->parts[ri].ctype;
			switch (v)
			{
			case 0:
				f ^= 2;
				break;
			case 2:
				f |= 4;
			default:
				goto brkloop_0;
			case 1:
				rept = sim->parts[ri].tmp;
				break;
			}
		}
		else break;
	}
brkloop_0:
	if (f & 1)
	{
		v = inflate(&strm, Z_FINISH);
		if (v == Z_STREAM_END || v == Z_BUF_ERROR)
			buffer_ptr1 = strm.next_out;
		inflateEnd(&strm);
	}
	if (buffer_ptr1 >= buffer_ptr2) return;

	*buffer_ptr1 = 0; // add null
	buffer_ptr1 = (uint8_t*)(((intptr_t)buffer_ptr1 & (-ALIGNSIZE)) + ALIGNSIZE); // align

	int totalsize = buffer_ptr1 - mytxt_buffer1_curr;
	int position = sim->parts[i].ctype;
	int16_t * begin_block_ptr3 = (int16_t*)mytxt_buffer1_curr;

	begin_block_ptr3[0] = totalsize >> 1;
	begin_block_ptr3[1] = position & 0xFFFF;
	begin_block_ptr3[2] = (position >> 16) & 0xFFFF;
	
	mytxt_buffer1_parts++;
	mytxt_buffer1_curr = buffer_ptr1;
	
#ifdef DEBUG
	std::cout << std::hex << (int)mytxt_buffer1 << std::endl;
	std::cout << std::hex << (int)mytxt_buffer1_curr << std::endl;
#endif

	if (f & 4)
	{
		x += ix; y += iy;
		if (!sim->InBounds(x, y)) return;
		r = sim->pmap[y][x];
		if (TYP(r) != ELEM_MULTIPP || sim->partsi(r).life != 10) return;
		i = part_ID(r);
		goto newseg_0;
	}
	return;
#endif
}