#include "simulation/Elements.h"
#include "simulation/MULTIPPE_Update.h"

//#TPT-Directive ElementClass Element_ARAY PT_ARAY 126
Element_ARAY::Element_ARAY()
{
	Identifier = "DEFAULT_PT_ARAY";
	Name = "ARAY";
	Colour = PIXPACK(0xFFBB00);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	HeatConduct = 0;
	Description = "Ray Emitter. Rays create points when they collide.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_ARAY::update;
}

using namespace MULTIPPE_Update;

//#TPT-Directive ElementHeader Element_ARAY static const unsigned char to_angle[9];
const unsigned char Element_ARAY::to_angle[9] = {0,1,2,7,0,3,6,5,4};

//#TPT-Directive ElementHeader Element_ARAY static const signed char param1[4][2];
const signed char Element_ARAY::param1[4][2] = {{-1, 1}, {1, -1}, {1, 1}, {-1, -1}};

//#TPT-Directive ElementHeader Element_ARAY static char tmpdir[2];
char Element_ARAY::tmpdir[2];

//#TPT-Directive ElementHeader Element_ARAY static int update(UPDATE_FUNC_ARGS)
int Element_ARAY::update(UPDATE_FUNC_ARGS)
{
	int r_life, swap;
	int modProp;
	int modFlag;
	static float flt1;
	static Particle *partt;
	// if (!parts[i].life)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (CHECK_EL_SPRKL3(r))
					{
						// EmitRay (i, r, x, y, rx, ry, sim, parts);
						bool isBlackDeco = false;
						int inputType = partsi(r).ctype;
						int destroy = (inputType == PT_PSCN) ? 1 : 0;
						int nostop = (inputType == PT_INST) ? 1 : 0;
						int spc_conduct = 0, ray_less = 0;
						int colored = 0, noturn = 0, rt, b;
						static int tmp[4];
						int max_turn = parts[i].tmp, tmpz = 1, spacetmp = 0;
						int r_incr = 1, pass_wall = 1;
						if (max_turn <= 0)
							max_turn = 256;
						modFlag = 0;
						int docontinue, nxx, nxi, nx2, nyy, nyi, ny2;
						for (docontinue = 1, nxx = nxi = rx*-1, nyy = nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi)
						{
							if (!(x+nxx<XRES && y+nyy<YRES && x+nxx >= 0 && y+nyy >= 0))
							{
							break1a:
								break;
							}
							if (!pass_wall)
							{
								int block1 = sim->bmap[(y+nyy)/CELL][(x+nxx)/CELL];
								if (block1 && (block1 != WL_STREAM && block1 != WL_FAN && block1 != WL_DETECT && block1 != WL_GRAV && block1 != WL_ALLOWALLELEC))
									break;
							}

						continue1a:
							r = pmap[y+nyy][x+nxx];
							rt = TYP(r);
							r = ID(r);
							
							if (!rt)
							{
								tmpz = 0;
								if (ray_less)
									continue;
								int nr = sim->create_part(-1, x+nxx, y+nyy, PT_BRAY);
								if (nr != -1)
								{
									// if it came from PSCN
									if (destroy)
									{
										parts[nr].tmp = 2;
										parts[nr].life = 2;
									}
									else
										parts[nr].ctype = colored;
									parts[nr].temp = parts[i].temp;
									if (isBlackDeco)
										parts[nr].dcolour = 0xFF000000;
								}
								continue;
							}
							else if (rt == ELEM_MULTIPP)
							{
								r_life = parts[r].life;
								int front1, front2;
								switch (r_life)
								{
								case 2:
									nyy += nyi; nxx += nxi;
									tmp[0] = pmap[y+nyy][x+nxx];
									switch (TYP(tmp[0]))
									{
									case PT_SWCH:
									case PT_HSWC:
										tmp[2] = TYP(tmp[0]);
										if (inputType == PT_INWR)
											destroy = partsi(tmp[0]).life >= 10;
										tmp[1] = destroy ? 9 : 10;
										docontinue = 0;
										do
										{
											partsi(tmp[0]).life = tmp[1];
											nyy += nyi; nxx += nxi;
											if (!BOUNDS_CHECK)
												goto break1a;
											tmp[0] = pmap[y+nyy][x+nxx];
										}
										while (TYP(tmp[0]) == tmp[2]);
										break;
									case PT_LAVA: 
										if (!partsi(tmp[0]).ctype || partsi(tmp[0]).ctype == PT_STNE)
										{
											partsi(tmp[0]).ctype = PT_BRCK;
											partsi(tmp[0]).tmp = 0;
										}
										break;
									case PT_CRAY:
										partt = &partsi(tmp[0]);
										ny2 = y + nyy + nyi * partt->tmp2;
										nx2 = x + nxx + nxi * partt->tmp2;
										{
											int remainp = partt->tmp;
											if (remainp < 1) remainp = 1;
											while (remainp--)
											{
												nx2 += nxi; ny2 += nyi;
												if (!sim->InBounds(nx2, ny2)) break;
												r = pmap[ny2][nx2];
												if ((sim->elements[TYP(r)].Properties2 & PROP_DRAWONCTYPE) || CHECK_EXTEL(r, 35))
													partsi(r).ctype = partt->ctype;
											}
										}
										goto break1a;
									case ELEM_MULTIPP:
										b = partsi(tmp[0]).life;
										if (b == 4)
										{
											partsi(tmp[0]).ctype = colored;
											docontinue = nostop;
										}
										else if (b == 35)
										{
											partsi(tmp[0]).ctype &= PMAPMASK;
											partsi(tmp[0]).ctype |= PMAPID(colored);
											docontinue = nostop;
										}
										break;
									default:
										tmp[2] = TYP(tmp[0]);
										if (tmp[2] == PT_INWR || tmp[2] == PT_QRTZ)
										{
											do
											{
												if (tmp[2] == PT_INWR)
													sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);
												else if (partsi(tmp[0]).tmp < 0)
													partsi(tmp[0]).tmp = 0;
												nyy += nyi; nxx += nxi;
												if (!BOUNDS_CHECK)
													goto break1a;
												tmp[0] = pmap[y+nyy][x+nxx];
											}
											while (TYP(tmp[0]) == tmp[2]);
										}
										goto continue1a;
									}
									continue;
								case 6:
									if (spc_conduct == 5)
										parts[r].temp = (modFlag & 2) ? flt1 : parts[i].temp;
									continue;
								case 13:
									if (parts[r].tmp2 == 3)
									{
										unsigned int deco = parts[r].ctype;
										int deco_length = parts[r].tmp;
										for (nyy+=(1+spacetmp)*nyi, nxx+=(1+spacetmp)*nxi; deco_length--; nyy+=nyi, nxx+=nxi)
										{
											if (!sim->InBounds(x+nxx, y+nyy))
												break;
											r = pmap[y+nyy][x+nxx];
											if (!r)
												continue;
											partsi(r).dcolour = deco;
										}
										goto break1a;
									}
									continue;
								case 16:
									switch (parts[r].ctype)
									{
									case 1:
										if (!parts[r].tmp && r > i) // If the other particle hasn't been life updated
											parts[r].flags |= FLAG_SKIPMOVE;
										parts[r].tmp += (r_incr > 1) ? r_incr : 1;
										break;
									case 8:
										tmpdir[1] = parts[r].tmp;
										continue;
									case 12: // random generator
										docontinue = rand() & 1;
										continue;
									case 13:
										if (max_turn-- <= 0) goto break1a;
										tmp[0] = parts[r].tmp;
										nyy += tmp[0] * nxi; nxx -= tmp[0] * nyi;
										nxi = -nxi; nyi = -nyi;
										continue;
									case 17:
									case 18:
										parts[r].tmp = PMAP(destroy, PT_BRAY);
										parts[r].tmp2 = 1;
										break;
									case 20:
										nyy += nyi; nxx += nxi;
										front1 = pmap[y+nyy][x+nxx];
										while (BOUNDS_CHECK && TYP(front1) == PT_FILT)
										{
											partsi(front1).life = 4;
											nyy += nyi; nxx += nxi;
											front1 = pmap[y+nyy][x+nxx];
										}
										tmp[0] = parts[r].tmp2 + (r < i); // delay time
										if (tmp[0] <= 0)
											tmp[0] += parts[r].tmp; // add pulse time
										switch (TYP(front1))
										{
										case PT_DLAY:
											front1 >>= 8;
											tmp[1] = parts[front1].life - (front1 > i);
											if (tmp[1] == 0 && front1 > i)
												Element_DLAY::update (sim, front1, x+nxx, y+nyy, 0, 0, parts, pmap);
											if (tmp[1] <= 0)
												parts[front1].life = tmp[0] + (front1 > i);
											break;
										case PT_INST:
											if (tmp[0] == 1)
											{
												partsi(front1).life = 4;
												partsi(front1).ctype = PT_INST;
												sim->part_change_type(ID(front1), x+nxx, y+nyy, PT_SPRK);
											}
										}
										goto break1a;
									case 27:
										{
											bool bef1 = (r > i) && !(parts[r].flags & FLAG_SKIPMOVE);
											if (destroy)
											{
												if (bef1)
												{
													parts[r].pavg[0] = parts[r].pavg[1];
													parts[r].flags |= FLAG_SKIPMOVE;
												}
												parts[r].pavg[1] = parts[i].tmp;
											}
											else if (parts[r].pavg[bef1 ? 1 : 0])
											{
												nyy += parts[r].tmp * nxi;
												nxx -= parts[r].tmp * nyi;
											}
										}
										continue;
									case 29: // Destroying FILT
										{
											int vtmp = parts[r].tmp + 1;
											int rem1 = parts[r].tmp2;
											nyy += vtmp * nyi; nxx += vtmp * nxi;
											while (sim->InBounds(x+nxx, y+nyy))
											{
												front1 = pmap[y+nyy][x+nxx];
												if (TYP(front1) == PT_FILT)
													sim->kill_part(ID(front1));
												else if (!rem1) break;
												if (rem1 && !--rem1) break;
												nyy += nyi, nxx += nxi;
											}
										}
										goto break1a;
									}
									docontinue = nostop;
									continue;
								case 19:
									r_incr += (int)((parts[r].temp + 26.85f) / 100) - 3;
									continue;
								case 20:
									if (!(modFlag & 1))
									{
										if (!destroy)
										{
											if (!colored)
												colored = 0x3FFFFFFF;
											tmp[0] = sim->elements[TYP(parts[r].ctype)].PhotonReflectWavelengths;
											if (parts[r].tmp2 & 0x1)
												tmp[0] = ~tmp[0];
											colored &= tmp[0];
											if (!colored)
												goto break1a;
										}
									}
									else
									{
										parts[r].ctype = modProp;
										if (!nostop)
											goto break1a;
									}
									continue;
								case 23:
									{
										front1 = pmap[y+nyi+nyy][x+nxi+nxx];
										int ftype1 = TYP(front1);
										while (ftype1 == PT_BIZR || ftype1 == PT_BIZRG || ftype1 == PT_BIZRS)
										{
											colored = partsi(front1).ctype;
											nyy+=nyi; nxx+=nxi;
											front1 = pmap[y+nyi+nyy][x+nxi+nxx];
											ftype1 = TYP(front1);
										}
										if (ftype1 == PT_SWCH && (parts[r].tmp > 0))
										{
											partsi(front1).life = 9 + (parts[r].tmp & 1);
											docontinue = nostop;
										}
									}
									continue;
								case 28:
									if (noturn)
										continue;
									if (!max_turn)
										goto break1a;
									// nxx += nxi; nyy += nyi;
									tmp[1] = parts[r].tmp & 0x1F;
									switch (tmp[1])
									{
									case 0: case 1: case 2: case 3: // turn right, turn left, "\" and "/" reflect
										tmp[0] = nxi;
										nxi = param1[tmp[1]][0] * nyi;
										nyi = param1[tmp[1]][1] * tmp[0];
										break;
									case 4: // go "/\"
										nxi = 0; nyi = -1;
										break;
									case 5: // go "\/"
										nxi = 0; nyi = 1;
										break;
									case 6: // go ">"
										nxi = 1; nyi = 0;
										break;
									case 7: // go "<"
										nxi = -1; nyi = 0;
										break;
									case 10: // "-" reflect
										nyi = -nyi;
										break;
									case 11: // "|" reflect
										nxi = -nxi;
										break;
									case 8:
									case 9:
										{
											int tdiff = (tmp[1] == 8 ? 1 : -1) * parts[r].tmp2;
											while (nxx += nxi, nyy += nyi, BOUNDS_CHECK)
											{
												front1 = pmap[y+nyy][x+nxx];
												if (!front1) goto break1a;
												switch (TYP(front1))
												{
												case PT_FILT:
													partsi(front1).life = 4; break;
												case PT_ARAY: case PT_CRAY:
													{
													float ftemp = partsi(front1).temp + tdiff;
													partsi(front1).temp = restrict_flt(ftemp, MIN_TEMP, MAX_TEMP);
													}
													goto break1a;
												case ELEM_MULTIPP:
													if (partsi(front1).life == 28)
														partsi(front1).tmp2 += tdiff;
													break;
												default: goto break1a;
												}
											}
										}
										goto break1a;
									case 12:
									case 13:
										{
											int nxi2 = (tmp[1] == 12 ? -nyi : nyi);
											int nyi2 = (tmp[1] == 12 ? nxi : -nxi);
											tmp[2] = nxx; tmp[3] = nyy;
											while (nxx += nxi2, nyy += nyi2, BOUNDS_CHECK)
											{
												r = pmap[y+nyy][x+nxx];
												if (TYP(r) != PT_FILT) break;
												partsi(r).ctype = colored;
											}
											nxx = tmp[2] - nxi; nyy = tmp[3] - nyi;
										}
										continue;
									case 14:
									case 15:
										tmp[0] = to_angle[nxi + 3 * nyi + 4];
										tmp[1] = tmp[1] * 6 + 5;
										tmp[0] += tmp[1] * parts[r].tmp2;
										tmp[0] &= 7;
										// see PRTI.cpp
										nxi = sim->portal_rx[tmp[0]];
										nyi = sim->portal_ry[tmp[0]];
										break;
									case 16:
									case 17:
										modFlag ^= 4;
										tmp[2] = to_angle[nxi + 3 * nyi + 4];
										if (modFlag & 4)
										{
											tmp[3] = parts[r].tmp2;
											tmpdir[0] = tmp[2];
										}
										else
										{
											tmp[3] = (tmpdir[0] + tmp[1] * 4) & 7;
											if ((tmp[3] ^ tmp[2]) == 4)
												tmp[3] ^= 4;
										}
										nxi = sim->portal_rx[tmp[3]];
										nyi = sim->portal_ry[tmp[3]];
										break;
									}
									// nxx -= nxi; nyy -= nyi;
									max_turn--;
									continue;
								case 32:
									tmp[0] = parts[r].tmp;
									tmp[1] = parts[r].tmp2;
									switch (tmp[1])
									{
									case 0:
										// temp_z1[8] = noturn;
										noturn = (tmp[0] >> (4 * noturn)) & 0x7; // Easier for inputing hexadecimal?
										if (noturn >= 3)
										{
											goto break1a;
										}
										break;
									case 1:
										// temp_z1[8] = tmp2 = nostop | (destroy << 1);
										tmp[0]  ^= 0x2A;
										nostop   = (tmp[0] >> (nostop   ? 1 : 0)) & 0x1;
										destroy  = (tmp[0] >> (destroy  ? 3 : 2)) & 0x1;
										ray_less = (tmp[0] >> (ray_less ? 5 : 4)) & 0x1;
										break;
									case 9:
										noturn = 2; // no break
									case 2:
										spc_conduct = tmp[0];
										break;
									case 3:
										if ((sim->portal_rx[tmp[0]] != nxi) || (sim->portal_ry[tmp[0]] != nyi))
											goto break1a;
										break;
									case 4:
										spacetmp = tmp[0];
										break;
									case 5:
										pass_wall = !pass_wall;
										break;
									case 6:
										{
											nxx += nxi; nyy += nyi;
											front1 = pmap[y+nyy][x+nxx];
											switch (TYP(front1))
											{
											case PT_NONE:
												sim->create_part(-1, x+nxx, y+nyy, PT_BRAY);
												break;
											case PT_ARAY:
											case PT_CRAY:
												if (modFlag & 2)
												{
													partsi(front1).temp = flt1;
													goto break1a;
												}
												break;
											case PT_STOR:
												modFlag |= 2;
												flt1  = partsi(front1).temp;
												break;
											case PT_FRAY:
												spacetmp += partsi(front1).tmp;
												break;
											case PT_INVIS:
												spacetmp += (int)(sim->sim_max_pressure + 0.5f);
												break;
											case ELEM_MULTIPP:
												while (BOUNDS_CHECK && CHECK_EXTEL(front1, 5))
												{
													if (!destroy)
													{
														partsi(front1).tmp  = 1;
														partsi(front1).tmp2 = tmp[0]*nxi;
														partsi(front1).tmp3 = tmp[0]*nyi;
													}
													else
													{
														partsi(front1).tmp  = 0;
														partsi(front1).tmp2 = 0;
													}
													nxx += nxi; nyy += nyi;
													front1 = pmap[y+nyy][x+nxx];
												}
												if (!nostop) goto break1a;
												nxx -= nxi; nyy -= nyi;
												break;
											case PT_SWCH:
												if (tmp[0])
												{
													nx2 = x+nxx-tmp[0]*nyi;
													ny2 = y+nyy+tmp[0]*nxi;
													if (sim->InBounds(nx2, ny2))
													{
														front2 = pmap[ny2][nx2];
														if (TYP(front2) == PT_SWCH)
															partsi(front1).life = ((partsi(front1).life >= 10) ? 9 : 10);
													}
													docontinue = nostop;
												}
												continue;
											case PT_FILT:
												Element_MULTIPP::setFilter(sim, x+nxx, y+nyy, nxi, nyi, colored);
												goto break1a;
											}
										}
										break;
									case 7: // remove front SPRK
										{
											nxx += nxi; nyy += nyi;
											front1 = pmap[y+nyy][x+nxx];
											int f_type = TYP(front1);
											if (TYP(front1) == PT_SPRK)
											{
												if (sim->clear_spark(ID(front1), x+nxi+nxx, y+nyi+nyy) == PT_SWCH)
													parts[ID(front1)].life = 19; // keep SWCH on
											}
											else if (f_type == PT_SWCH)
											{
												tmp[2] = TYP(tmp[0]);
												tmp[0] >>= 8;
												if (tmp[0])
												{
													nx2 = x+nxx-tmp[0]*nyi;
													ny2 = y+nyy+tmp[0]*nxi;
													if (!sim->InBounds(nx2, ny2))
														goto continue1b;
													front2 = pmap[ny2][nx2];
													if (TYP(front2) == PT_SWCH)
													{
														b = ((partsi(front2).life >= 10) ? 4 : 0) | ((partsi(front1).life >= 10) ? 2 : 0);
														b = (tmp[2] >> b);
														partsi(front1).life = ((b & 1) ? 10 : 9);
														partsi(front2).life = ((b & 2) ? 10 : 9);
													}
												continue1b:
													docontinue = nostop;
													continue;
												}
												else if (partsi(front1).life >= 10)
												{
													partsi(front1).life = 19; // keep SWCH on
												}
											}
											if (sim->elements[f_type].Properties & (PROP_CONDUCTS|PROP_CONDUCTS_SPEC))
												partsi(front1).life = 8;
											goto break1a;
										}
										break;
									case 8:
										if (max_turn-- <= 0)
											goto break1a;
										if (!((parts[r].ctype >> (tmpdir[1] & 0x1F)) & 1))
											nxx += tmp[0]*nxi, nyy += tmp[0]*nyi;
										break;
									case 10:
										nx2 = x+nxx-tmp[0]*nyi;
										ny2 = y+nyy+tmp[0]*nxi;
										if (sim->InBounds(nx2, ny2))
										{
											int rr = pmap[ny2][nx2];
											if (TYP(rr) == PT_FILT)
											{
												if (inputType == PT_NSCN)
												{
													partsi(pmap[ny2][nx2]).ctype = colored;
													goto break1a;
												}
												colored = Element_FILT::interactWavelengths(&partsi(pmap[ny2][nx2]), colored);
											}
											else if (TYP(rr) == PT_VOID || TYP(rr) == PT_PVOD && partsi(rr).life >= 10)
												goto break1a;
										}
										break;
									}
									tmpz = (tmp[1] != 8) ? 1 : 0;
								case 33:
									continue;
								case 35:
									if (!(modFlag & 1))
									{
										modFlag |= 1;
										modProp = parts[r].ctype;
										continue;
									}
									parts[r].ctype = modProp;
									if (!nostop)
										goto break1a;
									continue;
								}
							}
							else if (noturn >= 2)
							{
								if (CHECK_EL_INSL(rt))
									break;
								if (spc_conduct > 0)
								{
									if (rt != PT_INWR && (rt != PT_SPRK || parts[r].ctype != PT_INWR))
									{
										switch (spc_conduct)
										{
										case 1:
											sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);
											goto break1a;
										case 2:
											sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);
											if (!(parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (sim->elements[parts[r].ctype].Properties&PROP_CONDUCTS)))
												goto break1a;
										case 3:
											sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);
											break;
										case 4:
											if (rt == PT_INST)
											{
												docontinue = nostop;
												sim->FloodINST(x+nxx, y+nyy, PT_SPRK, PT_INST);
											}
											else
												sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);
											continue;
										case 5:
											if (rt == PT_WOOD)
												sim->part_change_type(r, x+nxx, y+nyy, PT_SAWD);
											else if (rt == PT_ARAY || rt == PT_BRAY || rt == PT_CRAY || rt == PT_HEAC)
											{
												// ARAY, BRAY, CRAY, DRAY, FRAY
												parts[r].temp = parts[i].temp;
												if (rt == PT_BRAY)
												{
													if (nostop && !(parts[r].tmp >> 1))
													{
														parts[r].tmp = 1;
														parts[r].life = 1020;
													}
													else if (destroy)
													{
														parts[r].tmp = 2;
														parts[r].life = 1;
													}
												}
												else if (inputType == PT_BMTL)
													goto break1a;
											}
											else if (rt == PT_QRTZ)
											{
												sim->part_change_type(r, x+nxx, y+nyy, PT_PQRT);
												parts[r].life = 5;
											}
											continue;
										case 6: // melting HEAC
											if (rt == PT_HEAC && parts[r].temp > sim->elements[PT_HEAC].HighTemperature)
											{
												sim->part_change_type(r, x+nxx, y+nyy, PT_LAVA);
												parts[r].ctype = PT_HEAC;
											}
											else if (rt == PT_VIBR || rt == PT_BVBR) // VIBR Explosion?
											{
												if (destroy)
												{
													if (parts[r].life)
														parts[r].tmp2 = 1;
													parts[r].tmp = 0;
												}
												else if (inputType == PT_TESC)
												{
													if (parts[r].life > 0 && parts[r].life < (((int)parts[i].temp + 732) / 10))
														parts[r].life += rand() % 300 + 200;
												}
												else if (!parts[r].life)
													parts[r].life = 750;
											}
											continue;
										}
									}
								}
								continue;
							} 
							if (!destroy)
							{
								if (rt == PT_BRAY)
								{
									// cases for hitting different BRAY modes
									switch(parts[r].tmp)
									{
									// normal white
									case 0:
										if (!tmpz)
										{
											parts[r].life = 1020; // makes it last a while
											parts[r].tmp = 1;
											if (!parts[r].ctype) // and colors it if it isn't already
												parts[r].ctype = colored;
										}
									// red bray or any other random tmp mode, stop
									case 2:
									default:
										docontinue = 0;
										break;
									// long life, reset it
									case 1:
										parts[r].life = 1020;
										//docontinue = 1;
										break;
									}
									if (isBlackDeco)
										parts[r].dcolour = 0xFF000000;
								}
								// get color if passed through FILT
								else if (rt == PT_FILT)
								{
									if (parts[r].tmp != 6)
									{
										colored = Element_FILT::interactWavelengths(&parts[r], colored);
										if (!colored)
											break;
									}
									isBlackDeco = (parts[r].dcolour==0xFF000000);
									parts[r].life = 4;
								}
								else if ((modFlag & 1) && (sim->elements[rt].Properties2 & PROP_DRAWONCTYPE))
								{
									parts[r].ctype = modProp;
									docontinue = nostop;
								}
								else if (rt == PT_STOR)
								{
									int rx1, ry1;
									if (parts[r].tmp)
									{
										//Cause STOR to release
										for (ry1 = 1; ry1 >= -1; ry1--)
										{
											for (rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1 - rx1 + 1)
											{
												int np = sim->create_part(-1, x + nxx + rx1, y + nyy + ry1, TYP(parts[r].tmp));
												if (np != -1)
												{
													parts[np].temp = parts[r].temp;
													parts[np].life = parts[r].tmp2;
													/* original code:
													parts[np].tmp = parts[r].pavg[0];
													parts[np].ctype = parts[r].pavg[1];
													parts[np].tmp2 = parts[r].tmp3;
													parts[np].tmp3 = parts[r].tmp4;
													*/
													parts[np].tmp = parts[r].tmp3;
													parts[np].ctype = parts[r].tmp4;
													parts[np].tmp2 = parts[r].pavg[0];
													parts[np].tmp3 = parts[r].pavg[1];
													parts[np].dcolour = parts[r].cdcolour;
													parts[r].tmp = 0;
													parts[r].life = 10;
													goto break1b;
												}
											}
										}
									}
									else
									{
										parts[r].life = 10;
									}
								// this if prevents BRAY from stopping on certain materials
								}
								else if (rt != PT_INWR && (rt != PT_SPRK || parts[r].ctype != PT_INWR) && rt != PT_ARAY && rt != PT_WIFI &&
									!(rt == PT_SWCH && parts[r].life >= 10) && !(rt == PT_LDTC && (parts[r].tmp2 & 0x10)))
								{
									if (!tmpz)
										sim->create_part(-1, x+nxx, y+nyy, PT_SPRK);

									if (!(nostop && parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (sim->elements[parts[r].ctype].Properties&PROP_CONDUCTS)))
										docontinue = 0;
									else
										docontinue = 1;
								}
							break1b:
								tmpz = 0;
							}
							else if (destroy)
							{
								if (rt == PT_BRAY)
								{
									parts[r].tmp = 2;
									parts[r].life = 1;
									docontinue = 1;
									if (isBlackDeco)
										parts[r].dcolour = 0xFF000000;
								//this if prevents red BRAY from stopping on certain materials
								}
								else if (rt==PT_STOR || rt==PT_INWR || (rt==PT_SPRK && parts[r].ctype==PT_INWR) || rt==PT_ARAY || rt==PT_WIFI || rt==PT_FILT ||
									(rt == PT_SWCH && parts[r].life >= 10) || (rt == PT_LDTC && (parts[r].tmp2 & 0x10)))
								{
									if (rt == PT_STOR)
									{
										parts[r].tmp = 0;
										parts[r].life = 0;
									}
									else if (rt == PT_FILT)
									{
										isBlackDeco = (parts[r].dcolour==0xFF000000);
										parts[r].life = 2;
									}
									docontinue = 1;
								}
								else
								{
									docontinue = 0;
								}
							}
						}
					}
					//parts[i].life = 4;
				}
	}
	return 0;
}


Element_ARAY::~Element_ARAY() {}
