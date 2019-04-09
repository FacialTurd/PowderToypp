#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_BTRY PT_BTRY 53
Element_BTRY::Element_BTRY()
{
	Identifier = "DEFAULT_PT_BTRY";
	Name = "BTRY";
	Colour = PIXPACK(0x858505);
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

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 251;
	Description = "Battery. Generates infinite electricity.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2273.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &Element_BTRY::update;
}

//#TPT-Directive ElementHeader Element_BTRY static int update(UPDATE_FUNC_ARGS)
int Element_BTRY::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, pavg;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry) && abs(rx)+abs(ry)<4)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r); r = ID(r);
				if (!sim->parts_avg_elec(i, r))
				{
					if ((sim->elements[rt].Properties&(PROP_CONDUCTS|PROP_INSULATED)) == PROP_CONDUCTS && parts[r].life==0)
					{
						parts[r].life = 4;
						parts[r].ctype = rt;
						sim->part_change_type(r,x+rx,y+ry,PT_SPRK);
					}
				}
			}
	return 0;
}


Element_BTRY::~Element_BTRY() {}
