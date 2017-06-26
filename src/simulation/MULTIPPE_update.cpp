#include "simulation/Elements.h"
#include "simulation/Air.h"
#include "simulation/MULTIPPE_Update.h"

#ifdef LUACONSOLE
#include "lua/LuaScriptInterface.h"
#include "lua/LuaScriptHelper.h"
#endif

#ifdef _MSC_VER
unsigned msvc_ctz(unsigned a)
{
	unsigned long i;
	_BitScanForward(&i, a);
	return i;
}

unsigned msvc_clz(unsigned a)
{
	unsigned long i;
	_BitScanReverse(&i, a);
	return 31 - i;
}

#define __builtin_ctz msvc_ctz
#define __builtin_clz msvc_clz
#endif


// 'UPDATE_FUNC_ARGS' definition: Simulation* sim, int i, int x, int y, int surround_space, int nt, Particle *parts, int pmap[YRES][XRES]
// FLAG_SKIPMOVE: not only implemented for PHOT

int MULTIPPE_Update::update(UPDATE_FUNC_ARGS)
{
	int return_value = 1; // skip movement, 'stagnant' check, legacyUpdate, etc.
	static int tron_rx[4] = {-1, 0, 1, 0};
	static int tron_ry[4] = { 0,-1, 0, 1};
	static int osc_r1 [4] = { 1,-1, 2,-2};
	int rx, ry, rlife = parts[i].life, r, ri, rtmp, rctype;
	int rr, rndstore, rt, rii, rrx, rry, nx, ny, pavg;
	// int tmp_r;
	float rvx, rvy, rdif;
	int docontinue;
	rtmp = parts[i].tmp;
	
	switch (rlife)
	{
	case 0: // acts like TTAN [压力绝缘体]
	case 1:
		{
			int ttan = 0;
			if (nt<=2)
				ttan = 2;
			else if (rlife)
				ttan = 2;
			else if (nt<=6)
				for (rx=-1; rx<2; rx++) {
					for (ry=-1; ry<2; ry++) {
						if ((!rx != !ry) && BOUNDS_CHECK) {
							if((pmap[y+ry][x+rx]&0xFF)==ELEM_MULTIPP)
								ttan++;
						}
					}
				}
			if(ttan>=2) {
				sim->air->bmap_blockair[y/CELL][x/CELL] = 1;
				sim->air->bmap_blockairh[y/CELL][x/CELL] = 0x8;
			}
		}
		break;
	case 2: // TRON input ["智能粒子" 的传送门入口]
		if (rtmp & 0x04)
			rtmp &= ~0x04;
		else if (rtmp & 0x01)
		{
			rr = (rtmp >> 5) & ((rtmp >> 19 & 1) - 1);
			int direction = (rr + (rtmp >> 17)) & 0x3;
			rx = x + tron_rx[direction];
			ry = y + tron_ry[direction];
			r = pmap[ry][rx];
			rii = parts[r >> 8].life;
			rrx = rii >> 1;
			if ((r & 0xFF) == ELEM_MULTIPP && (rrx == 1 || rrx == 15))
			{
				ri = r >> 8;
				if (rii == 31) // delay
				{
					if (parts[ri].tmp3)
						goto break1c;
					else
						parts[ri].tmp3 = parts[ri].ctype;
				}
				parts[ri].tmp &= (rii == 30 ? 0x700000 : 0) | 0xE0000;
				parts[ri].tmp |= (rtmp & 0x1FF9F) | (direction << 5);
				if (ri > i)
					sim->parts[ri].tmp |= 0x04;
				parts[ri].tmp2 = parts[i].tmp2;
			}
			else if ((r & 0xFF) == PT_METL || (r & 0xFF) == PT_PSCN || (r & 0xFF) == PT_NSCN)
			{
				conductTo (sim, r, rx, ry, parts);
			}
		break1c:
			rtmp &= 0xE0000;
		}
		parts[i].tmp = rtmp;
		break;
	case 3: // TRON output ["智能粒子" 的传送门出口]
		if (rtmp & 0x04)
			rtmp &= ~0x04;
		else if (rtmp & 0x01)
		{
			int direction = (rtmp >> 5) & 0x3;
			ry = y + tron_ry[direction];
			rx = x + tron_rx[direction];
			r = pmap[ry][rx];
			if (r)
			{
				direction = (direction + (rand()%2) * 2 + 1) % 4;
				ry = y + tron_ry[direction];
				rx = x + tron_rx[direction];
				r = pmap[ry][rx];
				if (r)
				{
					direction = direction ^ 0x2; // bitwise xor
					ry = y + tron_ry[direction];
					rx = x + tron_rx[direction];
					r = pmap[ry][rx];
				}
				if (r)
				{
					parts[i].tmp = 0;
					break;
				}
			}
			if (!r)
			{
				ri = sim->create_part(-1, rx, ry, PT_TRON);
				if (ri >= 0)
				{
					parts[ri].life = 5;
					parts[ri].tmp  = rtmp & 0x1FF9F | (direction << 5);
					if (ri > i)
						parts[ri].tmp |= 0x04;
					parts[ri].tmp2 = parts[i].tmp2;
				}
			}
			rtmp = 0;
		}
		parts[i].tmp = rtmp;
		break;
	case 4: // photon laser [激光器]
		if (!rtmp)
			break;

		rvx = (float)(((rtmp ^ 0x08) & 0x0F) - 0x08);
		rvy = (float)((((rtmp >> 4) ^ 0x08) & 0x0F) - 0x08);
		rdif = (float)((((rtmp >> 8) ^ 0x80) & 0xFF) - 0x80);

		ri = sim->create_part(-3, x + (int)rvx, y + (int)rvy, PT_PHOT);
		if (ri < 0)
			break;
		if (ri > i)
			parts[ri].flags |= FLAG_SKIPMOVE;
		parts[ri].vx = rvx * rdif / 16.0f;
		parts[ri].vy = rvy * rdif / 16.0f;
		rctype = parts[i].ctype;
		rtmp = rctype & 0x3FFFFFFF;
		rctype >>= 30;
		if (rtmp)
			parts[ri].ctype = rtmp;
		parts[ri].temp = parts[i].temp;
		parts[ri].life = parts[i].tmp2;
		parts[ri].tmp = rctype & 3;
		
		break;
	case 5: // reserved for Simulation.cpp
	case 7: // reserved for Simulation.cpp
#ifdef NO_SPC_ELEM_EXPLODE
	case 8:
	case 9:
#endif
	case 13: // decoration only, no update function
	case 15: // reserved for Simulation.cpp
	case 17: // reserved for 186.cpp and Simulation.cpp
	case 18: // decoration only, no update function
	case 22: // reserved for Simulation.cpp
	case 23: // reserved for stickmans
	case 25: // reserved for E189's life = 16, ctype = 10.
	case 27: // reserved for stickmans
	case 32: // reserved for ARAY / BRAY
		return return_value;
	case 6: // heater
		if (!sim->legacy_enable) //if heat sim is on
		{
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if ((rx || ry) && BOUNDS_CHECK)
					{
						r = pmap[y+ry][x+rx];
						if (!r)
							continue;
						if ((sim->elements[r&0xFF].HeatConduct > 0) && ((r&0xFF) != PT_HSWC || parts[r>>8].life == 10))
							parts[r>>8].temp = parts[i].temp;
					}
			if (sim->aheat_enable) //if ambient heat sim is on
			{
				sim->hv[y/CELL][x/CELL] = parts[i].temp;
				if (sim->air->bmap_blockairh[y/CELL][x/CELL] & 0x7)
				{
					// if bmap_blockairh exist or it isn't ambient heat insulator
					sim->air->bmap_blockairh[y/CELL][x/CELL] --;
					return return_value;
				}
			}
		}
		break;
#ifndef NO_SPC_ELEM_EXPLODE
	case 8: // acts like VIBR [振金]
		rr = parts[i].tmp2;
		if (parts[i].tmp > 20000)
		{
			sim->emp_trigger_count += 2;
			sim->emp_decor += 3;
			if (sim->emp_decor > 40)
				sim->emp_decor = 40;
			parts[i].life = 9;
			parts[i].temp = 0;
		}
		r = sim->photons[y][x];
		rndstore = rand();
		if (r && (r&0xFF) != PT_GRVT)
		{
			parts[i].tmp += 2;
			if (parts[r>>8].temp > 370.0f)
				parts[i].tmp += (int)parts[r>>8].temp - 369;
			if (3 > (rndstore & 0xF))
				sim->kill_part(r>>8);
			rndstore >>= 4;
		}
		// Pressure absorption code
		{
			float *pv1 = &sim->pv[y/CELL][x/CELL];
			{
				rii = (int)(*pv1);
				parts[i].tmp += *pv1 > 0.0f ? (10 * rii) : 0;
				*pv1 -= (float) rii;
			}
		}
		// Neighbor check loop
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK && (!rx != !ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (sim->elements[r&0xFF].HeatConduct > 0)
					{
						int transfer = (int)(parts[r>>8].temp - 273.15f);
						parts[i].tmp += transfer;
						parts[r>>8].temp -= (float)transfer;
					}
				}
		{
			int trade;
			for (trade = 0; trade < 9; trade++)
			{
				if (trade%2)
					rndstore = rand();
				rx = rndstore%7-3;
				rndstore >>= 3;
				ry = rndstore%7-3;
				rndstore >>= 3;
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					bool not_self = !((r&0xFF) == ELEM_MULTIPP && parts[r>>8].life == 8);
					if ((r&0xFF) != PT_VIBR && (r&0xFF) != PT_BVBR && not_self)
						continue;
					if (not_self)
					{
						if (rr & 1)
						{ // VIBR2 <- VIBR
							parts[i].tmp += parts[r>>8].tmp;
							parts[r>>8].tmp = 0;
						}
						else
						{ // VIBR2 -> VIBR
							parts[r>>8].tmp += parts[i].tmp;
							parts[i].tmp = 0;
						}
						break;
					}
					if (parts[i].tmp > parts[r>>8].tmp)
					{
						int transfer = (parts[i].tmp - parts[r>>8].tmp) >> 1;
						(transfer < 2) && (transfer = 1); // maybe CMOV instruction?
						parts[r>>8].tmp += transfer;
						parts[i].tmp -= transfer;
						break;
					}
				}
			}
		}
		if (parts[i].tmp < 0)
			parts[i].tmp = 0; // only preventing because negative tmp doesn't save
		break;
	case 9: // VIBR-like explosion
		if (parts[i].temp > (MAX_TEMP - 12))
		{
			sim->part_change_type(i, x, y, PT_VIBR);
			parts[i].temp = MAX_TEMP;
			parts[i].life = 1000;
			// parts[i].tmp2 = 0;
			sim->emp2_trigger_count ++;
			return return_value;
		}
		parts[i].temp += 12;
		{
			int trade = 5;
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						if (trade >= 5)
						{
							rndstore = rand(); trade = 0;
						}
						r = pmap[y+ry][x+rx];
						rt = r & 0xFF;
						if (!r || (sim->elements[rt].Properties2 & PROP_NODESTRUCT) || rt == PT_VIBR || rt == PT_BVBR || rt == PT_WARP || rt == PT_SPRK)
							continue;
						if (rt == ELEM_MULTIPP)
						{
							if (parts[r>>8].life == 8)
							{
								parts[r>>8].tmp += 1000;
								continue;
							}
							if (parts[r>>8].life == 9)
								continue;
						}
						if (!(rndstore & 0x7))
						{
							sim->part_change_type(r>>8, x+rx, y+ry, ELEM_MULTIPP);
							parts[r>>8].life = 8;
							parts[r>>8].tmp = 21000;
						}
						trade++; rndstore >>= 3;
					}
		}
		break;
