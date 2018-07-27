#include "simulation/Elements.h"
#include "simulation/Air.h"
//#include "simulation/Gravity.h"
#include "simulation/MULTIPPE_Update.h" // link to Renderer
#include "common/tpt-math.h"
#include "simplugin.h"

#ifdef _MSC_VER
#define __builtin_ctz msvc_ctz
#define __builtin_clz msvc_clz
#endif

#define ID(r) part_ID(r)

Renderer * MULTIPPE_Update::ren_;

// 'UPDATE_FUNC_ARGS' definition: Simulation* sim, int i, int x, int y, int surround_space, int nt, Particle *parts, int pmap[YRES][XRES]
// FLAG_SKIPMOVE: not only implemented for PHOT

/* Returns true for particles that start a ray search ("dtec" mode)
 */
bool MULTIPPE_Update::isAcceptedConductor(Simulation* sim, int r)
{
	return (sim->elements[TYP(r)].Properties & (PROP_CONDUCTS | PROP_INSULATED)) == PROP_CONDUCTS;
}

bool MULTIPPE_Update::isAcceptedConductor_i(Simulation* sim, int r)
{
	return isAcceptedConductor(sim, r) && sim->parts[ID(r)].life == 0;
}

void MULTIPPE_Update::do_breakpoint()
{
#ifdef X86
#if defined(WIN) && !defined(__GNUC__)
	// not tested yet
#ifndef _WIN64
	__asm {
		pushfd
		or dword ptr [esp], 0x100
		popfd
	}
#else
	// 64-bit MSVC doesn't support inline assembly yet
#endif
#else
	__asm__ __volatile (
#ifdef _64BIT
		"pushf; or{q} {$0x100, (%%rsp)|QWORD PTR [rsp], 0x100}; popf"
#else
		"pushf; or{l} {$0x100, (%%esp)|DWORD PTR [esp], 0x100}; popf"
#endif
	:);	
#endif
#endif
}

int MULTIPPE_Update::update(UPDATE_FUNC_ARGS)
{
	int return_value = 1; // skip movement, 'stagnant' check, legacyUpdate, etc.
	static int tron_rx[4] = {-1, 0, 1, 0};
	static int tron_ry[4] = { 0,-1, 0, 1};
	static int osc_r1 [4] = { 1,-1, 2,-2};
	int rx, ry, rlife = parts[i].life, r, ri, rtmp, rctype;
	int rr, rndstore, rt, rii, rrx, rry, nx, ny, pavg, rrt;
	// int tmp_r;
	float rvx, rvy, rdif;
	int docontinue, rtmp2;
	rtmp = parts[i].tmp;

	static Particle * temp_part, * temp_part_2, * prev_temp_part;
	
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
							if(TYP(pmap[y+ry][x+rx]) == ELEM_MULTIPP)
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
	case 4: // photon laser [激光器]
		if (!rtmp)
			break;

		rvx = (float)(((rtmp ^ 0x08) & 0x0F) - 0x08);
		rvy = (float)((((rtmp >> 4) ^ 0x08) & 0x0F) - 0x08);
		rdif = (float)((((rtmp >> 8) ^ 0x80) & 0xFF) - 0x80);

		ri = sim->create_part(-3, x + (int)rvx, y + (int)rvy, PT_PHOT);
		if (ri < 0) break;
		if (ri > i) ELEMPROPW(ri, |=);
		parts[ri].vx = rvx * rdif / 16.0f;
		parts[ri].vy = rvy * rdif / 16.0f;
		rctype = parts[i].ctype;
		rtmp = rctype >> 30;
		rctype &= 0x3FFFFFFF;
		if (rctype)
			parts[ri].ctype = rctype;
		parts[ri].temp = parts[i].temp;
		parts[ri].life = parts[i].tmp2;
		parts[ri].tmp = rtmp & 3;
		
		break;
	case 2: // reserved for TRON
	case 3: // reserved
	case 5: // reserved for Simulation.cpp
	case 7: // reserved for Simulation.cpp
#ifdef NO_SPC_ELEM_EXPLODE
	case 8:
	case 9:
#endif
	case 13: // decoration only, no update function [静态装饰元素]
	case 15: // reserved for Simulation.cpp
	case 17: // reserved for 186.cpp and Simulation.cpp
	case 18: // decoration only, no update function
	case 23: // reserved for stickmen
	case 25: // reserved for E189's life = 16, ctype = 10.
	case 27: // reserved for stickmen
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
						if ((sim->elements[TYP(r)].HeatConduct > 0) && (TYP(r) != PT_HSWC || partsi(r).life == 10))
							partsi(r).temp = parts[i].temp;
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
		{
		bool transferdir = (parts[i].tmp2 & 1);
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
		if (TYP(r) == PT_PHOT || TYP(r) == PT_PROT)
		{
			parts[i].tmp += 2;
			if (partsi(r).temp > 370.0f)
				parts[i].tmp += (int)partsi(r).temp - 369;
			if (3 > (rndstore & 0xF))
				sim->kill_part(ID(r));
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
					if (sim->elements[TYP(r)].HeatConduct > 0)
					{
						int transfer = (int)(partsi(r).temp - 273.15f);
						parts[i].tmp += transfer;
						partsi(r).temp -= (float)transfer;
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
					bool not_self = !CHECK_EXTEL(r, 8);
					if (TYP(r) != PT_VIBR && TYP(r) != PT_BVBR && not_self)
						continue;
					if (not_self)
					{
						if (transferdir)
						{ // VIBR2 <- VIBR
							parts[i].tmp += partsi(r).tmp;
							partsi(r).tmp = 0;
						}
						else
						{ // VIBR2 -> VIBR
							partsi(r).tmp += parts[i].tmp;
							parts[i].tmp = 0;
						}
						break;
					}
					if (parts[i].tmp > partsi(r).tmp)
					{
						int transfer = (parts[i].tmp - partsi(r).tmp) >> 1;
						(transfer < 2) && (transfer = 1); // maybe CMOV instruction?
						partsi(r).tmp += transfer;
						parts[i].tmp -= transfer;
						break;
					}
				}
			}
		}
		if (parts[i].tmp < 0)
			parts[i].tmp = 0; // only preventing because negative tmp doesn't save
		}
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
						rt = TYP(r);
						if (!r || (ELEMPROP2(rt) & PROP_NODESTRUCT) || rt == PT_VIBR || rt == PT_BVBR || rt == PT_WARP || rt == PT_SPRK)
							continue;
						if (rt == ELEM_MULTIPP)
						{
							if (partsi(r).life == 8)
							{
								partsi(r).tmp += 1000;
								continue;
							}
							if (partsi(r).life == 9)
								continue;
						}
						if (!(rndstore & 0x7))
						{
							sim->part_change_type(ID(r), x+rx, y+ry, ELEM_MULTIPP);
							partsi(r).life = 8;
							partsi(r).tmp = 21000;
						}
						trade++; rndstore >>= 3;
					}
		}
		break;
