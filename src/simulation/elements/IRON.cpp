#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_IRON PT_IRON 76
Element_IRON::Element_IRON()
{
	Identifier = "DEFAULT_PT_IRON";
	Name = "IRON";
	Colour = PIXPACK(0x707070);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
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
	Meltable = 1;
	Hardness = 50;

	Weight = 100;

	HeatConduct = 251;
	Description = "Rusts with salt, can be used for electrolysis of WATR.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;
	Properties2 |= PROP_ELEC_HEATING;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1687.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &Element_IRON::update;
}

//#TPT-Directive ElementHeader Element_IRON static int update(UPDATE_FUNC_ARGS)
int Element_IRON::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	bool rusting = false;
	if (parts[i].life)
		return 0;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_SALT:
					if (!(rand()%47))
						rusting = true;
					break;
				case PT_SLTW:
					if (!(rand()%67))
						rusting = true;
					break;
				case PT_WATR:
					if (!(rand()%1200))
						rusting = true;
					break;
				case PT_O2:
					if (!(rand()%250))
						rusting = true;
					break;
				case PT_LO2:
					rusting = true;
					break;
				case ELEM_MULTIPP:
					if (partsi(r).life == 2 && !(~partsi(r).tmp & 0x9))
						return 0; // anti-rusting stuff
					break;
#if 0
				case PT_CHRM: // iron + chromium = stainless steel
					return 0;
#endif
				default:
					break;
				}
			}
	if (rusting)
	{
		sim->part_change_type(i,x,y,PT_BMTL);
		parts[i].tmp=(rand()%10)+20;
	}
	return 0;
}

//#TPT-Directive ElementHeader Element_IRON static void makeAlloy(Simulation * sim, int i, int rt, int r)
void Element_IRON::makeAlloy (Simulation * sim, int i, int rt, int r)
{
	if (((rt==PT_COAL) || (rt==PT_BCOL)) && !(rand()%500))
	{
		sim->parts[i].ctype = PT_METL;
		sim->kill_part(r);
	}
	else 
	{
		// TODO: other IRON-based alloy here
	}
}

Element_IRON::~Element_IRON() {}