#endif /* NO_SPC_ELEM_EXPLODE */
	case 10: // electronics debugger input [电子产品调试]
		for (rx = -1; rx <= 1; rx++)
			for (ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						switch (rtmp & 0xFF)
						{
						case 0:
							if (Element_MULTIPP::maxPrior <= parts[i].ctype)
							{
								sim->SimExtraFunc |= 0x81;
								Element_MULTIPP::maxPrior = parts[i].ctype;
							}
							break;
						case 1: sim->SimExtraFunc |=  0x02; break;
						case 2: sim->SimExtraFunc |=  0x08; break;
						case 3: sim->SimExtraFunc &= ~0x08; break;
						case 4: sim->SimExtraFunc |=  0x10; break;
						case 5: sim->SimExtraFunc |=  0x20; break;
						case 6:
							switch (parts[r>>8].ctype)
							{
								case PT_NTCT: Element_PHOT::ignite_flammable = 0;
								case PT_PTCT: Element_PHOT::ignite_flammable = 1;
								default: sim->SimExtraFunc |= 0x40;
							}
							break;
						case 7:
							if (parts[i].temp < 273.15f)
								parts[i].temp = 273.15f;
							rdif = (int)(parts[i].temp - 272.65f);
							if (parts[r>>8].ctype != PT_INST)
								rdif /= 100.0f;
							sim->sim_max_pressure += rdif;
							if (sim->sim_max_pressure > 256.0f)
								sim->sim_max_pressure = 256.0f;
							break;
						case 8:
							if (parts[i].temp < 273.15f)
								parts[i].temp = 273.15f;
							rdif = (int)(parts[i].temp - 272.65f);
							if (parts[r>>8].ctype != PT_INST)
								rdif /= 100.0f;
							sim->sim_max_pressure -= rdif;
							if (sim->sim_max_pressure < 0.0f)
								sim->sim_max_pressure = 0.0f;
							break;
						case 9: // set sim_max_pressure
							if (parts[i].temp < 273.15f)
								parts[i].temp = 273.15f;
							if (parts[i].temp > 273.15f + 256.0f)
								parts[i].temp = 273.15f + 256.0f;
							sim->sim_max_pressure = (int)(parts[i].temp - 272.65f);
							break;
						case 10: // reset currentTick
							sim->lightningRecreate -= sim->currentTick;
							if (sim->lightningRecreate < 0)
								sim->lightningRecreate = 0;
							sim->currentTick = 0;
							if (parts[r>>8].ctype == PT_INST)
								sim->elementRecount = true;
							break;
						case 11:
							rctype = parts[r>>8].ctype;
							rr = pmap[y-ry][x-rx];
							{
								int rrt = parts[rr>>8].ctype & 0xFF, tFlag = 0;
								switch (rr & 0xFF)
								{
									case PT_STOR: tFlag = PROP_CTYPE_INTG; break;
									case PT_CRAY: tFlag = PROP_CTYPE_WAVEL; break;
									case PT_DRAY: tFlag = PROP_CTYPE_SPEC; break;
								}
								switch (rctype)
								{
									case PT_PSCN: sim->elements[rrt].Properties2 |=  tFlag; break;
									case PT_NSCN: sim->elements[rrt].Properties2 &= ~tFlag; break;
									case PT_INWR: sim->elements[rrt].Properties2 ^=  tFlag; break;
								}
							}
							break;
						case 12:
							if (Element_MULTIPP::maxPrior < parts[i].ctype)
							{
								sim->SimExtraFunc |= 0x80;
								sim->SimExtraFunc &= ~0x01;
								Element_MULTIPP::maxPrior = parts[i].ctype;
							}
							break;
							// 'decorations_enable' 属于 'Renderer', 不是 'Simulation'
						}
						if ((rtmp & 0x1FE) == 0x100 && (rx != ry))
							MULTIPPE_Update::InsertText(sim, i, x, y, -rx, -ry);
					}
				}
		break;
	case 11: // photons emitter
		for (rx = -1; rx <= 1; rx++)
			for (ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						ri = sim->create_part(-3, x-rx, y-ry, PT_PHOT);
						parts[ri].vx = (float)(-3 * rx);
						parts[ri].vy = (float)(-3 * ry);
						parts[ri].life = parts[i].tmp2;
						parts[ri].temp = parts[i].temp;

						rtmp = parts[i].ctype & 0x3FFFFFFF;
						if (rtmp)
							parts[ri].ctype = rtmp;

						if (ri > i)
							parts[ri].flags |= FLAG_SKIPMOVE;
					}
				}
		break;
	case 12: // SPRK reflector
		if (!(rtmp & 0x4))
		{
			for (rx = -1; rx <= 1; rx++)
				for (ry = -1; ry <= 1; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						rt = r & 0xFF;
						rtmp = parts[i].tmp;
						if (!r)
							continue;
						if (rt == PT_SPRK && ( !(rtmp & 8) == !(sim->elements[parts[r>>8].ctype].Properties & PROP_INSULATED) ) && parts[r>>8].life == 3)
						{
							switch (rtmp & 0x3)
							{
							case 0: case 1:
								parts[i].tmp ^= 1; break;
							case 2:
								rr = pmap[y-ry][x-rx];
								if ((rr & 0xFF) == ELEM_MULTIPP)
									sim->kill_part(rr >> 8);
								else
								{
									ri = sim->create_part(-1,x-rx,y-ry,ELEM_MULTIPP,12);
									if (ri >= 0)
										parts[ri].tmp = 3;
								}
								break;
							}
						}
						if ((rtmp & 0x1) && (sim->elements[rt].Properties & (PROP_CONDUCTS|PROP_INSULATED)) == PROP_CONDUCTS)
						{
							conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
		}
		break;
	case 14: // dynamic decoration (DECO2)
		switch (parts[i].tmp2 >> 24)
		{
		case 0:
			rtmp = (parts[i].tmp & 0xFFFF) + (parts[i].tmp2 & 0xFFFF);
			rctype = (parts[i].ctype >> 16) + (parts[i].tmp >> 16) + (rtmp >> 16);
			parts[i].tmp2 = (parts[i].tmp2 & ~0xFFFF) | (rtmp & 0xFFFF);
			parts[i].ctype = (parts[i].ctype & 0xFFFF) | ((rctype % 0x0600) << 16);
			break;
		case 1:
			rtmp  = (parts[i].ctype & 0x7F7F7F7F) + (parts[i].tmp & 0x7F7F7F7F);
			rtmp ^= (parts[i].ctype ^ parts[i].tmp) & 0x80808080;
			parts[i].ctype = rtmp;
			break;
		case 2:
			rtmp = parts[i].tmp2 & 0x00FFFFFF;
			rtmp ++;
			if (parts[i].tmp3 <= rtmp)
			{
				rtmp = parts[i].tmp;
				parts[i].tmp = parts[i].ctype;
				parts[i].ctype = rtmp;
				rtmp = 0;
			}
			parts[i].tmp2 = 0x02000000 | rtmp;
			break;
		}
		break;
	case 16:
		switch (rctype = parts[i].ctype)
		{
		case 0: // logic gate
			{
				int conductive;
				// char* ptr1 = &(parts[i].tmp2);
				rrx = parts[i].tmp2 & 0xFF;  // movzx reg, al ???
				if (rrx)
					parts[i].tmp2 --;
				rry = (parts[i].tmp2 >> 8) & 0xFF;  // movzx reg, ah ???
				if (rry)
					parts[i].tmp2 -= 0x100;
				switch (rtmp & 7)
				{
					case 0: conductive =  rrx ||  rry; break;
					case 1: conductive =  rrx &&  rry; break;
					case 2: conductive =  rrx && !rry; break;
					case 3: conductive = !rrx &&  rry; break;
					case 4: conductive = !rrx != !rry; break;
					case 5: conductive =  rrx; break; // input 1 detector
					case 6: conductive =  rry; break; // input 2 detector
				}
				if (rtmp & 8)
					conductive = !conductive;

				// PSCNCount = 0;
				{
					for (rx = -2; rx <= 2; rx++)
						for (ry = -2; ry <= 2; ry++)
							if (BOUNDS_CHECK && (rx || ry))
							{
								r = pmap[y+ry][x+rx];
								if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
								{
									if (parts[r>>8].ctype == PT_PSCN)
									{
										if (parts[r>>8].tmp & 1)
										{
											parts[i].tmp2 &= ~(0xFF<<8);
											parts[i].tmp2 |= 8<<8;
										}
										else
										{
											parts[i].tmp2 &= ~0xFF;
											parts[i].tmp2 |= 8;
										}
									}
								}
								else if ((r & 0xFF) == PT_NSCN && conductive)
									conductTo (sim, r, x+rx, y+ry, parts);
							}
				}
			}
			break;
		case 1: // conduct->insulate counter
			if (parts[i].tmp)
			{
				if (parts[i].flags & FLAG_SKIPMOVE) // if wait flag exist
				{
					parts[i].flags &= ~FLAG_SKIPMOVE; // clear wait flag
					return return_value;
				}
				if (parts[i].tmp2)
				{
					for (rx = -2; rx <= 2; rx++)
						for (ry = -2; ry <= 2; ry++)
							if (BOUNDS_CHECK && (rx || ry))
							{
								r = pmap[y+ry][x+rx];
								if ((r & 0xFF) == PT_NSCN)
									conductTo (sim, r, x+rx, y+ry, parts);
							}
					parts[i].tmp--;
					parts[i].tmp2 = 0;
				}
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if ((r & 0xFF) == PT_SPRK && parts[r>>8].ctype == PT_PSCN && parts[r>>8].life == 3)
							{
								parts[i].tmp2 = 1;
								return return_value;
							}
						}
			}
			return return_value;
		case 2: // insulate->conduct counter
			if (parts[i].tmp2)
			{
				if (parts[i].tmp2 == 1)
					parts[i].tmp--;
				parts[i].tmp2--;
			}
			else if (!parts[i].tmp)
			{
				sim->part_change_type(i, x, y, PT_METL);
				parts[i].life = 1;
				parts[i].ctype = PT_NONE;
				break;
			}
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == PT_SPRK && parts[r>>8].ctype == PT_PSCN && parts[r>>8].life == 3)
						{
							parts[i].tmp2 = 6;
							return return_value;
						}
					}
			break;
		case 3: // flip-flop
			if (parts[i].tmp >= (int)(parts[i].temp - 73.15f) / 100)
			{
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if ((r & 0xFF) == PT_NSCN)
								conductTo (sim, r, x+rx, y+ry, parts);
						}
				parts[i].tmp = 0;
			}
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == PT_SPRK && parts[r>>8].ctype == PT_PSCN && parts[r>>8].life == 3)
						{
							parts[i].tmp ++;
							return return_value;
						}
					}
			break;
		case 4: // virus curer
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == ELEM_MULTIPP && parts[r>>8].life == 16)
							r = pmap[y+2*ry][x+2*rx];
						if ((r & 0xFF) == PT_SPRK)
							goto break2a;
					}
			break;
		break2a:
			for (rii = 0; rii < 4; rii++)
			{
				if (BOUNDS_CHECK)
				{
					rx = tron_rx[rii];
					ry = tron_ry[rii];
					rr = pmap[y+ry][x+rx];
					if ((rr&0xFF) == PT_VIRS || (rr&0xFF) == PT_VRSS || (rr&0xFF) == PT_VRSG) // if is virus
					{
						parts[r>>8].pavg[0] += 10;
					}
				}
			}
			break;
		case 5:
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
						{
							parts[i].tmp2 = 10;
							goto break2b;
						}
					}
			// break;
		break2b:
			if (parts[i].tmp2)
			{
				if (parts[i].tmp2 == 9)
				{
					for (rtmp = 0; rtmp < 4; rtmp++)
					{
						if (BOUNDS_CHECK)
						{
							rx = tron_rx[rtmp];
							ry = tron_ry[rtmp];
							r = pmap[y+ry][x+rx];
							switch (r&0xFF)
							{
								case PT_NONE:  continue;
								case PT_STKM:  r = sim->player.underp; break;
								case PT_STKM2: r = sim->player2.underp; break;
								case PT_FIGH:
									rii = parts[r>>8].tmp;
									if (rii < 0 || rii >= MAX_FIGHTERS) continue;
									r = sim->fighters[parts[r>>8].tmp].underp;
								break;
								// case PT_PINVIS: r = parts[r>>8].tmp4; break;
							}
							rii = parts[r>>8].tmp2;
							if (rii & (r>>8)>i) // If the other particle hasn't been life updated
								rii--;
							if ((r&0xFF) == ELEM_MULTIPP && parts[r>>8].life == 16 && parts[r>>8].ctype == 5 && !rii)
							{
								parts[r>>8].tmp2 = (r>>8) > i ? 10 : 9;
							}
						}
					}
					parts[i].tmp = (parts[i].tmp & ~0x3F) | ((parts[i].tmp >> 3) & 0x7) | ((parts[i].tmp & 0x7) << 3);
				}
				parts[i].tmp2--;
			}
			break;
		case 6: // wire crossing
		case 7:
			{
				if (rtmp>>8)
				{
					if ((rtmp>>8) == 3)
					{
						for (rii = 0; rii < 4; rii++)
						{
							if (BOUNDS_CHECK)
							{
								r = osc_r1[rii], rtmp = parts[i].tmp;
								if (rtmp & 1 << (rctype & 1))
								{
									rx = pmap[y][x+r];
									if (sim->elements[rx&0xFF].Properties&PROP_CONDUCTS)
										conductTo(sim, rx, x+r, y, parts);
								}
								if (rtmp & 2 >> (rctype & 1))
								{
									ry = pmap[y+r][x];
									if (sim->elements[ry&0xFF].Properties&PROP_CONDUCTS)
										conductTo(sim, ry, x, y+r, parts);
								}
							}
						}
					}
					parts[i].tmp -= 1<<8;
				}
				for (rr = rii = 0; rii < 4; rii++)
				{
					if (BOUNDS_CHECK)
					{
						r = osc_r1[rii];
						rx = pmap[y][x+r];
						ry = pmap[y+r][x];
						if ((rx & 0xFF) == PT_SPRK && parts[rx>>8].life == 3) rr |= 1;
						if ((ry & 0xFF) == PT_SPRK && parts[ry>>8].life == 3) rr |= 2;
					}
				}
				if (rr && !((rctype & 1) && rtmp>>8))
				{
					parts[i].tmp = rr | 3<<8;
				}
			}
			break;
		case 8: // FIGH stopper
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
						{
							rtmp = parts[i].tmp;
							if (rtmp >= 0)
								sim->Extra_FIGH_pause_check |= 1 << (rtmp < 31 ? (rtmp > 0 ? rtmp : 0) : 31);
							else
								sim->SimExtraFunc |= 4;
							return return_value;
						}
					}
			break;
		case 9:
			if (rtmp >= 0)
				rt = 1 & (sim->Extra_FIGH_pause >> (rtmp & 0x1F));
			else
				rt = (int)(sim->no_generating_BHOL);
			for (rr = 0; rr < 4; rr++)
				if (BOUNDS_CHECK)
				{
					rx = tron_rx[rr];
					ry = tron_ry[rr];
					r = pmap[y+ry][x+rx];
					switch (r & 0xFF)
					{
					case PT_SPRK:
						if (!rt && parts[r>>8].ctype == PT_SWCH)
						{
							parts[r>>8].life = 9;
							parts[r>>8].ctype = PT_NONE;
							sim->part_change_type(r>>8, rx, ry, PT_SWCH);
						}
						break;
					case PT_SWCH:
					case PT_HSWC:
						rtmp = parts[r>>8].life;
						if (rt)
							parts[r>>8].life = 10;
						else if (rtmp >= 10)
							parts[r>>8].life = 9;
						break;
					case PT_LCRY:
						rtmp = parts[r>>8].tmp;
						if (rt && rtmp == 0)
							parts[r>>8].tmp = 2;
						if (!rt && rtmp == 3)
							parts[r>>8].tmp = 1;
						break;
					}
				}
			break;
		case 10: // with E189's life=17 (and life=25)
			for (rr = 0; rr < 4; rr++)
				if (BOUNDS_CHECK)
				{
					rx = tron_rx[rr];
					ry = tron_ry[rr];
					r = pmap[y+ry][x+rx];
					if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						int direction = rr;
						for (rr = 0; rr < 4; rr++) // reset "rr" variable
							if (BOUNDS_CHECK)
							{
								rrx = tron_rx[rr];
								rry = tron_ry[rr];
								ri = pmap[y+rry][x+rrx];
								if ((ri & 0xFF) == ELEM_MULTIPP && parts[ri>>8].life == 17) // using "PHOT diode"
								{
									rii = sim->create_part(-1, x-rx, y-ry, ELEM_MULTIPP, 24);
									rtmp = (direction << 2) | rr;
									if (rii >= 0)
									{
										parts[rii].ctype = rtmp;
										parts[rii].tmp = parts[ri>>8].tmp;
										parts[rii].tmp2 = parts[ri>>8].tmp2;
										parts[rii].tmp3 = parts[i].tmp;
										if (rii > i)
											parts[rii].flags |= FLAG_SKIPMOVE; // set wait flag
									}
									r = pmap[y-rry][x-rrx]; // variable "r" value override
									if ((r & 0xFF) == ELEM_MULTIPP && parts[r>>8].life == 25)
									{
										rii = sim->create_part(-1, x-rx-rrx, y-ry-rry, ELEM_MULTIPP, 24);
										if (rii >= 0)
										{
											parts[rii].ctype = rtmp ^ 2;
											parts[rii].tmp = parts[ri>>8].tmp;
											parts[rii].tmp2 = parts[ri>>8].tmp2;
											parts[rii].tmp3 = parts[r>>8].tmp; // fixed overflow?
											if (rii > i)
												parts[rii].flags |= FLAG_SKIPMOVE; // set wait flag
										}
									}
									return return_value;
								}
							}
						break;
					}
				}
			return return_value;
		// case 11: reserved for E189's life = 24.
		case 12:
			{
				rndstore = rand();
				rx = (rndstore&1)*2-1;
				ry = (rndstore&2)-1;
				if (parts[i].tmp2)
				{
					for (rii = 1; rii <= 2; rii++)
					{
						if (BOUNDS_CHECK)
						{
							rtmp = parts[i].tmp;
							rrx = pmap[y][x+rx*rii];
							rry = pmap[y+ry*rii][x];
							if ((rry & 0xFF) == PT_NSCN && (rtmp & 1))
								conductTo (sim, rry, x, y+ry*rii, parts);
							if ((rrx & 0xFF) == PT_NSCN && (rtmp & 2))
								conductTo (sim, rrx, x+rx*rii, y, parts);
						}
					}
					parts[i].tmp2 = 0;
				}
				for (rr = rii = 0; rii < 4; rii++)
				{
					if (BOUNDS_CHECK)
					{
						r = osc_r1[rii];
						rx = pmap[y][x+r];
						ry = pmap[y+r][x];
						if ((rx & 0xFF) == PT_SPRK && parts[rx>>8].life == 3 && parts[rx>>8].ctype == PT_PSCN) rr |= 1;
						if ((ry & 0xFF) == PT_SPRK && parts[ry>>8].life == 3 && parts[ry>>8].ctype == PT_PSCN) rr |= 2;
					}
				}
				if (rr && !((rctype & 1) && parts[i].tmp2))
				{
					parts[i].tmp = rr; parts[i].tmp2 = 1;
				}
			}
			break;
		case 13:
			if (parts[i].tmp2)
			{
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if ((r & 0xFF) == PT_NSCN)
								sim->create_part(r>>8,x+rx,y+ry,PT_PSCN);
							if ((r & 0xFF) == PT_PSCN)
							{
								parts[r>>8].ctype = PT_NSCN; // for different type
								sim->part_change_type(r>>8, x, y, PT_SPRK);
								parts[r>>8].life = 4;
							}
						}
				parts[i].tmp2 = 0;
			}
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						rr = ((r>>8) > i) ? (parts[r>>8].tmp) : (parts[r>>8].tmp2);
						if ((r & 0xFF) == ELEM_MULTIPP && parts[r>>8].life == 19 && rr == 9)
						{
							parts[i].tmp2 = 1;
							return return_value;
						}
					}
			break;
		case 14:
		case 15:
			if (parts[i].tmp2)
			{
				rdif = (parts[i].tmp == PT_PSCN) ? 100.0f : -100.0f;
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (rctype == 14 ? (r & 0xFF) == PT_WIFI : ((r & 0xFF) == ELEM_MULTIPP && parts[r>>8].life == 33) )
							{
								parts[r>>8].temp = restrict_flt(parts[r>>8].temp + rdif, MIN_TEMP, MAX_TEMP);
							}
						}
				parts[i].tmp = 0; // PT_NONE ( or clear .tmp )
				parts[i].tmp2 = 0;
			}
			goto continue1a;
		case 16:
			if (parts[i].tmp2)
			{
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if ((r & 0xFF) == PT_PSNS || (r & 0xFF) == PT_TSNS || (r & 0xFF) == PT_DTEC || (r & 0xFF) == PT_LSNS)
							{
								parts[r>>8].tmp3 ^= 1;
							}
						}
				parts[i].tmp2 = 0;
			}
			goto continue1a;
		case 17:
			if (parts[i].tmp2)
			{
				for (rx = -1; rx < 2; rx++)
					for (ry = -1; ry < 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							nx = x + rx; ny = y + ry;
							r = pmap[ny][nx];
							if (!r)
								continue;
							if ((r & 0xFF) == PT_FILT)
							{
								rrx = parts[r>>8].ctype;
								rry = parts[i].tmp;
								switch (rry) // rry = sender type
								{
								// rrx = wavelengths
								case PT_PSCN: case PT_NSCN:
									rrx += (rry == PT_PSCN) ? 1 : -1;
									break;
								case PT_INST:
									rrx <<= 1;
									rrx |= (rrx >> 29) & 1; // for 29-bit FILT data
									break;
								case PT_INWR:
									// for 29-bit FILT data
									rrx &= 0x1FFFFFFF;
									rrx = (rrx >> 1) | (rrx << 28);
									break;
								case PT_PTCT: // int's size is 32-bits
#if defined(__GNUC__) || defined(_MSVC_VER)
									rrx <<= 3;
									rrx ^= (signed int)0x80000000 >> __builtin_clz(~rrx); // increment reversed int.
									rrx >>= 3;
#else
									{
										int tmp_r = 0x10000000;
										while (tmp_r)
										{
											rrx ^= tmp_r;
											if ((rrx & tmp_r)) break;
											tmp_r >>= 1;
										}
									}
#endif
									break;
								case PT_NTCT: // int's size is 32-bits
#if defined(__GNUC__) || defined(_MSVC_VER)
									rrx <<= 3;
									rrx ^= (signed int)0x80000000 >> __builtin_clz(rrx | 1); // decrement reversed int.
									// __builtin_clz(0x00000000) is undefined.
									rrx >>= 3;
#else
									{
										int tmp_r = 0x10000000;
										while (tmp_r)
										{
											rrx ^= tmp_r;
											if (!(rrx & tmp_r)) break;
											tmp_r >>= 1;
										}
									}
#endif
									break;
								}
								rrx &= 0x1FFFFFFF;
								rrx |= 0x20000000;
								while (BOUNDS_CHECK && (
									(r & 0xFF) == PT_FILT
								)) // check another FILT
								{
									parts[r>>8].ctype = rrx;
									r = pmap[ny += ry][nx += rx];
								}
								continue;
							}
							else if ((r & 0xFF) == PT_CRAY)
							{
								int ncount = 0;
								docontinue = 1;
								rrx = (rtmp == PT_PSCN) ? 1 : 0;
								if (rtmp != PT_NSCN && !rrx)
									continue;
								rry = 0;
								if (parts[r>>8].dcolour == 0xFF000000) // if black deco is on
									rry = 0xFF000000; // set deco colour to black
								while (docontinue)
								{
									nx += rx; ny += ry;
									if (!sim->InBounds(nx, ny))
									{
									break1d:
										break;
									}
									ncount++;
									r = pmap[ny][nx];
									if (!r)
									{
										ri = sim->create_part(-1, nx, ny, PT_INWR);
										if (ri >= 0)
											parts[ri].dcolour = rry;
										docontinue = !rrx;
										continue;
									}
									switch (r&0xFF)
									{
									case PT_INWR:
										sim->kill_part(r>>8);
										docontinue = rrx;
										continue;
									case PT_FILT:
										if (parts[r>>8].tmp == 0) // if mode is "set colour"
											rry = parts[r>>8].dcolour;
										break;
									case PT_BRCK:
										docontinue = parts[r>>8].tmp;
										parts[r>>8].tmp = 1;
										continue;
									case ELEM_MULTIPP:
										if (parts[r>>8].life == 3)
										{
											r = pmap[ny+ry][nx+rx];
											if ((r&0xFF) == PT_METL || (r&0xFF) == PT_INDC)
												conductTo (sim, r, nx+rx, ny+ry, parts);
											while (--ncount)
											{
												nx -= rx; ny -= ry;
												rr = pmap[ny][nx];
												if ((rr & 0xFF) == PT_BRCK)
													parts[rr>>8].tmp = 0;
											}
										}
										goto break1d;
									default:
										docontinue = 0;
									}
								}
								continue;
							}
						}
				parts[i].tmp2 = 0;
			}
			goto continue1a;
		case 18:
			if (parts[i].tmp2)
			{
				for (rx = -1; rx < 2; rx++)
					for (ry = -1; ry < 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							nx = x + rx; ny = y + ry;
							r = pmap[ny][nx];
							if (!r)
								continue;
							switch (r & 0xFF)
							{
							case PT_FILT:
								rrx = parts[r>>8].ctype;
								rry = parts[i].tmp;
								switch (rry) // rry = sender type
								{
								// rrx = wavelengths
								case PT_PSCN:
									rrx &= ~0x1;
								case PT_NSCN:
									rrx |= 0x1;
									break;
								case PT_INWR:
									rrx ^= 0x1;
									break;
								case PT_PTCT:
									rrx <<= 1;
									break;
								case PT_NTCT:
									// for 29-bit FILT data
									rrx = (rrx >> 1) & 0x0FFFFFFF;
									break;
								case PT_INST:
									rry = (rrx ^ (rrx>>28)) & 0x00000001; // swap 28
									rrx ^= (rry| (rry<<28));
									rry = (rrx ^ (rrx>>18)) & 0x000003FE; // swap 18
									rrx ^= (rry| (rry<<18));
									rry = (rrx ^ (rrx>> 6)) & 0x00381C0E; // swap 6
									rrx ^= (rry| (rry<< 6));
									rry = (rrx ^ (rrx>> 2)) & 0x02492492; // swap 2
									parts[r>>8].ctype = rry ^ (rry | (rry << 2));
									goto continue1b;
								}
								rrx &= 0x1FFFFFFF;
								rrx |= 0x20000000;
							continue1b:
								while (BOUNDS_CHECK && (
									(r & 0xFF) == PT_FILT
								)) // check another FILT
								{
									parts[r>>8].ctype = rrx;
									r = pmap[ny += ry][nx += rx];
								}
								break;
							case PT_PUMP: case PT_GPMP:
								rrx = r & 0xFF;
								rdif = (parts[i].tmp == PT_PSCN) ? 1.0f : -1.0f;
								while (BOUNDS_CHECK && (r & 0xFF) == rrx) // check another pumps
								{
									parts[r>>8].temp += rdif;
									r = pmap[ny += ry][nx += rx];
								}
								break;
							case PT_FRAY:
								rrx = r & 0xFF;
								rii = (parts[i].tmp == PT_PSCN) ? 1 : -1;
								parts[r>>8].tmp += rii;
								if (parts[r>>8].tmp < 0)
									parts[r>>8].tmp = 0;
								break;
							case PT_C5:
								r = pmap[ny += ry][nx += rx];
								if (((r&0xFF) == PT_VIBR || (r&0xFF) == PT_BVBR) && parts[r>>8].life)
								{
									parts[r>>8].tmp2 = 1; parts[r>>8].tmp = 0;
								}
								break;
							case PT_WIFI:
								{ // for 29-bit FILT data
									rii = (int)((parts[r>>8].temp-73.15f)/100+1);
									if (rii < 0) rii = 0;
									r = pmap[ny += ry][nx += rx];
									sim->ISWIRE = 2;
									if ((r&0xFF) == PT_FILT)
									{
										rrx = parts[r>>8].ctype & 0x1FFFFFFF;
										for (; rrx && rii < CHANNELS; rii++)
										{
											if (rrx & 1)
												sim->wireless[rii][1] = 1;
											rrx >>= 1;
										}
									}
								}
								break;
							}
						}
				parts[i].tmp2 = 0;
			}
			goto continue1a;
		continue1a:
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (!r) continue;
						pavg = sim->parts_avg(i,r>>8,PT_INSL);
						if ((pavg != PT_INSL && pavg != PT_INDI) && (r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
						{
							parts[i].tmp = parts[r>>8].ctype;
							parts[i].tmp2 = 1;
							return return_value;
						}
					}
			break;
		case 19: // universal conducts (without WWLD)?
			// int oldl;
			/* oldl */ rii = parts[i].tmp2;
			if (parts[i].tmp2)
			{
				parts[i].tmp2--;
				// break;
			}
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (!rx || !ry))
					{
						r = pmap[y+ry][x+rx];
						if (!r) continue;
						pavg = sim->parts_avg(i,r>>8,PT_INSL);
						if (pavg == PT_INSL || pavg == PT_INDI)
							continue;
						if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3 && !rii)
						{
							parts[i].tmp2 = 2;
							// return return_value;
						}
						else if ((r & 0xFF) == PT_WIRE)
						{
							if (!rii && parts[r>>8].tmp == 1)
							{
								parts[i].tmp2 = 2;
							}
							else if (!parts[r>>8].tmp && rii == 2)
							{
								parts[r>>8].ctype = 1;
								if ((r>>8) > i)
									parts[r>>8].flags |= FLAG_SKIPMOVE;
							}
						}
						else if (rii == 2)
						{
							if ((r & 0xFF) == PT_INST)
								sim->FloodINST(x+rx,y+ry,PT_SPRK,PT_INST);
							else if ((r & 0xFF) == PT_SWCH)
								conductToSWCH (sim, r, x+rx, y+ry, parts);
							else if (sim->elements[r & 0xFF].Properties & PROP_CONDUCTS)
								conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
			break;
		case 20: // clock (like battery)
			if (parts[i].tmp2)
				parts[i].tmp2--;
			else
			{
				for (rx = -1; rx < 2; rx++)
					for (ry = -1; ry < 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (sim->elements[r&0xFF].Properties & PROP_CONDUCTS)
								conductTo (sim, r, x+rx, y+ry, parts);
						}
				parts[i].tmp2 = parts[i].tmp - 1;
			}
			
			break;
		case 21: // subframe SPRK generator/canceller
			rrx = (parts[i].temp < 373.0f) ? 4 : 3;
			rry = (parts[i].temp < 273.15f);
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r&0xFF) == PT_SPRK)
						{
							if (rry)
							{
								rii = parts[r>>8].ctype;
								if (rii > 0 && rii < PT_NUM && sim->elements[rii].Enabled)
								{
									sim->part_change_type(r>>8, x+rx, y+ry, rii);
									parts[r>>8].life = (rii == PT_SWCH) ? 10 : 0; // keep SWCH on
									parts[r>>8].ctype = PT_NONE;
								}
							}
							else
								parts[r>>8].life = rrx;
						}
						else if (sim->elements[r&0xFF].Properties & PROP_CONDUCTS || (r&0xFF) == PT_INST)
						{
							if (rry)
								parts[r>>8].life = 0;
							else
							{
								parts[r>>8].ctype = r&0xFF;
								parts[r>>8].life = rrx;
								sim->part_change_type(r>>8, x+rx, y+ry, PT_SPRK);
							}
						}
					}
			break;
		case 22: // custom GOL type changer
			if (parts[i].tmp2)
			{
				for (rx = -1; rx < 2; rx++)
					for (ry = -1; ry < 2; ry++)
						if (BOUNDS_CHECK && (!rx != !ry))
						{
							nx = x + rx; ny = y + ry;
							r = pmap[ny][nx];
							if (!r)
								continue;
							switch (r & 0xFF)
							{
							case PT_FILT:
								rrx = parts[r>>8].ctype;
								rry = parts[i].tmp;
								switch (rry)
								{
								case PT_PSCN: case PT_NSCN: // load GOL rule from sim particles
									{
										int ipos = 0x1;
										int imask = (rry == PT_PSCN ? 0x1 : 0x2);
										for (int _it = 0; _it < 9; _it++)
										{
											sim->grule[NGT_CUSTOM+1][_it] &= ~imask;
											if (rrx & ipos)
												sim->grule[NGT_CUSTOM+1][_it] |= imask;
											ipos <<= 1;
										}
									}
									break;
								}
								break;
							case PT_ACEL:
								sim->grule[NGT_CUSTOM+1][9] += std::max(0, parts[r>>8].life);
								break;
							case PT_DCEL:
								rrx = sim->grule[NGT_CUSTOM+1][9] - std::max(0, parts[r>>8].life);
								sim->grule[NGT_CUSTOM+1][9] = (rrx < 2 ? 2 : rrx);
								break;
							case PT_FRAY:
								sim->grule[NGT_CUSTOM+1][9] = std::max(0, parts[r>>8].tmp) + 2;
								break;
							case PT_DTEC: // store GOL rule to sim particles
								{
									rrx = parts[r>>8].ctype;
									int c_gol = 0;
									if (rrx == PT_LIFE)
									{
										rii = parts[r>>8].tmp;
										if (rii >= 0 && rii < NGOL)
											c_gol = rii + 1;
									}
									rry = parts[i].tmp;
									int frr = pmap[ny+=ry][nx+=rx]; // front particle "r"
									switch (frr&0xFF) // check front particle type
									{
									case PT_FILT:
										{
											rctype = parts[frr>>8].ctype;
											rctype &= 0x3FFFFE00; // 0x3FFFFE00 == (0x3FFFFFFF & ~0x000001FF)
											int ipos = 0x1;
											int imask = (rry == PT_PSCN ? 0x1 : 0x2);
											for (int _it = 0; _it < 9; _it++)
											{
												rctype |= ((sim->grule[c_gol][_it] & imask) ? ipos : 0);
												ipos <<= 1;
											}
											parts[frr>>8].ctype = rctype | 0x20000000;
										}
										break;
									case PT_FRAY:
										parts[frr>>8].tmp = sim->grule[c_gol][9] - 2;
										break;
									}
								}
								break;
							}
						}
			}
			goto continue1a;
		case 23: // powered BTRY
			for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry) && abs(rx)+abs(ry) < 4)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				pavg = sim->parts_avg(i,r>>8,PT_INSL);
				if (pavg != PT_INSL && pavg != PT_INDI)
				{
					rt = (r&0xFF);
					if (rt == PT_SPRK && parts[r>>8].life == 3)
					{
						switch (parts[r>>8].ctype)
						{
							case PT_NSCN: parts[i].tmp = 0; break;
							case PT_PSCN: parts[i].tmp = 1; break;
							case PT_INST: parts[i].tmp = !parts[i].tmp; break;
						}
					}
					else if (rtmp && rt != PT_PSCN && rt != PT_NSCN &&
						(sim->elements[rt].Properties&(PROP_CONDUCTS|PROP_INSULATED)) == PROP_CONDUCTS)
						conductTo (sim, r, x+rx, y+ry, parts);
				}
			}
			return return_value;
		case 24: // shift register
			if (rtmp <= 0)
				rtmp = XRES + YRES;
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y-ry][x-rx];
						if ((r&0xFF) == PT_SPRK && parts[r>>8].life == 3)
						{
							rctype = parts[r>>8].ctype;
							switch (rctype)
							{
							case PT_PTCT: // PTCT and NTCT don't moving diagonal
								if (rx && ry) continue;
							case PT_PSCN:
								ri = floor((parts[i].temp - 268.15)/10); // How many tens of degrees above 0 C
								break;
							case PT_NTCT:
								if (rx && ry) continue;
							case PT_NSCN:
								ri = -floor((parts[i].temp - 268.15)/10);
								break;
							default:
								continue;
							}
							nx = x + rx, ny = y + ry;
							if ((pmap[ny][nx]&0xFF) == PT_FRME)
							{
								nx += rx, ny += ry;
							}
							int nx2 = nx, ny2 = ny;
							for (rrx = 0; sim->InBounds(nx, ny) && rrx < rtmp; rrx++, nx+=rx, ny+=ry) // fixed
							{
								rr = pmap[ny][nx];
								rt = rr & 0xFF;
								if (rt == PT_SPRK)
									rt = parts[rr>>8].ctype;
								if (rt && rt != PT_INWR && rt != PT_FILT && rt != PT_STOR && rt != PT_BIZR && rt != PT_BIZRG && rt != PT_BIZRS && rt != PT_GOO && rt != PT_BRAY)
									break;
								pmap[ny][nx] = 0; // clear pmap
								Element_PSTN::tempParts[rrx] = rr;
							}
							for (rry = 0; rry < rrx; rry++) // "rrx" in previous "for" loop
							{
								rr = Element_PSTN::tempParts[rry]; // moving like PSTN
								if (!rr)
									continue;
								rii = rry + ri;
								if (rii < 0 || rii >= rrx) // assembly: "cmp rii, rrx" then "jae/jb ..."
								{
									if (!(parts[i].tmp2 & 1))
									{
										sim->kill_part(rr >> 8);
										continue;
									}
									else if (rii < 0)
										rii += rrx;
									else
										rii -= rrx;
								}
								nx = nx2 + rii * rx; ny = ny2 + rii * ry;
								parts[rr>>8].x = nx; parts[rr>>8].y = ny;
								pmap[ny][nx] = rr;
							}
						}
					}
			return return_value;
		case 25: // arrow key detector
			rrx = Element_MULTIPP::Arrow_keys; // current state
			rry = (parts[i].flags >> 16); // previous state
			switch (rtmp)
			{
				case 0: break;
				case 1: rry = rrx & ~rry; break; // start pressing key
				case 2: rry &= ~rrx; break; // end pressing key
				default:
					return return_value;
			}
			parts[i].flags &= 0x0000FFFF;
			parts[i].flags |= (rrx << 16);
			for (rii = 0; rii < 4; rii++) // do 4 times
			{
				if (rry & (1 << rii)) // check direction
				{
					rx = tron_ry[rii];
					ry = tron_rx[rii];
					r = pmap[y+ry][x+rx]; // checking distance = 1
					if (!r)
					{
						rx *= 2; ry *= 2;
						r = pmap[y+ry][x+rx]; // checking distance = 2
					}
					if (!r)
						continue;
					if ((sim->elements[r & 0xFF].Properties&(PROP_CONDUCTS|PROP_INSULATED)) == PROP_CONDUCTS)
						conductTo (sim, r, x+rx, y+ry, parts);
				}
			}
			return return_value;
		case 26: // SWCH toggler
			parts[i].tmp &= ~0x100;
			for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry) && abs(rx)+abs(ry) < 4)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				pavg = sim->parts_avg(i,r>>8,PT_INSL);
				if (pavg != PT_INSL && pavg != PT_INDI)
				{
					rt = r & 0xFF;
					r >>= 8;
					if (rt == PT_SPRK)
					{
						rctype = parts[r].ctype;
						if (rtmp & 0x100 && rctype == PT_SWCH) // 由于 "&" 和 "==" 的优先级高于 "&&".
						{
							sim->part_change_type(r, x+rx, y+ry, PT_SWCH);
							parts[r].life = 9;
							parts[r].ctype = 0; // clear .ctype value
						}
						else if (parts[r].life == 3 && (rtmp & 1 || rctype != PT_SWCH)) // 由于 "&", "&&" 和 "!=" 的优先级高于 "||".
							parts[i].tmp |= 0x100;
					}
					else if (rt == PT_SWCH && rtmp & 0x100)
					{
						if (parts[r].life < 10)
							parts[r].life = (r > i ? 15 : 14);
						else
							parts[r].life = 9;
					}
				}
			}
			break;
		}
		break;
			
	case 19:
		parts[i].tmp2 = parts[i].tmp;
		if (parts[i].tmp)
			--parts[i].tmp;
		for (rx = -2; rx <= 2; rx++)
			for (ry = -2; ry <= 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if ((r & 0xFF) == PT_SPRK && parts[r>>8].ctype == PT_PSCN && parts[r>>8].life == 3)
					{
						parts[i].tmp = 9;
						return return_value;
					}
				}
		return return_value;
	case 20: // particle emitter
		{
		rctype = parts[i].ctype;
		int rctypeExtra = rctype >> 8;
		// int EMBR_modifier;
		rctype &= 0xFF;
		if (!rctype)
			return return_value;
		if (rtmp <= 0)
			rtmp = 3;
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y-ry][x-rx];
					if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						// if (sim->elements[parts[r>>8].ctype].Properties & PROP_INSULATED && rx && ry) // INWR, PTCT, NTCT, etc.
						//	continue;
						// EMBR_modifier = 0;
						if (rctype != PT_LIGH || parts[r>>8].ctype == PT_TESC || !(rand() & 15))
						{
							nx = x+rx; ny = y+ry;
							if (rctype == PT_EMBR) // use by EMBR (explosion spark) emitter
							{
								rr = pmap[ny][nx];
								if ((rr & 0xFF) != PT_GLAS)
									continue;
								ny += ry; nx += rx;
							}
							int np = sim->create_part(-1, nx, ny, rctype, rctypeExtra);
							if (np >= 0) {
								parts[np].vx = rtmp*rx; parts[np].vy = rtmp*ry;
								parts[np].dcolour = parts[i].dcolour;
								switch (rctype)
								{
								case PT_PHOT:
									if (np > i) parts[np].flags |= FLAG_SKIPMOVE; // like E189 (life = 11)
									parts[np].temp = parts[i].temp;
									break;
								case PT_LIGH:
									parts[np].tmp = atan2(-ry, (float)rx)/M_PI*180;
									break;
								case PT_EMBR:
									parts[np].temp = parts[rr>>8].temp; // set temperature to EMBR
									if (parts[rr>>8].life > 0)
										parts[np].life = parts[rr>>8].life;
									break;
								}
							}
						}
						// return return_value;
					}
				}
		}
		break;
	case 21:
	/* MERC/DEUT/YEST expander, or SPNG "water releaser",
	 *   or TRON detector.
	 * note: exclude POLC "replicating powder"
	 */
		{
			rndstore = rand();
			int trade = 5;
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
				{
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r & 0xFF) == PT_TRON && !(rx && ry)) // (!(rx && ry)) equivalent to (!rx || !ry)
						{
							rr = pmap[y-ry][x-rx];
							rt = rr & 0xFF;
							rr >>= 8;
							if (rt == ELEM_MULTIPP && parts[rr].life == 30)
							{
								if ((parts[rr].tmp >> 20) == 3)
								{
									parts[rr].ctype &= ~0x1F;
									parts[rr].ctype |= (parts[r>>8].tmp >> 11) & 0x1F;
								}
								else
									parts[rr].ctype = (parts[r>>8].tmp >> 7) & 0x1FF;
							}
						}
						else
						{
							if (!(rndstore&7))
							{
								switch (r & 0xFF)
								{
								case PT_MERC:
									parts[r>>8].tmp += parts[i].tmp;
									break;
								case PT_DEUT:
									parts[r>>8].life += parts[i].tmp;
									break;
								case PT_YEST:
									// rtmp = parts[i].tmp;
									if (rtmp > 0)
										parts[r>>8].temp = 303.0f + (rtmp > 28 ? 28 : (float)rtmp * 0.5f);
									else if (-rtmp > (rand()&31))
										sim->kill_part(r>>8);
									break;
								case PT_SPNG:
									if (parts[r>>8].life > 0)
									{
										rr = sim->create_part(-1, x-rx, y-ry, PT_WATR);
										if (rr >= 0)
											parts[r>>8].life --;
									}
									break;
								case PT_BTRY:
									// if (parts[r>>8].tmp > 0)
									{
										rr = pmap[y-ry][x-rx];
										rt = rr & 0xFF;
										if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_FRZW || rt == PT_WTRV)
										{
											rr >>= 8;
											if(!(rand()%3))
												sim->part_change_type(rr, x-rx, y-ry, PT_O2);
											else
												sim->part_change_type(rr, x-rx, y-ry, PT_H2);
											if (rt == PT_CBNW)
											{
												rrx = rand() % 5 - 2;
												rry = rand() % 5 - 2;
												sim->create_part(-1, x+rrx, y+rry, PT_CO2);
											}
											// parts[r>>8].tmp --;
										}
									}
									break;
								case PT_GOLD:
									rr = pmap[y-ry][x-rx];
									rt = rr & 0xFF;
									if (rt == PT_BMTL || rt == PT_BRMT)
									{
										sim->part_change_type(rr >> 8, x-rx, y-ry, rtmp >= 0 ? PT_IRON : PT_TUNG);
									}
									break;
								case PT_CAUS:
									rr = pmap[y-ry][x-rx];
									rt = rr & 0xFF;
									if (rt == PT_CLNE || rt == PT_PCLN && parts[rr>>8].life == 10)
										rt = parts[rr>>8].ctype;
									if (rt == PT_WATR) // if it's water or water generator
									{
										sim->part_change_type(r>>8, x+rx, y+ry, PT_ACID);
										parts[r>>8].life += 10;
									}
									break;
								case ELEM_MULTIPP:
									if (parts[r>>8].life == 38)
									{
										rctype = parts[r>>8].ctype;
										if (rctype == PT_RFRG || rctype == PT_RFGL) // ACID/CAUS + GAS --> 3 RFRG
										{
											rr = pmap[y-ry][x-rx];
											if ((rr&0xFF) == PT_CAUS || (rr&0xFF) == PT_ACID)
											{
												parts[r>>8].tmp2 += 8;
												sim->kill_part(rr>>8);
											}

											rr = pmap[y+2*ry][x+2*rx];
											if ((rr&0xFF) == PT_GAS && parts[r>>8].tmp2 >= 8)
											{
												parts[r>>8].tmp2 -= 8;
												parts[r>>8].tmp += 3;
												sim->kill_part(rr>>8);
											}
										}
									}
								}
							}
							if (!--trade)
							{
								trade = 5;
								rndstore = rand();
							}
							else
								rndstore >>= 3;
						}
					}
				}
		}
		break;
	case 24: // moving duplicator particle
		if (parts[i].flags & FLAG_SKIPMOVE) // if wait flag exist
			parts[i].flags &= ~FLAG_SKIPMOVE; // clear wait flag
		else if (BOUNDS_CHECK)
		{
			/* definition:
			 *   tmp = length, tmp2 = total distance
			 * first step: like DRAY action
			 */
			rctype = parts[i].ctype;
			rr   = parts[i].tmp2;
			rtmp = parts[i].tmp;
			rtmp = rtmp > rr ? rr : (rtmp <= 0 ? rr : rtmp);
			rx = tron_rx[(rctype>>2) & 3], ry = tron_ry[(rctype>>2) & 3];
			int x_src = x + rx, y_src = y + ry, rx_dest = rx * rr, ry_dest = ry * rr;
			// int x_copyTo, y_copyTo;

			rr = pmap[y_src][x_src]; // override "rr" variable
			while (sim->InBounds(x_src, y_src) && rtmp--)
			{
				r = pmap[y_src][x_src];
				if (r) // if particle exist
				{
					rt = r & 0xFF;
					nx = x_src + rx_dest;
					ny = y_src + ry_dest;
					if (!sim->InBounds(nx, ny))
						break;
					rii = sim->create_part(-1, nx, ny, (rt == PT_SPRK) ? PT_METL : rt); // spark hack
					if (rii >= 0)
					{
						if (rt == PT_SPRK)
							sim->part_change_type(rii, nx, ny, PT_SPRK); // restore type for spark hack
						parts[rii] = parts[r>>8]; // duplicating all properties?
						parts[rii].x = nx; // restore X coordinates
						parts[rii].y = ny; // restore Y coordinates
					}
				}
				x_src += rx, y_src += ry;
			}
			
			rx_dest = x + tron_rx[rctype & 3], ry_dest = y + tron_ry[rctype & 3]; // override 2 variables (variable renaming?)
			if (parts[i].tmp3)
			{
				if (!(--parts[i].tmp3))
				{
					sim->kill_part(i); break;
				}
			}
			else if ((rr&0xFF) == ELEM_MULTIPP && parts[rii = rr>>8].life == 16 && parts[rii].ctype == 11)
			{
				sim->kill_part(i); break;
			}
			if (!sim->InBounds(rx_dest, ry_dest) || pmap[ry_dest][rx_dest]) // if out of boundary
				sim->kill_part(i);
			else
			{
				sim->pmap[y][x] = 0; // what stacked particle?
				sim->pmap[ry_dest][rx_dest] = (i << 8) | ELEM_MULTIPP; // actual is particle's index shift left by 8 plus particle's type
				parts[i].x = rx_dest;
				parts[i].y = ry_dest;
			}
		}
		break;
	case 26: // button
		if (rtmp)
		{
			if (rtmp == 8)
			{
				for (rx = -1; rx <= 1; rx++)
					for (ry = -1; ry <= 1; ry++)
					{
						r = pmap[y+ry][x+rx];
						rt = r & 0xFF;
						if ((sim->elements[rt].Properties & (PROP_CONDUCTS|PROP_INSULATED)) == PROP_CONDUCTS)
						{
							conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
			}
			parts[i].tmp --;
		}
		break;
	case 28: // ARAY/BRAY reflector
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						parts[i].tmp ^= 1;
						return return_value;
					}
				}
		break;
	case 29: // TRON emitter
		for (rr = 0; rr < 4; rr++)
			if (BOUNDS_CHECK)
			{
				rx = tron_rx[rr];
				ry = tron_ry[rr];
				r = pmap[y-ry][x-rx];
				if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
				{
					r = sim->create_part (-1, x+rx, y+ry, PT_TRON);
					if (r >= 0) {
						rrx = parts[r].tmp;
						rrx &= ~0x1006A; // clear direction data and custom flags
						rrx |= rr << 5; // set direction data
						rrx |= ((rtmp & 1) << 1) | ((rtmp & 2) << 2) | ((rtmp & 4) << 14); // set custom flags
						if (r > i) rrx |= 0x04;
						parts[r].tmp = rrx;
					}
				}
			}
		break;
	case 30: // TRON filter
		if (rtmp & 0x04)
			rtmp &= ~0x04;
		else if (rtmp & 0x01)
		{
			rt = rtmp >> 20;
			rrx = parts[i].ctype;
			if (rt == 4) // TRON splitter
			{
				for (rr = 0; rr < 4; rr++)
				{
					r = pmap[y + tron_ry[rr]][x + tron_rx[rr]];
					if ((r & 0xFF) == ELEM_MULTIPP && parts[r >> 8].life == 3)
					{
						ri = r >> 8;
						parts[ri].tmp &= 0xE0000;
						parts[ri].tmp |= (rtmp & 0x1FF9F) | (rr << 5);
						if (ri > i)
							sim->parts[ri].tmp |= 0x04;
						parts[ri].tmp2 = parts[i].tmp2;
					}
				}
				parts[i].tmp = rtmp & 0x7E0000;
				break;
			}
			rr = (rtmp >> 5) & ((rtmp >> 19 & 1) - 1);
			int direction = (rr + (rtmp >> 17)) & 0x3;
			r = pmap[y + tron_ry[direction]][x + tron_rx[direction]];
			rii = parts[r >> 8].life;
			if ((r & 0xFF) == ELEM_MULTIPP && rii == 3)
			{
				ri = r >> 8;
				parts[ri].tmp &= 0xE0000;
				rctype = (rtmp >> 7) & 0x1FF;
				switch (rt & 7)
				{
					case 0: rctype  = rrx; break; // set colour
					case 1: rctype += rrx; break; // hue shift (add)
					case 2: rctype -= rrx; break; // hue shift (subtract)
					case 3: // if color is / isn't ... then pass through
						if ((((rctype >> 4) & 0x1F) == rrx) == ((rrx >> 5) & 1)) // rightmost 5 bits xnor 6th bit
							rtmp = 0;
					break;
				}
				parts[ri].tmp |= (rtmp & 0x1009F) | (((rctype % 368 + 368) % 368) << 7) | (direction << 5); // colour modulo 368, rather than 360
				if (ri > i)
					sim->parts[ri].tmp |= 0x04;
				parts[ri].tmp2 = parts[i].tmp2;
			}
			rtmp &= 0x7E0000;
		}
		parts[i].tmp = rtmp;
		break;
	case 31: // TRON delay
		if (rtmp & 0x04)
			rtmp &= ~0x04;
		else if (parts[i].tmp3)
			parts[i].tmp3--;
		else if (rtmp & 0x01)
		{
			rr = (rtmp >> 5) & ((rtmp >> 19 & 1) - 1);
			int direction = (rr + (rtmp >> 17)) & 0x3;
			r = pmap[y + tron_ry[direction]][x + tron_rx[direction]];
			rii = parts[r >> 8].life;
			if ((r & 0xFF) == ELEM_MULTIPP && (rii & ~0x1) == 2)
			{
				ri = r >> 8;
				parts[ri].tmp |= (rtmp & 0x1FF9F) | (direction << 5);
				if (ri > i)
					sim->parts[ri].tmp |= 0x04;
				parts[ri].tmp2 = parts[i].tmp2;
			}
			rtmp &= 0xE0000;
		}
		parts[i].tmp = rtmp;
		break;
	case 33: // Second Wi-Fi
		rr = (1 << (parts[i].ctype & 0x1F));
		rii = (parts[i].ctype >> 4) & 0xE;
		parts[i].tmp = (int)((parts[i].temp-73.15f)/100+1);
		if (parts[i].tmp>=CHANNELS) parts[i].tmp = CHANNELS-1;
		else if (parts[i].tmp<0) parts[i].tmp = 0;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					// wireless[][0] - whether channel is active on this frame
					// wireless[][1] - whether channel should be active on next frame
					if (sim->wireless2[parts[i].tmp][rii] & rr)
					{
						if (((r&0xFF)==PT_NSCN||(r&0xFF)==PT_PSCN||(r&0xFF)==PT_INWR) && sim->wireless2[parts[i].tmp][rii])
						{
							conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
					if ((r&0xFF)==PT_SPRK && parts[r>>8].ctype!=PT_NSCN && parts[r>>8].life>=3)
					{
						sim->wireless2[parts[i].tmp][rii+1] |= rr;
						sim->ISWIRE2 = 2;
					}
				}
		break;
	case 34: // Sub-frame filter incrementer
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					nx = x + rx; ny = y + ry;
					r = pmap[ny][nx];
					if (!r)
						continue;
					if ((r & 0xFF) == PT_FILT)
					{
						rr = parts[r>>8].ctype + parts[i].ctype;
						rr &= 0x1FFFFFFF;
						rr |= 0x20000000;
						while (BOUNDS_CHECK && (
							(r & 0xFF) == PT_FILT
						)) // check another FILT
						{
							parts[r>>8].ctype = rr;
							r = pmap[ny += ry][nx += rx];
						}
					}
				}
		break;
	case 35:
		nx = x; ny = y;
		rrx = parts[i].ctype;
		for (rr = 0; rr < 4; rr++)
			if (BOUNDS_CHECK)
			{
				rx = tron_rx[rr];
				ry = tron_ry[rr];
				r = pmap[y-ry][x-rx];
				if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
				{
					rry = parts[r>>8].ctype == PT_INST ? 1 : 0;
					docontinue = 1;
					do
					{
						nx += rx; ny += ry;
						if (!sim->InBounds(nx, ny))
							break;
						r = pmap[ny][nx];
						if (!r || (r&0xFF) == PT_INWR || (r&0xFF) == PT_SPRK && parts[r>>8].ctype == PT_INWR) // if it's empty or insulated wire
							continue;
						if ((sim->elements[r&0xFF].Properties2 & PROP_DRAWONCTYPE))
						{
							parts[r>>8].ctype = rrx;
						}
						else if ((r&0xFF) == ELEM_MULTIPP)
						{
							int p_life = parts[r>>8].life;
							if (p_life == 20 || p_life == 35 || p_life == 36)
							{
								parts[r>>8].ctype = rrx;
							}
						}
						else if ((r&0xFF) == PT_INSL || (r&0xFF) == PT_INDI)
							break;
						docontinue = rry;
					} while (docontinue);
				}
			}
		break;
	case 36:
		rctype = parts[i].ctype & 0xFF;
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (!rx != !ry))
				{
					r = pmap[y+ry][x+rx];
					if ((r & 0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						switch (parts[r>>8].ctype)
						{
							case PT_PSCN: sim->elements[rctype].Properties2 |=  PROP_DEBUG_USE_TMP2; break;
							case PT_NSCN: sim->elements[rctype].Properties2 &= ~PROP_DEBUG_USE_TMP2; break;
							case PT_INWR: sim->elements[rctype].Properties2 ^=  PROP_DEBUG_USE_TMP2; break;
						}
						return return_value;
					}
				}
		break;
	case 37: // Simulation of Langton's ant (turmite)
		// At a white square (NONE), turn 90° right ((tmp % 4) += 1), flip the color of the square, move forward one unit
		// At a black square (INWR), turn 90° left  ((tmp % 4) -= 1), flip the color of the square, move forward one unit
		// direction: 0 = right, 1 = down, 2 = left, 3 = up
		if (!(rtmp & 4)) // white square
		{
			rr = (rtmp + 1) & 3;
		}
		else // black square
		{
			ri = parts[i].tmp2;
			rr = (rtmp - (ri ? 0 : 1)) & 3;
		}
		rr |= rtmp & ~7;
		rx = x - tron_rx[rr]; ry = y - tron_ry[rr];
		if (sim->edgeMode == 2)
		{
			if (rx < CELL)
				rx += XRES - 2*CELL;
			if (rx >= XRES-CELL)
				rx -= XRES - 2*CELL;
			if (ry < CELL)
				ry += YRES - 2*CELL;
			if (ry >= YRES-CELL)
				ry -= YRES - 2*CELL;
		}
		
		pmap[y][x] = 0;
		if (!(rtmp & 4)) // black <-> white square
		{
			ri = sim->create_part(-1, x, y, PT_INWR);
			if (ri >= 0)
				parts[ri].dcolour = parts[i].ctype;
		}
		if (sim->IsWallBlocking(rx, ry, 0))
			goto kill1;
		else
		{
			r = pmap[ry][rx];
			if (r)
			{
				if ((r&0xFF) == PT_INWR || (r&0xFF) == PT_SPRK && parts[r>>8].ctype == PT_INWR)
					sim->kill_part(r>>8), rr |= 4;
				else
					goto kill1;
			}
		}
		parts[i].x = rx;
		parts[i].y = ry;
		
		parts[i].tmp = rr;
		pmap[ry][rx] = parts[i].type | (i<<8);
		break;
	kill1:
		sim->kill_part(i);
		return return_value;
	case 38: // particle transfer medium (diffusion)
		{
			static char k1[8][8] = {
				{0,1,2,2,1,3,3,3},
				{2,0,1,2,1,3,3,3},
				{1,2,0,2,1,3,3,3},
				{1,1,1,0,1,3,3,3},
				{2,2,2,2,0,3,3,3},
				{3,3,3,3,3,3,3,3},
				{3,3,3,3,3,3,3,3},
				{3,3,3,3,3,3,3,3}
			};
			rrx = parts[i].tmp2 & 0x7;
			rctype = parts[i].ctype;
			int rctypeExtra = rctype >> 8;
			rctype &= 0xFF;
			for (int trade = 0; trade < 4; trade++)
			{
				if (!(trade & 1)) rndstore = rand();
				rx = rndstore%3-1;
				rndstore >>= 3;
				ry = rndstore%3-1;
				rndstore >>= 3;
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
					{
						if (rtmp > 0 && rctype && rrx != 3)
						{
							ri = sim->create_part(-1, x+rx, y+ry, rctype); // acts like CLNE ?
							if (ri >= 0)
							{
								parts[ri].temp = parts[i].temp;
								rctype == PT_LAVA && (parts[ri].ctype = rctypeExtra);
								rtmp--;
							}
						}
						continue;
					}
					switch (r&0xFF)
					{
					case ELEM_MULTIPP:
						if (parts[r>>8].life==38)
						{
							rii = rctype | rctypeExtra<<8;
							if (!(parts[r>>8].ctype&0xFF) && rctype)
								parts[r>>8].ctype = rii;
							if (parts[r>>8].ctype == rii)
							{
								rry = parts[r>>8].tmp2 & 0x7;
								switch (k1[rrx][rry])
								{
									case 0: rii = (parts[r>>8].tmp - rtmp) >> 1; break;
									case 1: rii = -rtmp; break;
									case 2: rii = parts[r>>8].tmp; break;
									default: rii = 0;
								}
								rtmp += rii;
								parts[r>>8].tmp -= rii;
							}
						}
						break;
					case PT_PCLN:
					case PT_PBCN:
						if (parts[r>>8].life != 10)
							break;
					case PT_CLNE:
					case PT_BCLN:
						if (!rctype)
						{
							parts[i].ctype = rctype = parts[r>>8].ctype;
							if (rctype == PT_LAVA)
							{
								rctypeExtra = parts[r>>8].tmp;
								parts[i].ctype |= rctypeExtra << 8;
							}
						}
						if (rctype == parts[r>>8].ctype && (rctype != PT_LAVA || rctypeExtra == parts[r>>8].tmp))
							rtmp += 5;
						break;
					default:
						if (sim->elements[r&0xFF].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS))
						{
							if (!rctype)
							{
								parts[i].ctype = rctype = r&0xFF;
								if (rctype == PT_LAVA)
								{
									rctypeExtra = parts[r>>8].ctype;
									parts[i].ctype |= rctypeExtra << 8;
								}
							}
							if (rctype == (r&0xFF) && (rctype != PT_LAVA || rctypeExtra == parts[r>>8].ctype) && rrx != 4)
							{
								sim->kill_part(r>>8);
								rtmp++;
							}
						}
					}
				}
			}
			parts[i].tmp = rtmp;
		}
		break;
#if !defined(RENDERER) && defined(LUACONSOLE)
	default: // reserved by Lua
		if (lua_el_mode[parts[i].type] == 1)
			return_value = 0;
#endif
	}
		
	return return_value;
}
