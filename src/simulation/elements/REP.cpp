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
	Description = "Replicating powder.";

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
	if (parts[i].life < 1)
	{
		float tempTemp = parts[i].temp;
		int rnd = rand(), t = parts[i].type;
		int rx = (rnd % 7) - 3;
		rnd >>= 3;
		int ry = (rnd % 7) - 3;
		float mv = std::max((float)abs(rx), (float)abs(ry));
		bool b = false;
		if (sim->InBounds(x+rx, y+ry) && mv > ISTP)
		{
			// TODO: Simulation::CheckLine
			float dx = (float)rx * ISTP / mv;
			float dy = (float)ry * ISTP / mv;
			float xf = x, yf = y;
			for (;;)
			{
				xf += dx; yf += dy; mv -= ISTP;
				if (mv <= 0.0f)
					break;
				int xx = (int)(xf+0.5f), yy = (int)(yf+0.5f);
				int r = pmap[yy][xx];
				if ((r && (sim->elements[TYP(r)].Properties & TYPE_SOLID)) ||
					sim->IsWallBlocking(xx, yy, t) || sim->bmap[yy/CELL][xx/CELL] == WL_DESTROYALL)
				{
					b = true;
					break;
				}
			}
		}
		if (!b)
		{
			int np = sim->create_part(-1, x + rx, y + ry, t);
			if (np >= 0)
			{
				parts[np].temp = tempTemp;
			}
		}
		parts[i].tmp--;
		parts[i].life = 10;
	}
	if (parts[i].tmp <= 0)
	{
		sim->kill_part(i);
		return 1;
	}
	return 0;
}

Element_REPP::~Element_REPP() {}
