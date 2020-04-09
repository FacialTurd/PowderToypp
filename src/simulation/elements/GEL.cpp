#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_GEL PT_GEL 142
Element_GEL::Element_GEL()
{
	Identifier = "DEFAULT_PT_GEL";
	Name = "GEL";
	Colour = PIXPACK(0xFF9900);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f  * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 35;

	DefaultProperties.temp = R_TEMP-2.0f  +273.15f;
	HeatConduct = 29;
	Description = "Gel. A liquid with variable viscosity and heat conductivity.";

	Properties = TYPE_LIQUID|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_GEL::update;
	Graphics = &Element_GEL::graphics;
}

//#TPT-Directive ElementHeader Element_GEL static int update(UPDATE_FUNC_ARGS)
int Element_GEL::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt;
	bool gel;
	int absorbChanceDenom;
	if (parts[i].tmp>100) parts[i].tmp = 100;
	if (parts[i].tmp<0) parts[i].tmp = 0;
	absorbChanceDenom = parts[i].tmp*10 + 500;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				gel=false;
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r); r = ID(r);
				//Desaturation
				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_FRZW:
					if (parts[i].tmp<100 && 500>rand()%absorbChanceDenom)
					{
						parts[i].tmp++;
						sim->kill_part(r);
					}
					break;
				case PT_PSTE:
					if (parts[i].tmp<100 && 20>rand()%absorbChanceDenom)
					{
						parts[i].tmp++;
						sim->create_part(r, x+rx, y+ry, PT_CLST);
					}
					break;
				case PT_SLTW:
					if (parts[i].tmp<100 && 50>rand()%absorbChanceDenom)
					{
						parts[i].tmp++;
						if (rand()%4)
							sim->kill_part(r);
						else
							sim->part_change_type(r, x+rx, y+ry, PT_SALT);
					}
					break;
				case PT_CBNW:
					if (parts[i].tmp<100 && 100>rand()%absorbChanceDenom)
					{
						parts[i].tmp++;
						sim->part_change_type(r, x+rx, y+ry, PT_CO2);
					}
					break;
				case PT_SPNG:
					// Concentration diffusion
					if (parts[r].life>0 && parts[i].tmp<100 && ((parts[r].life+1)>parts[i].tmp))
					{
						// SPNG -> GEL
						parts[r].life--;
						parts[i].tmp++;
					}
					else if (parts[i].tmp>0 && (parts[r].life+1)<parts[i].tmp)
					{
						// SPNG <- GEL (saturation limit of SPNG is ignored here)
						parts[r].life++;
						parts[i].tmp--;
					}
					break;
				case PT_GEL:
					if ((parts[r].tmp+1)<parts[i].tmp)
					{
						parts[r].tmp++;
						parts[i].tmp--;
					}
					gel = true;
					break;
				case ELEM_MULTIPP:
					if (parts[r].life == 38 && (parts[r].tmp2 & 7) != 4)
					{
						int * ctype = &parts[r].ctype;
						if (!(*ctype)) *ctype = PT_WATR;
						if (*ctype == PT_WATR || *ctype == PT_DSTW)
						{
							parts[r].tmp += parts[i].tmp;
							parts[i].tmp = 0;
						}
						else if (*ctype == PT_GEL)
						{
							if (parts[i].tmp <= 1)
							{
								if (parts[i].tmp <= 0)
									sim->kill_part(i);
								else
									sim->part_change_type(i, x, y, PT_WATR);
								parts[r].tmp++;
								return 1;
							}
							else // merging N GELs
							{
								int rndstore = rand();
								int mx, my, mr, ms, mi;
								mx = x + sim->portal_rx[rndstore&7];
								my = y + sim->portal_ry[rndstore&7];
								mr = pmap[my][mx]; mi = ID(mr);
								if (TYP(mr) == PT_GEL)
								{
									ms = parts[mi].tmp + parts[i].tmp;
									if (ms < 100)
									{
										parts[i].tmp = ms;
										sim->kill_part(mi);
										parts[r].tmp++;
									}
								}
							}
						}
					}
					break;
				case PT_PINVIS:
					r = parts[r].tmp4;
					if (!r) continue;
					rt = TYP(r);
					if (rt == PT_GEL) gel = true;
					break;
				default:
					break;
				}
				float dx, dy;
				dx = parts[i].x - parts[r].x;
				dy = parts[i].y - parts[r].y;

				//Stickiness
				if ((dx*dx + dy*dy)>1.5 && (gel || !sim->elements[rt].Falldown || (fabs((float)rx)<2 && fabs((float)ry)<2)))
				{
					float per, nd;
					nd = dx*dx + dy*dy - 0.5;
					per = 5*(1 - parts[i].tmp/100)*(nd/(dx*dx + dy*dy + nd) - 0.5);
					if (sim->elements[rt].Properties&TYPE_LIQUID)
						per *= 0.1;
					dx *= per; dy *= per;
					parts[i].vx += dx;
					parts[i].vy += dy;
					if ((sim->elements[rt].Properties&TYPE_PART) || rt==PT_GOO)
					{
						parts[r].vx -= dx;
						parts[r].vy -= dy;
					}
				}
			}
	return 0;
}



//#TPT-Directive ElementHeader Element_GEL static int graphics(GRAPHICS_FUNC_ARGS)
int Element_GEL::graphics(GRAPHICS_FUNC_ARGS)

{
	int q = cpart->tmp;
	*colr = q*(32-255)/120+255;
	*colg = q*(48-186)/120+186;
	*colb = q*208/120;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}



Element_GEL::~Element_GEL() {}
