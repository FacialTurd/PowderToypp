#include "simulation/Elements.h"
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

	Temperature = R_TEMP+0.0f +273.15f;
	HeatConduct = 0;
	Description = "Ray Emitter. Rays create points when they collide.";

	Properties = TYPE_SOLID|PROP_LIFE_DEC;

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

// #TPT-Directive ElementHeader Element_ARAY static int temp_z1[5];
// int Element_ARAY::temp_z1[5];

//#TPT-Directive ElementHeader Element_ARAY static int update(UPDATE_FUNC_ARGS)
int Element_ARAY::update(UPDATE_FUNC_ARGS)
{
	int r_life, swap;
	int modProp;
	int modFlag;
	if (!parts[i].life)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF) == PT_SPRK && parts[r>>8].life == 3)
					{
						bool isBlackDeco = false;
						int destroy = (parts[r>>8].ctype==PT_PSCN) ? 1 : 0;
						int nostop = (parts[r>>8].ctype==PT_INST) ? 1 : 0;
						int spc_conduct = 0, ray_less = 0;
						int colored = 0, noturn = 0, rt;
						static int tmp[4];
						int max_turn = parts[i].tmp, tmpz = 0, tmpz2 = 0;
						int r_incr = 1, pass_wall = 1;
						if (max_turn <= 0)
							max_turn = 256;
						modFlag = 0;
						for (int docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi)
						{
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0))
							{
							break1a:
								break;
							}
							if (!pass_wall)
							{
								int block1 = sim->bmap[(y+nyi+nyy)/CELL][(x+nxi+nxx)/CELL];
								if (block1 && (block1 != WL_STREAM && block1 != WL_FAN && block1 != WL_DETECT && block1 != WL_GRAV && block1 != WL_ALLOWALLELEC))
									break;
							}

							r = pmap[y+nyi+nyy][x+nxi+nxx];
							rt = r & 0xFF;
							r = r >> 8;
							
							if (!rt)
							{
								if (ray_less)
									continue;
								int nr = sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_BRAY);
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
							else if (rt == PT_E189)
							{
								r_life = parts[r].life;
								switch (r_life)
								{
								case 6:
									if (spc_conduct == 5)
										parts[r].temp = parts[i].temp;
								case 13:
									if (parts[r].tmp2 == 3)
									{
										tmp[0] = parts[r].ctype;
										tmp[1] = parts[r].tmp;
										for (nyy+=(2+tmpz2)*nyi, nxx+=(2+tmpz2)*nxi; tmp[1]--; nyy+=nyi, nxx+=nxi)
										{
											if (!sim->InBounds(x+nxx, y+nyy))
												break;
											r = pmap[y+nyy][x+nxx];
											if (!r)
												continue;
											parts[r>>8].dcolour = tmp[0];
										}
										goto break1a;
									}
									
								case 16:
									if (parts[r /* actually: r>>8 */].ctype == 1)
									{
										if (!parts[r].tmp && r > i) // If the other particle hasn't been life updated
											parts[r].flags |= FLAG_SKIPMOVE;
										parts[r].tmp += (r_incr > 1) ? r_incr : 1;
									}
									docontinue = nostop;
									continue;
								case 19:
									r_incr += (int)((parts[r].temp + 26.85f) / 100) - 3;
									continue;
								case 20:
									if (!(modFlag & 1))
									{
										if (!colored)
											colored = 0x3FFFFFFF;
										tmp[0] = sim->elements[parts[r].ctype & 0xFF].PhotonReflectWavelengths;
										if (parts[r].tmp & 0x1)
											tmp[0] = ~tmp[0];
										colored &= tmp[0];
										if (!colored)
											goto break1a;
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
										int front1 = pmap[y+2*nyi+nyy][x+2*nxi+nxx];
										int ftype1 = front1 & 0xFF;
										while (ftype1 == PT_BIZR || ftype1 == PT_BIZRG || ftype1 == PT_BIZRS)
										{
											colored = parts[front1 >> 8].ctype;
											nyy+=nyi; nxx+=nxi;
											front1 = pmap[y+2*nyi+nyy][x+2*nxi+nxx];
											ftype1 = front1 & 0xFF;
										}
									}
									continue;
								case 28:
									if (noturn)
										continue;
									if (!max_turn)
										goto break1a;
									nxx += nxi; nyy += nyi;
									tmp[1] = parts[r].tmp & 0xF;
									switch (tmp[1])
									{
									case 0: // turn right
										tmp[0] = nxi;
										nxi = -nyi;
										nyi = tmp[0];
										break;
									case 1: // turn left
										tmp[0] = nxi;
										nxi = nyi;
										nyi = -tmp[0];
										break;
									case 2: // "/" reflect
										tmp[0] = nxi;
										nxi = nyi;
										nyi = tmp[0];
										break;
									case 3: // "\" reflect
										tmp[0] = nxi;
										nxi = -nyi;
										nyi = -tmp[0];
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
									case 8:
									case 9:
										while (nxx += nxi, nyy += nyi, BOUNDS_CHECK)
										{
											int front1 = pmap[y+nyy][x+nxx];
											if (!front1) goto break1a;
											else if ((front1 & 0xFF) == PT_FILT)
											{
												parts[front1>>8].life = 4;
											}
											else if ((front1 & 0xFF) == PT_ARAY)
											{
												float ftemp = parts[front1>>8].temp + (tmp[1] == 8 ? 1 : -1) * parts[r].tmp2;
												parts[front1>>8].temp = restrict_flt(ftemp, MIN_TEMP, MAX_TEMP);
												goto break1a;
											}
											else goto break1a;
										}
										goto break1a;
									}
									nxx -= nxi; nyy -= nyi;
									max_turn--;
									continue;
								case 32:
									tmp[0] = parts[r].tmp;
									tmp[1] = parts[r].tmp2;
									switch (tmp[1])
									{
									case 0:
										// temp_z1[8] = noturn;
										noturn = (tmp[0] >> (3 * noturn)) & 0x7; // Easier for inputing hexadecimal?
										if (noturn == 3)
										{
											goto break1a;
										}
										break;
									case 1:
										// temp_z1[8] = tmp2 = nostop | (destroy << 1);
										nostop  = (tmp[0] >> (nostop  ? 1 : 0)) & 0x1;
										destroy = (tmp[0] >> (destroy ? 3 : 2)) & 0x1;
										break;
									case 2:
										// temp_z1[8] = spc_conduct;
										spc_conduct = tmp[0];
										break;
									case 3:
										// temp_z1[8] = ray_less;
										ray_less = ((tmp[0] ^ 1) >> ray_less) & 0x1;
										break;
									case 4:
										tmpz2 = tmp[0];
										break;
									case 5:
										pass_wall = !pass_wall;
										break;
									case 6:
										{
											nxx += nxi; nyy += nyi;
											int front1 = pmap[y+nyi+nyy][x+nxi+nxx];
											switch (front1 & 0xFF)
											{
											case PT_FRAY:
												tmpz2 += parts[front1 >> 8].tmp;
											break;
											case PT_INVIS:
												tmpz2 += (int)(sim->sim_max_pressure + 0.5f);
											break;
											}
										}
										break;
									case 7: // remove front SPRK
										{
											nxx += 2*nxi; nyy += 2*nyi;
											int front1 = pmap[y+nyy][x+nxx];
											int f_type = front1 & 0xFF;
											if ((front1 & 0xFF) == PT_SPRK)
											{
												f_type = parts[front1>>8].ctype;
												if (f_type <= 0 || f_type >= PT_NUM || !sim->elements[f_type].Enabled)
													f_type = PT_METL;
												sim->part_change_type(front1>>8, x+nxi+nxx, y+nyi+nyy, f_type);
												parts[front1>>8].ctype = PT_NONE; // clear ctype
												if (f_type == PT_SWCH)
													parts[front1>>8].life = 19; // keep SWCH on
											}
											else if (f_type == PT_SWCH && parts[front1>>8].life >= 10)
											{
												parts[front1>>8].life = 19; // keep SWCH on
											}
											if (sim->elements[f_type].Properties & (PROP_CONDUCTS|PROP_CONDUCTS_SPEC))
												parts[front1>>8].life = 8;
											goto break1a;
										}
										break;
									}
									tmpz = 1;
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
								if (rt == PT_INSL || rt == PT_INDI)
									break;
								if (spc_conduct > 0)
								{
									if (rt != PT_INWR && (rt != PT_SPRK || parts[r].ctype != PT_INWR))
									{
										switch (spc_conduct)
										{
										case 1:
											sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK);
											goto break1a;
										case 2:
											sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK);
											if (!(parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (sim->elements[parts[r].ctype].Properties&PROP_CONDUCTS)))
												goto break1a;
										case 3:
											sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK);
											break;
										case 4:
											if (rt == PT_INST)
											{
												docontinue = nostop;
												sim->FloodINST(x+nxi+nxx, y+nyi+nyy, PT_SPRK, PT_INST);
											}
											else
												sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK);
											continue;
										case 5:
											if (rt == PT_WOOD)
												sim->part_change_type(r, x+nxi+nxx, y+nyi+nyy, PT_SAWD);
											else if (rt == PT_ARAY)
												parts[r].temp = parts[i].temp;
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
										if (!tmpz && (nyy != 0 || nxx != 0))
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
									if (parts[r].tmp)
									{
										//Cause STOR to release
										for (int ry1 = 1; ry1 >= -1; ry1--)
										{
											for (int rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1 - rx1 + 1)
											{
												int np = sim->create_part(-1, x + nxi + nxx + rx1, y + nyi + nyy + ry1, parts[r].tmp&0xFF);
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
													break;
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
								else if (rt != PT_INWR && (rt != PT_SPRK || parts[r].ctype != PT_INWR) && rt != PT_ARAY && rt != PT_WIFI && !(rt == PT_SWCH && parts[r].life >= 10))
								{
									if (nyy!=0 || nxx!=0)
										sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK);

									if (!(nostop && parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (sim->elements[parts[r].ctype].Properties&PROP_CONDUCTS)))
										docontinue = 0;
									else
										docontinue = 1;
								}
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
								else if (rt==PT_STOR || rt==PT_INWR || (rt==PT_SPRK && parts[r].ctype==PT_INWR) || rt==PT_ARAY || rt==PT_WIFI || rt==PT_FILT || (rt==PT_SWCH && parts[r].life>=10))
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