#endif /* NO_SPC_ELEM_EXPLODE */
	case 10: // electronics debugger input [电子产品调试]
		{
		int funcid = rtmp & 0xFF, fcall = -1;
		if (rtmp & 0x100)
		{
			fcall = funcid;
		}
		else if (funcid >= 0x78)
			funcid -= 0x66;
		else if (funcid >= 0x12)
			return return_value;
		for (rx = -1; rx <= 1; rx++)
			for (ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (CHECK_EL_SPRKL3(r))
					{
						if (fcall >= 0)
						{
#if !defined(RENDERER) && defined(LUACONSOLE)
							LuaScriptInterface::simulation_debug_trigger::_main(fcall, i, x, y);
#endif
							continue;
						}
						static char shift1[7] = {8,9,1,3,11,4,5}; // 23,24,1,2,15,4,5
						switch (funcid & 0xFF)
						{
						case 0:
							if (Element_MULTIPP::maxPrior <= parts[i].ctype)
							{
								sim->SimExtraFunc |= 0x81;
								Element_MULTIPP::maxPrior = parts[i].ctype;
							}
							break;
						case 3: sim->SimExtraFunc &= ~0x08; break;
						case 6:
							switch (partsi(r).ctype)
							{
								case PT_NTCT: Element_PHOT::ignite_flammable = 0; break;
								case PT_PTCT: Element_PHOT::ignite_flammable = 1; break;
								default: sim->SimExtraFunc |= 0x40;
							}
							break;
						case 7:
						case 8:
							if (parts[i].temp < 273.15f)
								parts[i].temp = 273.15f;
							rdif = (int)(parts[i].temp - 272.65f);
							if (partsi(r).ctype != PT_INST)
								rdif /= 100.0f;
							sim->sim_max_pressure += (funcid & 1) ? rdif : -rdif;
							if (sim->sim_max_pressure > 256.0f)
								sim->sim_max_pressure = 256.0f;
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
							if (partsi(r).ctype == PT_INST)
								sim->elementRecount = true;
							break;
						case 11:
							rctype = partsi(r).ctype;
							rr = pmap[y-ry][x-rx];
							if (rr)
							{
								rrt = TYP(partsi(rr).ctype);
								int tFlag = 0;
								switch (TYP(rr))
								{
									case PT_STOR: tFlag = PROP_CTYPE_INTG; break;
									case PT_CRAY: tFlag = PROP_CTYPE_WAVEL; break;
									case PT_DRAY: tFlag = PROP_CTYPE_SPEC; break;
								}
								switch (rctype)
								{
									case PT_PSCN: ELEMPROP2(rrt) |=  tFlag; break;
									case PT_NSCN: ELEMPROP2(rrt) &= ~tFlag; break;
									case PT_INWR: ELEMPROP2(rrt) ^=  tFlag; break;
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
						case 13: // heal/harm stickmen lifes
							{
								int lifeincx = parts[i].ctype;
								rctype = partsi(r).ctype;
								(rctype == PT_NSCN) && (lifeincx = -lifeincx);
								sim->SimExtraFunc |= 0x1000;
								pavg = (Element_STKM::phase + 2) % 4;
								Element_STKM::lifeinc[pavg] |= 2 | (rctype == PT_INST);
								Element_STKM::lifeinc[pavg + 1] += lifeincx;
							}
							break;
						case 14: // set stickman's element power
							rctype = partsi(r).ctype;
							rii = parts[i].ctype;
							if (rctype == PT_METL || rctype == PT_INWR)
							{
								if (sim->player2.spwn)
								{
									if (sim->player.spwn)
									{
										rrx = (sim->player2.rocketBoots ? 4 : 0) + (sim->player.rocketBoots ? 2 : 0);
										sim->player.rocketBoots  = (rii >> (rrx++)) & 1;
									}
									else
										rrx = (sim->player2.rocketBoots ? 11 : 10);
									sim->player2.rocketBoots = (rii >> rrx) & 1;
								}
								else if (sim->player.spwn)
								{
									sim->player.rocketBoots = (rii >> (sim->player.rocketBoots ? 9 : 8)) & 1;
								}
							}
							else
							{
								if (!rii)
								{
									int fr = pmap[y-ry][x-rx];
									int frt = TYP(fr);
									if (frt == PT_CRAY)
									{
										rii = TYP(parts[ID(fr)].ctype);
										if (rii == PT_LIFE)
											rii = PMAP(1, parts[ID(fr)].ctype);
									}
									else if (frt == PT_FILT)
										rii = PMAP(1, Element_FILT::getWavelengths(&parts[ID(fr)]));
								}
								if (!sim->IsValidElement(rii) && (rii < PMAPID(1) || rii > PMAP(1, 5)))
									break;
								if (rctype == PT_PSCN || rctype == PT_INST)
									Element_STKM::STKM_set_element_Ex(sim, &sim->player, rii | PMAPID(2));
								if (rctype == PT_NSCN || rctype == PT_INST)
									Element_STKM::STKM_set_element_Ex(sim, &sim->player2, rii | PMAPID(2));
							}
							break;
						case 15:
							sim->extraDelay += parts[i].ctype;
						case 1: case 2: case 4: case 5: case 23: case 24:
							sim->SimExtraFunc |= 1 << shift1[(funcid + 1) % 12]; break;
						case 16:
#if defined(LUACONSOLE) && defined(TPT_NEED_DLL_PLUGIN)
							__asm__ __volatile__ ("lock or%z0 %1, %0" :: "m"(Element_NEUT::cooldown_counter_b[2]), "i"(1));
#else
							Element_NEUT::cooldown_counter_b[2] |= 1;
#endif
							break;
						case 25:
							do_breakpoint();
							return return_value;
						}
						if ((rtmp & 0x3FE) == 0x200 && (rx != ry))
							MULTIPPE_Update::InsertText(sim, i, x, y, -rx, -ry);
					}
				}
		}
		break;
	case 11: // photons emitter [单光子发射器]
		for (rx = -1; rx <= 1; rx++)
			for (ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (CHECK_EL_SPRKL3(r))
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
	case 12: // SPRK reflector [电脉冲反射器]
		if ((rtmp & 0x7) != 0x4)
		{
			for (rx = -1; rx <= 1; rx++)
				for (ry = -1; ry <= 1; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						rt = TYP(r);
						rtmp = parts[i].tmp;
						if (!r)
							continue;
						if (rt == PT_SPRK && ( !(rtmp & 8) == !(ELEMPROPP(r, ctype) & PROP_INSULATED) ) && partsi(r).life == 3)
						{
							switch (rtmp & 0x7)
							{
							case 0: case 1:
								parts[i].tmp ^= 1; break;
							case 2:
								rr = pmap[y-ry][x-rx];
								switch (TYP(rr))
								{
								case ELEM_MULTIPP:
									if (partsi(rr).life == 12)
										sim->kill_part(ID(rr));
									else if (partsi(rr).life == 15)
									{
										rrt = partsi(rr).tmp;
										if (rt == PT_NSCN)
											rrt = 30-rrt;
										if (rrt <  0) rrt =  0;
										if (rrt > 30) rrt = 30;
										rr = pmap[y-2*ry][x-2*rx];
										if (TYP(rr) == PT_FILT)
										{
											rctype = partsi(rr).ctype;
											partsi(rr).ctype = (rctype << rrt | rctype >> (30-rrt)) & 0x3FFFFFFF;
										}
									}
									break;
								case PT_NONE:
									ri = sim->create_part(-1,x-rx,y-ry,ELEM_MULTIPP,12);
									if (ri >= 0)
										parts[ri].tmp = 3;
									break;
								case PT_FILT: // might to making FILT oscillator
									{
										int * ctype_ptr = &partsi(rr).ctype;
										if (parts[i].temp > 373.0f)
										{
											if (*ctype_ptr == 1)
												{ parts[i].temp = 295.15f; continue; }
											*ctype_ptr >>= 1;
										}
										else
										{
											if (*ctype_ptr == 0x20000000)
												{ parts[i].temp = 395.15f; continue; }
											*ctype_ptr <<= 1;
											*ctype_ptr &= 0x3FFFFFFF;
										}
									}
									break;
								}
								break;
							case 5:
								{
									int rlen = ((int)parts[i].temp - 258) / 10;
									ri = rlen;
									if (ri <= 0) ri = 1;
									int rix = -ri*rx, riy = -ri*ry;
									if (rtmp & 0x10)
										nx = x - rx, ny = y - ry;
									else
										nx = x + rix, ny = y + riy;
									if (sim->InBounds(nx, ny))
									{
										rr = pmap[ny][nx];
										if (TYP(rr) == PT_LDTC)
										{
											nx -= rx, ny -= ry;
											if (sim->InBounds(nx, ny))
												rr = pmap[ny][nx];
										}
										if (TYP(rr) != PT_FILT)
											break;
										rctype = partsi(rr).ctype;
										if (rtmp & 0x10)
											nx += rix, ny += riy;
										if (rlen <= 0)
											while (sim->InBounds(nx, ny))
											{
												rr = pmap[ny][nx];
												if (rr) break;
												nx -= rx, ny -= ry;
											}
										Element_MULTIPP::setFilter(sim, nx, ny, -rx, -ry, rctype);
									}
								}
								break;
							}
						}
						if ((rtmp & 0x5) == 0x1 && isAcceptedConductor(sim, r))
							conductTo (sim, r, x+rx, y+ry, parts);
					}
		}
		break;
	case 14: // dynamic decoration (DECO2) [动态装饰元素]
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
	case 16: // [电子产品大集合]
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
								if (TYP(r) == PT_SPRK && partsi(r).life == 3)
								{
									if (partsi(r).ctype == PT_PSCN)
									{
										if (partsi(r).tmp & 1)
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
								else if (TYP(r) == PT_NSCN && conductive)
									conductTo (sim, r, x+rx, y+ry, parts);
							}
				}
			}
			break;
		case 1: // conduct->insulate counter
			if (parts[i].tmp)
			{
				if (ELEMPROPW(i, &)) // if wait flag exist
				{
					ELEMPROPW(i, &=~); // clear wait flag
					return return_value;
				}
				if (parts[i].tmp2)
				{
					for (rx=-2; rx<3; rx++)
						for (ry=-2; ry<3; ry++)
							if (BOUNDS_CHECK && (rx || ry))
							{
								r = pmap[y+ry][x+rx];
								if (TYP(r) == PT_NSCN)
									conductTo (sim, r, x+rx, y+ry, parts);
							}
					parts[i].tmp--;
					parts[i].tmp2 = 0;
				}
				for (rx=-2; rx<3; rx++)
					for (ry=-2; ry<3; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (CHECK_EL_SPRKL3T(r, PT_PSCN))
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
						if (CHECK_EL_SPRKL3T(r, PT_PSCN))
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
							if (TYP(r) == PT_NSCN)
								conductTo (sim, r, x+rx, y+ry, parts);
						}
				parts[i].tmp = 0;
			}
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (CHECK_EL_SPRKL3T(r, PT_PSCN))
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
						if (CHECK_EXTEL(r, 16))
							r = pmap[y+2*ry][x+2*rx];
						if (TYP(r) == PT_SPRK /* && partsi(r).life == 3 */)
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
					if ((TYP(rr) == PT_VIRS || TYP(rr) == PT_VRSS || TYP(rr) == PT_VRSG) && !partsi(rr).pavg[0]) // if is virus
					{
						// VIRS.cpp: .pavg[0] measures how many frames until it is cured (0 if still actively spreading and not being cured)
						(rtmp <= 0) && (rtmp = 15);
						partsi(rr).pavg[0] = (float)rtmp;
					}
				}
			}
			break;
		case 5:
			if (parts[i].tmp2 < 2)
			{
				for (rx = -1; rx < 2; rx++)
					for (ry = -1; ry < 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (CHECK_EL_SPRKL3(r) || !(rx && ry) && sim->emap[(y+ry)/CELL][(x+rx)/CELL] == 15)
							{
								parts[i].tmp2 = 10;
								goto break2b;
							}
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
							switch (TYP(r))
							{
								case PT_NONE:  continue;
								case PT_STKM:  r = sim->player.underp; break;
								case PT_STKM2: r = sim->player2.underp; break;
								case PT_FIGH:
									rii = partsi(r).tmp;
									if (rii < 0 || rii >= MAX_FIGHTERS) continue;
									r = sim->fighters[partsi(r).tmp].underp;
								break;
								// case PT_PINVIS: r = partsi(r).tmp4; break;
							}
							rii = partsi(r).tmp2;
							if (rii & ID(r)>i) // If the other particle hasn't been life updated
								rii--;
							if (TYP(r) == ELEM_MULTIPP && partsi(r).life == 16 && partsi(r).ctype == 5 && !rii)
							{
								partsi(r).tmp2 = ID(r) > i ? 10 : 9;
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
									if (ELEMPROPT(rx)&PROP_CONDUCTS)
										conductTo(sim, rx, x+r, y, parts);
								}
								if (rtmp & 2 >> (rctype & 1))
								{
									ry = pmap[y+r][x];
									if (ELEMPROPT(ry)&PROP_CONDUCTS)
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
						if (CHECK_EL_SPRKL3(rx)) rr |= 1;
						if (CHECK_EL_SPRKL3(ry)) rr |= 2;
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
						if (CHECK_EL_SPRKL3(r))
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
					switch (TYP(r))
					{
					case PT_SPRK:
						if (!rt && partsi(r).ctype == PT_SWCH)
						{
							partsi(r).life = 9;
							partsi(r).ctype = PT_NONE;
							sim->part_change_type(ID(r), rx, ry, PT_SWCH);
						}
						break;
					case PT_SWCH:
					case PT_HSWC:
						rtmp = partsi(r).life;
						if (rt)
							partsi(r).life = 10;
						else if (rtmp >= 10)
							partsi(r).life = 9;
						break;
					case PT_LCRY:
						rtmp = partsi(r).tmp;
						if (rt && rtmp == 0)
							partsi(r).tmp = 2;
						if (!rt && rtmp == 3)
							partsi(r).tmp = 1;
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
					if (TYP(r) == PT_SPRK && partsi(r).life == 3)
					{
						int direction = rr;
						for (rr = 0; rr < 4; rr++) // reset "rr" variable
							if (BOUNDS_CHECK)
							{
								rrx = tron_rx[rr];
								rry = tron_ry[rr];
								ri = pmap[y+rry][x+rrx];
								if (CHECK_EXTEL(ri, 17)) // using "PHOT diode"
								{
									rii = sim->create_part(-1, x-rx, y-ry, ELEM_MULTIPP, 24);
									rtmp = (direction << 2) | rr;
									if (rii >= 0)
									{
										parts[rii].ctype = rtmp;
										parts[rii].tmp = partsi(ri).tmp;
										parts[rii].tmp2 = partsi(ri).tmp2;
										parts[rii].tmp3 = parts[i].tmp;
										if (rii > i) ELEMPROPW(rii, |=); // set wait flag
									}
									r = pmap[y-rry][x-rrx]; // variable "r" value override
									if (CHECK_EXTEL(r, 25))
									{
										rii = sim->create_part(-1, x-rx-rrx, y-ry-rry, ELEM_MULTIPP, 24);
										if (rii >= 0)
										{
											parts[rii].ctype = rtmp ^ 2;
											parts[rii].tmp = partsi(ri).tmp;
											parts[rii].tmp2 = partsi(ri).tmp2;
											parts[rii].tmp3 = partsi(r).tmp; // fixed overflow?
											if (rii > i) ELEMPROPW(rii, |=); // set wait flag
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
							if (TYP(rry) == PT_NSCN && (rtmp & 1))
								conductTo (sim, rry, x, y+ry*rii, parts);
							if (TYP(rrx) == PT_NSCN && (rtmp & 2))
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
						if (TYP(rx) == PT_SPRK && partsi(rx).life == 3 && partsi(rx).ctype == PT_PSCN) rr |= 1;
						if (TYP(ry) == PT_SPRK && partsi(ry).life == 3 && partsi(ry).ctype == PT_PSCN) rr |= 2;
					}
				}
				if (rr && !((rctype & 1) && parts[i].tmp2))
				{
					parts[i].tmp = rr; parts[i].tmp2 = 1;
				}
			}
			break;
		case 13:
			rii = parts[i].tmp2;
			parts[i].tmp2 = 0;
			for (rx = -2; rx <= 2; rx++)
				for (ry = -2; ry <= 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (TYP(r) == PT_SPRK)
						{
							if (partsi(r).ctype == PT_NSCN)
								partsi(r).tmp ^= (rii & 1);
							if (partsi(r).ctype == PT_PSCN && partsi(r).life == 3)
								parts[i].tmp2 |= 1;
						}
						else if (rii && TYP(r) == PT_NSCN)
						{
							partsi(r).tmp ^= 1;
							if (!partsi(r).tmp)
								conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
			break;
		case 14: // fast laser?
			rx = tron_rx[rtmp & 3];
			ry = tron_ry[rtmp & 3];
			ri = sim->create_part(-1, x + rx, y + ry, PT_E195);
			parts[ri].ctype = PMAP(1, 5);
			rtmp >>= 2;
			parts[ri].vx = rx * rtmp;
			parts[ri].vy = ry * rtmp;
			if (ri > i) ELEMPROPW(ri, |=);
			break;
		case 15: // capacitor
			rii = parts[i].tmp2;
			rii && (parts[i].tmp2--);
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (CHECK_EL_SPRKL3T(r, PT_PSCN))
						{
							parts[i].tmp2 = parts[i].tmp;
						}
						else if (rii && (TYP(r) == PT_NSCN))
							conductTo (sim, r, x+rx, y+ry, parts);
					}
			break;
		case 16:
			if (parts[i].tmp2)
			{
				for (rx = -2; rx <= 2; rx++)
					for (ry = -2; ry <= 2; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (TYP(r) == PT_PSNS || TYP(r) == PT_TSNS)
								partsi(r).tmp ^= 2;
							else if (TYP(r) == PT_DTEC || TYP(r) == PT_LSNS)
								partsi(r).tmp3 ^= 1;
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
							int nrx, nry, brx, bry, wrx, wry;
							if (!r)
								continue;
							switch (TYP(r))
							{
							case PT_FILT:
								rrx = partsi(r).ctype;
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
								Element_MULTIPP::setFilter(sim, nx, ny, rx, ry, rrx);
								break;
							case PT_CRAY:
								{
									int ncount = 0;
									docontinue = (rtmp == PT_INST) ? 2 : 1;
									rrx = (rtmp == PT_PSCN) ? 1 : 0;
									if (rtmp != PT_NSCN && rtmp != PT_INST && !rrx)
										continue;
									rry = 0;
									prev_temp_part = &partsi(r);
									rrt = ((int)prev_temp_part->temp - 268) / 10;
									if (rrt < 0) rrt = 0;
									temp_part = NULL;
									if (partsi(r).dcolour == 0xFF000000) // if black deco is on
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
											if (docontinue == 1)
											{
												ri = sim->create_part(-1, nx, ny, PT_INWR);
												if (ri >= 0)
													parts[ri].dcolour = rry;
												docontinue = !rrx;
											}
											else
												docontinue = 0;
											continue;
										}
										switch (TYP(r))
										{
										case PT_INWR:
											if (docontinue == 1)
											{
												sim->kill_part(ID(r));
												docontinue = rrx;
											}
											else
												docontinue = 0;
											continue;
										case PT_FILT:
											if (partsi(r).tmp == 0) // if mode is "set colour"
												rry = partsi(r).dcolour;
											break;
										case PT_BRCK:
											docontinue = partsi(r).tmp;
											partsi(r).tmp = 1;
											continue;
										case ELEM_MULTIPP:
											switch (partsi(r).life)
											{
											case 3:
												r = pmap[ny+ry][nx+rx];
												if (TYP(r) == PT_METL || TYP(r) == PT_INDC)
													conductTo (sim, r, nx+rx, ny+ry, parts);
												while (--ncount)
												{
													nx -= rx; ny -= ry;
													rr = pmap[ny][nx];
													if (TYP(rr) == PT_BRCK)
														partsi(rr).tmp = 0;
												}
												break;
											case 4:
												partsi(r).tmp = (rx & 15) | ((ry & 15) << 4) | 0x3000;
												break;
											case 12:
												if (rrx && !(partsi(r).tmp & 6))
													partsi(r).tmp |= 2;
												break;
											case 39:
												if (rtmp != PT_PSCN && temp_part == NULL)
												{
													partsi(r).life = 0;
													sim->part_change_type(ID(r), nx, ny, TYP(partsi(r).ctype));
													if (rtmp == PT_INST && partsi(r).type == PT_QRTZ)
														temp_part = &partsi(r);
												}
												docontinue = 2;
												continue;
											}
											goto break1d;
										case PT_QRTZ:
											if (rtmp == PT_PSCN)
											{
												sim->part_change_type(ID(r), nx, ny, ELEM_MULTIPP);
												partsi(r).life = 39;
												partsi(r).ctype = PMAP(rrt, PT_QRTZ);
											}
											else if (rtmp == PT_INST && temp_part != NULL)
											{
												if (temp_part->tmp >= 0 && partsi(r).tmp >= 0)
												{
													partsi(r).tmp += temp_part->tmp;
													temp_part->tmp = 0;
													goto break1d;
												}
											}
											docontinue = 2;
											break;
										case PT_FRAY:
											rii = partsi(r).tmp;
											rrt++;
											rr = pmap[ny+ry*rrt][nx+rx*rrt];
											if (TYP(rr) == PT_CRAY || TYP(rr) == PT_DRAY || TYP(rr) == PT_PSTN)
											{
												if (prev_temp_part->ctype == PT_ACEL)
													partsi(rr).tmp = rii;
												else if (prev_temp_part->ctype == PT_DCEL)
													partsi(rr).tmp2 = rii;
											}
											goto break1d;
										case PT_RPEL:
											for (pavg = 1; pavg >= -2; pavg -= 2)
											{
												static int * r1, * r2;
												r1 = &pmap[ny+rx*pavg][nx-ry*pavg];
												r2 = &pmap[ny+rx*pavg*2][nx-ry*pavg*2];
												if (TYP(*r1) == PT_BTRY && !(*r2))
												{
													partsi(*r1).x -= ry*pavg;
													partsi(*r1).y += rx*pavg;
													*r2 = *r1;
													*r1 = 0;
												}
											}
											goto break1d;
										default:
											docontinue = 0;
										}
									}
								}
								break;
							case ELEM_MULTIPP:
								if (partsi(r).life == 28 && (partsi(r).tmp & 0x1C) == 0x4 && !(rx && ry))
								{
									int dir = partsi(r).tmp;
									int rxi = 0, ryi = 0;
									if (dir & 2)
										rxi = (dir == 7 ? -1 : 1);
									else
										ryi = (dir == 4 ? -1 : 1);
									rrx = nx + rxi, rry = ny + ryi;
									temp_part = NULL;
									while (sim->InBounds(rrx, rry))
									{
										rii = pmap[rry][rrx];
										if (TYP(rii) == PT_BRAY)
											sim->kill_part(ID(rii)), rii = 0;
										if (!rii)
										{
										continue2a:
											rrx += rxi, rry += ryi;
											continue;
										}
										if (temp_part)
										{
											nrx = rrx - rxi;
											nry = rry - ryi;
										}
										else if (TYP(rii) == ELEM_MULTIPP && ((pavg = partsi(rii).life) == 28 || pavg == 32))
										{
											// use "rtmp"
											temp_part = &partsi(rii);
											nrx = brx = rrx;
											nry = bry = rry;
											if (rtmp == PT_PSCN) nrx += rxi, nry += ryi;
											else if (rtmp == PT_NSCN) nrx -= rxi, nry -= ryi;
											else break;
											rr = pmap[nry][nrx];
											if (TYP(rr) == PT_BRAY)
												sim->kill_part(ID(rr));
											else if (rr)
											{
												if (rtmp == PT_PSCN) nrx = nx + rxi, nry = ny + ryi;
												else goto continue2a;
											}
										}
										else break;
										temp_part->x = (float)nrx;
										temp_part->y = (float)nry;
										pmap[bry][brx] = 0;
										pmap[nry][nrx] = rii;
										break;
									}
								}
								else if (partsi(r).life == 16)
								{
									if (partsi(r).ctype == 5 && partsi(r).tmp >= 0x40)
									{
										rrt = partsi(r).tmp;
										partsi(r).tmp = ((rrt - 0x40) ^ 0x40) + 0x40;
									}
									else if (partsi(r).ctype == 8)
									{
										for (int j = 1; j <= 2; j++)
											BreakWallTest(sim, nx + j*rx, ny + j*ry, true);
									}
								}
								break;
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
							switch (TYP(r))
							{
							case PT_FILT:
								rrx = partsi(r).ctype;
								rry = parts[i].tmp;
								switch (rry) // rry = sender type
								{
								// rrx = wavelengths
								case PT_PSCN:
									rrx &= ~0x1;
									// no break
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
									partsi(r).ctype = rry ^ (rry | (rry << 2));
									goto continue1b;
								}
								rrx &= 0x1FFFFFFF;
								rrx |= 0x20000000;
							continue1b:
								Element_MULTIPP::setFilter(sim, nx, ny, rx, ry, rrx);
								break;
							case PT_PUMP: case PT_GPMP:
								rrx = TYP(r);
								rdif = (parts[i].tmp == PT_PSCN) ? 1.0f : -1.0f;
								while (BOUNDS_CHECK && TYP(r) == rrx) // check another pumps
								{
									partsi(r).temp += rdif;
									r = pmap[ny += ry][nx += rx];
								}
								break;
							case PT_FRAY:
								rrx = TYP(r);
								if (TYP(rtmp) == PT_BRAY)
								{
									rii = floor((parts[i].temp - 268.15) / 10);
									if (ID(rtmp) & 0x1)
										rii = -rii;
								}
								else
									rii = (parts[i].tmp == PT_PSCN) ? 1 : -1;
								partsi(r).tmp += rii;
								if (partsi(r).tmp < 0)
									partsi(r).tmp = 0;
								break;
							case PT_C5:
								r = pmap[ny += ry][nx += rx];
								if ((TYP(r) == PT_VIBR || TYP(r) == PT_BVBR) && partsi(r).life)
								{
									partsi(r).tmp2 = 1; partsi(r).tmp = 0;
								}
								else 
								{
									while (BOUNDS_CHECK && ((rt = TYP(r)) == PT_PRTI || rt == PT_PRTO))
									{
										if (rt == PT_PRTO)
											rt = PT_PRTI;
										else
											rt = PT_PRTO;
										sim->part_change_type(ID(r), nx, ny, rt);
										r = pmap[ny += ry][nx += rx];
									}
								}
								break;
							case PT_WIFI:
								{ // for 29-bit FILT data
									rii = (int)((partsi(r).temp-73.15f)/100+1);
									if (rii < 0) rii = 0;
									r = pmap[ny += ry][nx += rx];
									sim->ISWIRE = 2;
									continue1c:
									if (TYP(r) == PT_FILT)
									{
										rry = rii;
										rrx = partsi(r).ctype & 0x1FFFFFFF;
										for (; rrx && rry < CHANNELS; rry++)
										{
											if (rrx & 1)
												sim->wireless[rry][1] = 1;
											rrx >>= 1;
										}
										r = pmap[ny += ry][nx += rx];
										rii += 29;
										if (BOUNDS_CHECK && rii < CHANNELS)
											goto continue1c;
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
						pavg = PAVG_INSL(r);
						if (!CHECK_EL_INSL(pavg) && CHECK_EL_SPRKL3(r))
						{
							parts[i].tmp = partsi(r).ctype;
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
						pavg = PAVG_INSL(r);
						if (CHECK_EL_INSL(pavg))
							continue;
						if (TYP(r) == PT_SPRK && partsi(r).life == 3 && !rii)
						{
							parts[i].tmp2 = 2;
							// return return_value;
						}
						else if (TYP(r) == PT_WIRE)
						{
							if (!rii && partsi(r).tmp == 1)
							{
								parts[i].tmp2 = 2;
							}
							else if (!partsi(r).tmp && rii == 2)
							{
								partsi(r).ctype = 1;
								if (ID(r) > i) ELEMPROPW(ID(r), |=);
							}
						}
						else if (rii == 2)
						{
							if (TYP(r) == PT_INST)
								sim->FloodINST(x+rx,y+ry,PT_SPRK,PT_INST);
							else if (TYP(r) == PT_SWCH)
								conductToSWCH (sim, r, x+rx, y+ry, parts);
							else if (sim->elements[TYP(r)].Properties & PROP_CONDUCTS)
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
							if (!r) continue;
							if (ELEMPROPT(r) & PROP_CONDUCTS)
								conductTo (sim, r, x+rx, y+ry, parts);
							else if (TYP(r) == PT_FUSE && partsi(r).life == 40)
							{
								rr = pmap[y+2*ry][x+2*rx];
								if (ELEMPROPT(rr) & PROP_CONDUCTS)
								{
									partsi(rr).ctype = TYP(rr);
									sim->part_change_type(ID(rr), x+2*rx, y+2*ry, PT_SPRK);
									partsi(rr).life = partsi(r).tmp - 40;
								}
							}
							else if (TYP(r) == PT_WIRE)
							{
								partsi(r).ctype = 1;
								if (ID(r) > i) ELEMPROPW(ID(r), |=);
							}
						}
				parts[i].tmp2 = parts[i].tmp - 1;
			}
			
			break;
		case 21: // subframe SPRK generator/canceller
			rrx = (parts[i].temp < 373.0f) ? 4 : (parts[i].temp < 523.0f) ? 3 : 2;
			rry = (parts[i].temp < 273.15f);
			for (rx = -1; rx < 2; rx++)
				for (ry = -1; ry < 2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (TYP(r) == PT_SPRK)
						{
							if (rry)
							{
								rii = partsi(r).ctype;
								if (rii > 0 && rii < PT_NUM && sim->elements[rii].Enabled)
								{
									sim->part_change_type(ID(r), x+rx, y+ry, rii);
									partsi(r).life = (rii == PT_SWCH) ? 10 : 0; // keep SWCH on
									partsi(r).ctype = PT_NONE;
								}
							}
							else
								partsi(r).life = rrx;
						}
						else if (ELEMPROPT(r) & PROP_CONDUCTS || TYP(r) == PT_INST)
						{
							if (rry)
								partsi(r).life = 0;
							else
							{
								partsi(r).ctype = TYP(r);
								partsi(r).life = rrx;
								sim->part_change_type(ID(r), x+rx, y+ry, PT_SPRK);
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
							switch (TYP(r))
							{
							case PT_FILT:
								rrx = partsi(r).ctype;
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
								sim->grule[NGT_CUSTOM+1][9] += std::max(0, partsi(r).life);
								break;
							case PT_DCEL:
								rrx = sim->grule[NGT_CUSTOM+1][9] - std::max(0, partsi(r).life);
								sim->grule[NGT_CUSTOM+1][9] = (rrx < 2 ? 2 : rrx);
								break;
							case PT_FRAY:
								sim->grule[NGT_CUSTOM+1][9] = std::max(0, partsi(r).tmp) + 2;
								break;
							case PT_DTEC: // store GOL rule to sim particles
								{
									rrx = partsi(r).ctype;
									int c_gol = 0;
									if (rrx == PT_LIFE)
									{
										rii = partsi(r).tmp;
										if (rii >= 0 && rii < NGOL)
											c_gol = rii + 1;
									}
									rry = parts[i].tmp;
									int frr = pmap[ny+=ry][nx+=rx]; // front particle "r"
									switch (TYP(frr)) // check front particle type
									{
									case PT_FILT:
										{
											rctype = parts[ID(frr)].ctype;
											rctype &= 0x3FFFFE00; // 0x3FFFFE00 == (0x3FFFFFFF & ~0x000001FF)
											int ipos = 0x1;
											int imask = (rry == PT_PSCN ? 0x1 : 0x2);
											for (int _it = 0; _it < 9; _it++)
											{
												rctype |= ((sim->grule[c_gol][_it] & imask) ? ipos : 0);
												ipos <<= 1;
											}
											parts[ID(frr)].ctype = rctype | 0x20000000;
										}
										break;
									case PT_FRAY:
										parts[ID(frr)].tmp = sim->grule[c_gol][9] - 2;
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
				pavg = PAVG_INSL(r);
				if (!CHECK_EL_INSL(pavg))
				{
					rt = TYP(r);
					if (rt == PT_SPRK && partsi(r).life == 3)
					{
						switch (partsi(r).ctype)
						{
							case PT_NSCN: parts[i].tmp = 0; break;
							case PT_PSCN: parts[i].tmp = 1; break;
							case PT_INST: parts[i].tmp = !parts[i].tmp; break;
						}
					}
					else if (rtmp && rt != PT_PSCN && rt != PT_NSCN && isAcceptedConductor(sim, r))
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
						if (CHECK_EL_SPRKL3(r))
						{
							rctype = partsi(r).ctype;
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
							if (TYP(pmap[ny][nx]) == PT_FRME)
							{
								nx += rx, ny += ry;
							}
							int nx2 = nx, ny2 = ny;
							for (rrx = 0; sim->InBounds(nx, ny) && rrx < rtmp; rrx++, nx+=rx, ny+=ry) // fixed
							{
								rr = pmap[ny][nx], rt = TYP(rr);
								if (rt == PT_SPRK)
									rt = partsi(rr).ctype;
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
										sim->kill_part(ID(rr));
										continue;
									}
									else if (rii < 0)
										rii += rrx;
									else
										rii -= rrx;
								}
								nx = nx2 + rii * rx; ny = ny2 + rii * ry;
								partsi(rr).x = nx; partsi(rr).y = ny;
								pmap[ny][nx] = rr;
							}
						}
					}
			return return_value;
		case 25: // arrow key detector
			if (Element_MULTIPP::Arrow_keys == NULL)
				return return_value; // for failure case
			{
			int cst = Element_MULTIPP::Arrow_keys[0]; // current state
			int pst = Element_MULTIPP::Arrow_keys[1]; // previous state
			int ast = 0;
			if (rtmp & 0x10)
			{
				rtmp &= ~0x10;
				cst = (cst & 0x10) ? 0xF : 0;
				pst = (pst & 0x10) ? 0xF : 0;
			}
			switch (rtmp)
			{
				case 0: ast = pst; break;
				case 1: ast = cst & ~pst; break; // start pressing key
				case 2: ast = pst & ~cst; break; // end pressing key
				case 3: // check single arrow key
					(pst & (pst-1) & 0xF) || (ast = pst);
				break;
				case 4: ast = pst & (pst >> 2) & 3; ast |= (ast << 2); break;
				case 5: case 6:
					ast = pst & 0xF; ast |= (ast << 4); ast &= (ast >> (rtmp == 5 ? 1 : 3));
				break;
				default:
					return return_value;
			}
			for (rii = 0; rii < 4; rii++) // do 4 times
			{
				if (ast & (1 << rii)) // check direction
				{
					rx = tron_ry[rii];
					ry = tron_rx[rii];
					r = pmap[y+ry][x+rx]; // checking distance = 1
					if (!r || TYP(r) == PT_FILT || CHECK_EXTEL(r, ELEM_MDECOR))
					{
						rx *= 2; ry *= 2;
						r = pmap[y+ry][x+rx]; // checking distance = 2
					}
					if (!r)
						continue;
					if (isAcceptedConductor(sim, r))
						conductTo (sim, r, x+rx, y+ry, parts);
				}
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
				pavg = PAVG_INSL(r);
				if (!CHECK_EL_INSL(pavg))
				{
					rt = TYP(r);
					r >>= PMAPBITS;
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
		case 27: // powered BRAY shifter
			if (ELEMPROPW(i, &));
			{
				ELEMPROPW(i, &=~);
				return return_value;
			}
			parts[i].pavg[0] = parts[i].pavg[1];
			if (parts[i].pavg[1])
				parts[i].pavg[1] -= 1;
			break;
		case 28: // edge detector / SPRK signal lengthener
			rrx = parts[i].tmp2;
			parts[i].tmp2 = (rrx & 0xFE ? rrx - 1 : 0);
			switch (rtmp & 7)
			{
				case 0: rry = rrx >= 0x102; break; // positive edge detector
				case 1: rry = (rrx & 0xFF) == 1; break; // negative edge detector
				case 2: rry = (rrx & 0xFF); break; // lengthener
				case 3: rry = rrx >= 2 && rrx <= 0xFF; break; // shortener
				case 4: rry = (rrx & ~0xFF) || ((rrx & 0xFF) == 1); break; // double edge detector
				case 5: rry = rrx == 0x101; break; // single SPRK detector
				default: return return_value;
			}
			rrx &= 0xFE;
			for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				pavg = PAVG_INSL(r);
				if (!CHECK_EL_INSL(pavg))
				{
					rt = TYP(r);
					if (rt == PT_SPRK && partsi(r).life == 3 && partsi(r).ctype == PT_PSCN)
					{
						parts[i].tmp2 = rrx ? 9 : 0x109;
					}
					else if (rt == PT_NSCN && rry)
					{
						conductTo (sim, r, x+rx, y+ry, parts);
					}
				}
			}
			break;
		case 30: // get TPT options
			{
			bool inverted = rtmp & 0x80;
			rtmp &= 0x7F;
			switch (rtmp)
			{
				case  0: rr = sim->pretty_powder; break;		// get "P" option state
				case  1: rr = ren_->gravityFieldEnabled; break;	// get "G" option state
				case  2: rr = ren_->decorations_enable; break;	// get "D" option state
				case  3: rr = sim->grav->ngrav_enable; break;	// get "N" option state
				case  4: rr = sim->aheat_enable; break;			// get "A" option state
				case  5: rr = !sim->legacy_enable; break;		// check "Heat simulation"
				case  6: rr = sim->water_equal_test; break;		// check "Water equalization"
				case  7: rr = sim->air->airMode != 4; break;	// check "Air updating"
				case  8: rr = !(sim->air->airMode & 2); break;	// check "Air velocity"
				case  9: rr = !(sim->air->airMode & 1); break;	// check "Air pressure"
				case 10: rr = !sim->gravityMode; break;			// check "Vertical gravity mode"
				case 11: rr = sim->gravityMode == 2; break;		// check "Radial gravity mode"
#ifdef TPT_NEED_DLL_PLUGIN
				case 12: rr = (sim->dllexceptionflag & 2); break;	// is DLL call error trigged?
#endif
			}
			inverted && (rr = !rr);
			if (rr)
				Element_BTRY::update(UPDATE_FUNC_SUBCALL_ARGS);
			}
			break;
		}
		break;
	case 19:
		parts[i].tmp2 = parts[i].tmp;
		if (parts[i].tmp)
			--parts[i].tmp;
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (CHECK_EL_SPRKL3T(r, PT_PSCN))
					{
						parts[i].tmp = 9;
						return return_value;
					}
				}
		return return_value;
	case 20: // particle emitter
		{
		rctype = parts[i].ctype;
		int rctypeExtra = ID(rctype);
		// int EMBR_modifier;
		rctype &= PMAPMASK;
		if (!rctype)
			return return_value;
		if (rtmp <= 0)
			rtmp = 3;
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y-ry][x-rx];
					if (CHECK_EL_SPRKL3(r))
					{
						// if (ELEMPROPP(r, ctype) & PROP_INSULATED && rx && ry) // INWR, PTCT, NTCT, etc.
						//	continue;
						// EMBR_modifier = 0;
						if (rctype != PT_LIGH || partsi(r).ctype == PT_TESC || !(rand() & 15))
						{
							nx = x+rx; ny = y+ry;
							if (rctype == PT_EMBR || rctype == PT_ACID) // use by EMBR (explosion spark) emitter
							{
								rr = pmap[ny][nx];
								if (TYP(rr) != PT_GLAS)
									continue;
								ny += ry, nx += rx;
							}
							else if (rctype == PT_FIRE || rctype == PT_PLSM)
							{
								rr = pmap[ny][nx];
								while (BOUNDS_CHECK && TYP(rr) == PT_PSTN && partsi(rr).life)
								{
									ny += ry, nx += rx, rr = pmap[ny][nx];
								}
								rrt = TYP(rr);
								switch (rrt)
								{
								case PT_NONE:
									break;
								case PT_SPRK:
									rrt = partsi(rr).ctype;
									if (rrt <= 0 && rrt >= PT_NUM) break;
								default:
									if (!(sim->elements[rrt].Flammable || sim->elements[rrt].Explosive))
										break;
								case PT_BANG:
								case PT_COAL:
								case PT_BCOL:
								case PT_FIRW:
									sim->part_change_type(ID(rr), nx, ny, rctype);
									partsi(rr).life = rand()%50+150;
									partsi(rr).temp = restrict_flt(partsi(rr).temp + 5 * sim->elements[rrt].Flammable, MIN_TEMP, MAX_TEMP);
									partsi(rr).ctype = 0; // hackish, if ctype isn't 0 the PLSM might turn into NBLE later
									partsi(rr).tmp = 0; // hackish, if tmp isn't 0 the FIRE might turn into DSTW 
									break;
								case PT_THRM:
									sim->part_change_type(ID(rr), nx, ny, PT_LAVA); // from thermite
									partsi(rr).life = 400;
									partsi(rr).temp = MAX_TEMP;
									partsi(rr).ctype = PT_THRM;
									partsi(rr).tmp = 20;
									break;
								case PT_FWRK:
									{
										PropertyValue propv;
										propv.Integer = PT_DUST;
										sim->flood_prop(nx, ny, offsetof(Particle, ctype), propv, StructProperty::Integer);
									}
									break;
								case PT_IGNT:
									partsi(rr).tmp = 1;
									break;
								case PT_FUSE:
								case PT_FSEP:
									if (partsi(rr).life > 40)
										partsi(rr).life = 39;
									break;
								}
							}
							int np = sim->create_part(-1, nx, ny, rctype, rctypeExtra);
							if (np >= 0) {
								parts[np].vx = rtmp*rx; parts[np].vy = rtmp*ry;
								parts[np].dcolour = parts[i].dcolour;
								switch (rctype)
								{
								case PT_PHOT:
									if (np > i) ELEMPROPW(np, |=); // like E189 (life = 11)
									parts[np].temp = parts[i].temp;
									break;
								case PT_LIGH:
									parts[np].tmp = atan2(-ry, (float)rx)/M_PI*180;
									break;
								case PT_EMBR:
									parts[np].temp = partsi(rr).temp; // set temperature to EMBR
									if (partsi(rr).life > 0)
										parts[np].life = partsi(rr).life;
									break;
								}
							}
						// return return_value;
						}
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
						if (TYP(r) == PT_TRON && !(rx && ry)) // (!(rx && ry)) equivalent to (!rx || !ry)
						{
							rr = pmap[y-ry][x-rx];
							rt = TYP(rr);
							rr >>= PMAPBITS;
							if (rt == ELEM_MULTIPP && parts[rr].life == 30)
							{
								parts[rr].ctype &= ~0x1F;
								parts[rr].ctype |= (partsi(r).tmp >> 11) & 0x1F;
							}
						}
						else
						{
							if (!(rndstore&7))
							{
								switch (TYP(r))
								{
								case PT_MERC:
									partsi(r).tmp += parts[i].tmp;
									break;
								case PT_DEUT:
									partsi(r).life += parts[i].tmp;
									break;
								case PT_YEST:
									// rtmp = parts[i].tmp;
									if (rtmp > 0)
										partsi(r).temp = 303.0f + (rtmp > 28 ? 28 : (float)rtmp * 0.5f);
									else if (-rtmp > (rand()&31))
										sim->kill_part(ID(r));
									break;
								case PT_SPNG:
									if (partsi(r).life > 0)
									{
										rr = sim->create_part(-1, x-rx, y-ry, PT_WATR);
										if (rr >= 0)
											partsi(r).life --;
									}
									break;
								case PT_BTRY:
									// if (partsi(r).tmp > 0)
									{
										rr = pmap[y-ry][x-rx];
										rt = TYP(rr);
										if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_FRZW || rt == PT_WTRV)
										{
											rr >>= PMAPBITS;
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
											// partsi(r).tmp --;
										}
									}
									break;
								case PT_GOLD:
									rr = pmap[y-ry][x-rx];
									rt = TYP(rr);
									if (rt == PT_BMTL || rt == PT_BRMT)
									{
										sim->part_change_type(ID(rr), x-rx, y-ry, rtmp >= 0 ? PT_IRON : PT_TUNG);
									}
									break;
								case PT_CAUS:
									rr = pmap[y-ry][x-rx];
									rt = TYP(rr);
									if (rt == PT_CLNE || rt == PT_PCLN && partsi(rr).life == 10)
										rt = partsi(rr).ctype;
									if (rt == PT_WATR) // if it's water or water generator
									{
										sim->part_change_type(ID(r), x+rx, y+ry, PT_ACID);
										partsi(r).life += 10;
									}
									break;
								case PT_PQRT:
									if (partsi(r).tmp >= 0)
									{
										rr = pmap[y-ry][x-rx];
										if (TYP(rr) == PT_QRTZ && partsi(rr).tmp >= 0)
										{
											if (rtmp >= 0)
												rrt = partsi(r).tmp;
											else
												rrt = -partsi(rr).tmp;
											partsi(rr).tmp += rrt;
											partsi(r).tmp -= rrt;
										}
										else if (TYP(rr) == PT_SLTW)
										{
											partsi(r).tmp++;
											sim->kill_part(ID(rr));
										}
									}
									break;
								case ELEM_MULTIPP:
									if (partsi(r).life == 38)
									{
										rctype = partsi(r).ctype;
										if (rctype == PT_RFRG || rctype == PT_RFGL) // ACID/CAUS + GAS --> N RFRG
										{
											rr = pmap[y-ry][x-rx];
											if (TYP(rr) == PT_CAUS || TYP(rr) == PT_ACID)
											{
												partsi(r).tmp2 += 8;
												sim->kill_part(ID(rr));
											}

											rr = pmap[y+2*ry][x+2*rx];
											if (TYP(rr) == PT_GAS && partsi(r).tmp2 >= 8)
											{
												partsi(r).tmp2 -= 8;
												partsi(r).tmp += 3;
												sim->kill_part(ID(rr));
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
	case 22:
		if (parts[i].tmp2) parts[i].tmp2 --; break;
	case 24: // moving duplicator particle
		if (ELEMPROPW(i, &)) ELEMPROPW(i, &=~); // if wait flag exist, then clear wait flag
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
					rt = TYP(r);
					nx = x_src + rx_dest;
					ny = y_src + ry_dest;
					if (!sim->InBounds(nx, ny))
						break;
					rii = sim->create_part(-1, nx, ny, (rt == PT_SPRK) ? PT_METL : rt); // spark hack
					if (rii >= 0)
					{
						if (rt == PT_SPRK)
							sim->part_change_type(rii, nx, ny, PT_SPRK); // restore type for spark hack
						parts[rii] = partsi(r); // duplicating all properties?
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
			else if (CHECK_EXTEL(rr, 16) && parts[ID(rr)].ctype == 11)
			{
				sim->kill_part(i); break;
			}
			if (!sim->InBounds(rx_dest, ry_dest) || pmap[ry_dest][rx_dest]) // if out of boundary
				sim->kill_part(i);
			else
			{
				sim->pmap[y][x] = 0; // what stacked particle?
				sim->pmap[ry_dest][rx_dest] = PMAP(i, ELEM_MULTIPP); // actual is particle's index shift left by 8 plus particle's type
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
						if (isAcceptedConductor(sim, r))
							conductTo (sim, r, x+rx, y+ry, parts);
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
					if (CHECK_EL_SPRKL3(r))
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
				if (CHECK_EL_SPRKL3(r))
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
			int phshift = parts[i].ctype;
			int direction = Element_TRON::convertToAbsDirection(rtmp);
			nx = x + tron_rx[direction];
			ny = y + tron_ry[direction];
			r = pmap[ny][nx];
			if (!r)
			{
				int hue = (rtmp >> 11) & 0x1F;
				switch (rt & 7)
				{
					case 0: hue  = phshift; break; // set colour
					case 1: hue += phshift; break; // hue shift (add)
					case 2: hue -= phshift; break; // hue shift (subtract)
					case 3: // if color is / isn't ... then pass through
						if (((hue & 0x1F) == phshift) == ((phshift >> 5) & 1)) // rightmost 5 bits xnor 6th bit
							rtmp = 0;
					break;
				}
				if (rtmp & 1)
				{
					hue %= 23, (hue < 0) && (hue += 23);
					int np = Element_TRON::new_tronhead(sim, nx, ny, i, direction);
					if (np >= 0)
						parts[np].tmp &= ~0x0FF80,
						parts[np].tmp |= hue << 11;
				}
			}
			rtmp &= ~0x1FFFF;
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
			int direction = Element_TRON::convertToAbsDirection(rtmp);
			nx = x + tron_rx[direction];
			ny = y + tron_ry[direction];
			r = pmap[ny][nx];
			if (!r)
				Element_TRON::new_tronhead(sim, nx, ny, i, direction);
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
						if ((TYP(r)==PT_NSCN||TYP(r)==PT_PSCN||TYP(r)==PT_INWR) && sim->wireless2[parts[i].tmp][rii])
						{
							conductTo (sim, r, x+rx, y+ry, parts);
						}
					}
					if (TYP(r)==PT_SPRK && partsi(r).ctype!=PT_NSCN && partsi(r).life>=3)
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
					if (TYP(r) == PT_FILT)
					{
						rr = partsi(r).ctype + parts[i].ctype;
						rr &= 0x1FFFFFFF;
						rr |= 0x20000000;
						Element_MULTIPP::setFilter(sim, nx, ny, rx, ry, rr);
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
				if (CHECK_EL_SPRKL3(r))
				{
					rry = partsi(r).ctype == PT_INST ? 1 : 0;
					docontinue = 1;
					do
					{
						nx += rx; ny += ry;
						if (!sim->InBounds(nx, ny))
							break;
						r = pmap[ny][nx];
						if (!r || CHECK_EL_SPRK(r, PT_INWR)) // if it's empty or insulated wire
							continue;
						if ((ELEMPROP2(TYP(r)) & PROP_DRAWONCTYPE))
						{
							partsi(r).ctype = rrx;
						}
						else if (TYP(r) == ELEM_MULTIPP)
						{
							int p_life = partsi(r).life;
							if (p_life == 20 || p_life == 35 || p_life == 36)
							{
								partsi(r).ctype = rrx;
							}
						}
						else if (CHECK_EL_INSL(TYP(r)))
							break;
						docontinue = rry;
					} while (docontinue);
				}
			}
		break;
	case 36:
		rctype = TYP(parts[i].ctype);
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if (BOUNDS_CHECK && (!rx != !ry))
				{
					r = pmap[y+ry][x+rx];
					if (CHECK_EL_SPRKL3(r))
					{
						rii = (rtmp & 1) ? PROP_DEBUG_HIDE_TMP : PROP_DEBUG_USE_TMP2;
						switch (partsi(r).ctype)
						{
							case PT_PSCN: ELEMPROP2(rctype) |=  rii; break;
							case PT_NSCN: ELEMPROP2(rctype) &= ~rii; break;
							case PT_INWR: ELEMPROP2(rctype) ^=  rii; break;
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
				if (CHECK_EL_SPRK(r, PT_INWR))
					sim->kill_part(ID(r)), rr |= 4;
				else
					goto kill1;
			}
		}
		parts[i].x = rx;
		parts[i].y = ry;
		
		parts[i].tmp = rr;
		pmap[ry][rx] = PMAP(i, parts[i].type);
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
			int rctypeExtra = ID(rctype);
			rctype &= PMAPMASK;
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
					switch (TYP(r))
					{
					case ELEM_MULTIPP:
						if (partsi(r).life == 38)
						{
							rii = PMAP(rctypeExtra, rctype);
							if (!TYP(partsi(r).ctype) && rctype)
								partsi(r).ctype = rii;
							if (partsi(r).ctype == rii)
							{
								rry = partsi(r).tmp2 & 0x7;
								switch (k1[rrx][rry])
								{
									case 0: rii = (partsi(r).tmp - rtmp) >> 1; break;
									case 1: rii = -rtmp; break;
									case 2: rii = partsi(r).tmp; break;
									default: rii = 0;
								}
								rtmp += rii;
								partsi(r).tmp -= rii;
							}
						}
						break;
					case PT_PCLN:
					case PT_PBCN:
						if (partsi(r).life != 10)
							break;
					case PT_CLNE:
					case PT_BCLN:
						if (!rctype)
						{
							parts[i].ctype = rctype = partsi(r).ctype;
							if (rctype == PT_LAVA)
							{
								rctypeExtra = partsi(r).tmp;
								parts[i].ctype |= PMAPID(rctypeExtra);
							}
						}
						if (rctype == partsi(r).ctype && (rctype != PT_LAVA || rctypeExtra == partsi(r).tmp))
							rtmp += 5;
						break;
					case PT_SPNG:
						rctype || (parts[i].ctype = rctype = PT_WATR);
						if (rctype == PT_WATR || rctype == PT_DSTW || rctype == PT_CBNW)
						{
							int * absorb_ptr = &partsi(r).life;
							if (sim->pv[y/CELL][x/CELL]<=3 && sim->pv[y/CELL][x/CELL]>=-3)
							{
								rtmp += *absorb_ptr, *absorb_ptr = 0;
							}
							else
							{
								*absorb_ptr += rtmp, rtmp = 0;
							}
						}
					case PT_GEL: // reserved by GEL.cpp
						break;
					default:
						if (ELEMPROPT(r) & (TYPE_PART | TYPE_LIQUID | TYPE_GAS))
						{
							if (!rctype)
							{
								parts[i].ctype = rctype = ID(r);
								if (rctype == PT_LAVA)
								{
									rctypeExtra = partsi(r).ctype;
									parts[i].ctype |= PMAPID(rctypeExtra);
								}
							}
							if (rctype == TYP(r) && (rctype != PT_LAVA || rctypeExtra == partsi(r).ctype) && rrx != 4)
							{
								rtmp++;
								sim->kill_part(ID(r));
							}
						}
					}
				}
			}
			parts[i].tmp = rtmp;
		}
		break;
	case 39:
		if (ID(parts[i].ctype))
		{
			parts[i].ctype -= PMAPID(1);
			if (!ID(parts[i].ctype))
			{
				parts[i].life = 0;
				sim->part_change_type(i, x, y, parts[i].ctype);
				return return_value;
			}
		}
		break;
#if !defined(RENDERER) && defined(LUACONSOLE)
	case 40:
		{
			int funcid = (parts[i].ctype & 0x3F) + 0x100;
			if (luacon_ci->trigger_func[funcid].m) luacall_debug_trigger (funcid, i, x, y);
		}
		break;

	default: // reserved by Lua
		if (lua_el_mode[parts[i].type] == 1)
			return_value = 0;
#endif
	}
		
	return return_value;
}
