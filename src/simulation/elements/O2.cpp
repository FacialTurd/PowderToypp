#include "simulation/Elements.h"

// in real life, burning is actually a reaction of oxygen and the burning material.
// TODO: New element: realistic oxygen gas.

//#TPT-Directive ElementClass Element_O2 PT_O2 61
Element_O2::Element_O2()
{
	Identifier = "DEFAULT_PT_O2";
	Name = "OXYG";
	Colour = PIXPACK(0x80A0FF);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 3.0f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 70;
	Description = "Oxygen gas. Ignites easily.";

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 90.0f;
	LowTemperatureTransition = PT_LO2;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_O2::update;
}

//#TPT-Directive ElementHeader Element_O2 static int update(UPDATE_FUNC_ARGS)
int Element_O2::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt;
	float temp = parts[i].temp;
	bool burn = false;

	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);
				if (rt==PT_FIRE)
				{
					parts[ID(r)].temp+=(rand()%100);
					if(parts[ID(r)].tmp&0x01)
						parts[ID(r)].temp=3473;
					parts[ID(r)].tmp |= 2;
					burn = true;
				}
				else if (rt==PT_PLSM && !(parts[ID(r)].tmp&4))
				{
					burn = true;
				}
			}
	if (burn)
	{
		temp += rand() % 100;
		rt = temp >= sim->elements[PT_FIRE].HighTemperature ? PT_PLSM : PT_FIRE;
		sim->create_part(i,x,y,rt);
		parts[i].temp = temp;
		parts[i].tmp |= 2;
	}
	if (temp > 9973.15 && sim->pv[y/CELL][x/CELL] > 250.0f)
	{
		int gravPos = ((y/CELL)*(XRES/CELL))+(x/CELL);
		float gravx = sim->gravx[gravPos];
		float gravy = sim->gravy[gravPos];
		if (gravx*gravx + gravy*gravy > 400)
		{
			if (!(rand()%5))
			{
				int j;
				sim->create_part(i,x,y,PT_BRMT);

				j = sim->create_part(-3,x,y,PT_NEUT);
				if (j != -1)
					parts[j].temp = MAX_TEMP;
				j = sim->create_part(-3,x,y,PT_PHOT);
				if (j != -1)
				{
					parts[j].temp = MAX_TEMP;
					parts[j].tmp = 0x1;
				}
				rx = x+rand()%3-1, ry = y+rand()%3-1, r = TYP(pmap[ry][rx]);
				if (sim->can_move[PT_PLSM][r] || r == PT_O2)
				{
					j = sim->create_part(-3,rx,ry,PT_PLSM);
					if (j > -1)
					{
						parts[j].temp = MAX_TEMP;
						parts[j].tmp |= 4;
					}
				}
				j = sim->create_part(-3,x,y,PT_GRVT);
				if (j != -1)
					parts[j].temp = MAX_TEMP;
				parts[i].temp = MAX_TEMP;
				sim->pv[y/CELL][x/CELL] = 256;
			}
		}
	}
	return 0;
}


Element_O2::~Element_O2() {}
