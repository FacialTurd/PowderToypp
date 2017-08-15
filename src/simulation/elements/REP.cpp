#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_REPP PT_REPP 194

Element_REPP::Element_REPP()
{
	Identifier = "DEFAULT_PT_REPP";
	Name = "REP";
	Colour = PIXPACK(0xFFA000);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 80;

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 70;
	Description = "Replicating powder";

	Properties = TYPE_PART|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_REPP::update;
	Graphics = NULL;
}

//#TPT-Directive ElementHeader Element_REPP static int update(UPDATE_FUNC_ARGS)
int Element_REPP::update(UPDATE_FUNC_ARGS)
{
	float tempTemp = parts[i].temp;
	if (parts[i].life <= 1)
	{
		for (int s = parts[i].tmp; s > 0; s--)
		{
			int rr = sim->create_part(-1, x + rand()%7-3, y + rand()%7-3, PT_REPP);
			if (rr >= 0)
			{
				parts[rr].temp = tempTemp;
				parts[rr].tmp  = parts[i].tmp + ((rand() % 10 - 1) >> 3);
				if (parts[rr].tmp > 8)
					parts[rr].tmp = 8;
				parts[rr].life = 32 + rand() % 8;
			}
		}
		sim->kill_part(i);
		return 1;
	}
	return 0;
}

Element_REPP::~Element_REPP() {}
