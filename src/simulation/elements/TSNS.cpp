#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_TSNS PT_TSNS 164
Element_TSNS::Element_TSNS()
{
	Identifier = "DEFAULT_PT_TSNS";
	Name = "TSNS";
	Colour = PIXPACK(0xFD00D5);
	MenuVisible = 1;
	MenuSection = SC_SENSOR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.96f;
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

	DefaultProperties.tmp2 = 2;
	HeatConduct = 0;
	Description = "Temperature sensor, creates a spark when there's a nearby particle with a greater temperature.";

	Properties = TYPE_SOLID;
	Properties2 = PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_TSNS::update;
}

//#TPT-Directive ElementHeader Element_TSNS static int update(UPDATE_FUNC_ARGS)
int Element_TSNS::update(UPDATE_FUNC_ARGS)
{
	int rd = parts[i].tmp2;
	if (rd > 25)
		parts[i].tmp2 = rd = 25;
	if (parts[i].life)
	{
		parts[i].life = 0;
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					int rt = TYP(r); r = ID(r);
					if (!sim->parts_avg_elec(i, r))
					{
						if ((sim->elements[rt].Properties & (PROP_CONDUCTS | PROP_INSULATED)) == PROP_CONDUCTS && partsi(r).life == 0)
						{
							parts[r].life = 4;
							parts[r].ctype = rt;
							sim->part_change_type(r, x+rx, y+ry, PT_SPRK);
						}
					}
				}
	}
	bool setFilt = false;
	int photonWl = 0;
	for (int rx = -rd; rx <= rd; rx++)
		for (int ry = -rd; ry <= rd; ry++)
			if (x + rx >= 0 && y + ry >= 0 && x + rx < XRES && y + ry < YRES && (rx || ry))
			{
				int r = pmap[y+ry][x+rx], rt;
				if (!r)
					r = sim->photons[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);
				if (parts[i].tmp == 0 && TYP(r) != PT_TSNS && TYP(r) != PT_METL && parts[ID(r)].temp > parts[i].temp)
					parts[i].life = 1;
				if (parts[i].tmp == 2 && TYP(r) != PT_TSNS && TYP(r) != PT_METL && parts[ID(r)].temp < parts[i].temp)
 	 	 			parts[i].life = 1;
				if (parts[i].tmp == 1 && rt != PT_TSNS && rt != PT_FILT)
				{
					setFilt = true;
					photonWl = partsi(r).temp;
				}
			}
	if (setFilt)
	{
		int nx, ny;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					photonWl += 0x10000000;
					Element_MULTIPP::setFilter(sim, x+rx, y+ry, rx, ry, photonWl);
				}
	}
	return 0;
}



Element_TSNS::~Element_TSNS() {}
