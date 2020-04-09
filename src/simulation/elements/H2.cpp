#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_H2 PT_H2 148
Element_H2::Element_H2()
{
	Identifier = "DEFAULT_PT_H2";
	Name = "HYGN";
	Colour = PIXPACK(0x5070FF);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.00f;
	Diffusion = 3.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	HeatConduct = 251;
	Description = "Hydrogen. Combusts with OXYG to make WATR. Undergoes fusion at high temperature and pressure.";

	Properties = TYPE_GAS | PROP_TRANSPARENT;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_H2::update;
}

//#TPT-Directive ElementHeader Element_H2 static int update(UPDATE_FUNC_ARGS)
int Element_H2::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt;
	float temp = parts[i].temp, pres = sim->pv[y/CELL][x/CELL];

	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);
				r = ID(r);

				if (pres > 8.0f && rt == PT_DESL) // This will not work. DESL turns to fire above 5.0 pressure
				{
					sim->part_change_type(r,x+rx,y+ry,PT_WATR);
					sim->part_change_type(i,x,y,PT_OIL);
					return 1;
				}
				if (pres > 45.0f)
				{
					if (parts[r].temp > 2273.15)
						continue;
				}
				else
				{
					bool burn = false;
					if (rt==PT_FIRE)
					{
						if(parts[r].tmp&0x02)
							parts[r].temp=3473.0f;
						else
							parts[r].temp=2473.15f;
						parts[r].tmp |= 1;
						burn = true;
					}
					else if ((rt==PT_PLSM && !(parts[r].tmp&4)) || (rt==PT_LAVA && parts[r].ctype != PT_BMTL))
					{
						burn = true;
					}
					if (burn)
					{
						temp += rand() % 100;
						rt = temp >= sim->elements[PT_FIRE].HighTemperature ? PT_PLSM : PT_FIRE;
						sim->create_part(i,x,y,rt);
						parts[i].temp = temp;
						parts[i].tmp |= 1;
						return 1;
					}
				}
			}
	if (temp > 2273.15 && pres > 50.0f)
	{
		if (!(rand()%5))
		{
			int j;
			sim->create_part(i,x,y,PT_NBLE);
			parts[i].tmp = 0x1;

			j = sim->create_part(-3,x,y,PT_NEUT);
			if (j>-1)
				parts[j].temp = temp;
			if (!(rand()%10))
			{
				j = sim->create_part(-3,x,y,PT_ELEC);
				if (j>-1)
					parts[j].temp = temp;
			}
			j = sim->create_part(-3,x,y,PT_PHOT);
			if (j>-1)
			{
				parts[j].ctype = 0x7C0000;
				parts[j].temp = temp;
				parts[j].tmp = 0x1;
			}
			rx = x+rand()%3-1, ry = y+rand()%3-1, rt = TYP(pmap[ry][rx]);
			if (sim->can_move[PT_PLSM][rt] || rt == PT_H2)
			{
				j = sim->create_part(-3,rx,ry,PT_PLSM);
				if (j>-1)
				{
					parts[j].temp = temp;
					parts[j].tmp |= 4;
				}
			}
			parts[i].temp = temp+750+rand()%500;
			sim->pv[y/CELL][x/CELL] += 30;
			return 1;
		}
	}
	return 0;
}


Element_H2::~Element_H2() {}
