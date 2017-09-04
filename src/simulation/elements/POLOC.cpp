#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_POLC PT_POLC 188

/*
TODO: 
	- "muted version" a powder created by mixing POLC with ? that is weaker
*/

Element_POLC::Element_POLC()
{
	Identifier = "DEFAULT_PT_POLC";
	Name = "POL2";
	Colour = PIXPACK(0x447722);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.4f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 0;
	PhotonReflectWavelengths = 0x000FF200;

	Weight = 90;

	Temperature = 388.15f; 
	HeatConduct = 251;
	Description = "Experimental element. Some kind of nuclear fuel, POLO decay catalyst";

	Properties = TYPE_PART|PROP_NEUTPASS|PROP_RADIOACTIVE|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 526.95f;
	HighTemperatureTransition = PT_LAVA;

	Update = &Element_POLC::update;
	Graphics = &Element_POLC::graphics;
}

//#TPT-Directive ElementHeader Element_POLC static int update(UPDATE_FUNC_ARGS)
int Element_POLC::update(UPDATE_FUNC_ARGS)
{
	int r, s, rx, ry, rr, sctype, stmp, trade;
	int is_warp;
	const int cooldown = 15;
	const int limit = 20;
	float tempTemp, tempPress;
	rr = sim->photons[y][x];
	stmp = parts[i].tmp;

	if (stmp < limit && !parts[i].life)
	{
		sctype = parts[i].ctype & 0xFF; // don't create SPC_AIR
		is_warp = (sctype == PT_WARP);
		if (((is_warp ? (int)(parts[i].temp) >> 6 : 1) > rand() % 8192) && !stmp)
		{
			if (!sctype)
				s = sim->create_part(-3, x, y, PT_ELEC);
			else if (sim->elements[sctype].Properties & TYPE_ENERGY)
				s = sim->create_part(-3, x, y, sctype);
			else
			{
				rx = rand() % 5 - 2;
				ry = rand() % 5 - 2;
				if (sim->IsWallBlocking(x+rx, y+ry, sctype))
					s = -1; // it's wall blocked
				else
					s = sim->create_part(-1, x+rx, y+ry, sctype);
			}
			if (s >= 0)
			{
				parts[i].life = cooldown;
				parts[i].tmp = 1;
				if (parts[i].temp < 520.0f)
					parts[i].temp += 5.0f;
				parts[s].temp = parts[i].temp;
				if (sctype == PT_GRVT)
					parts[s].tmp = 0;
				else if (is_warp)
					parts[s].tmp2 = 3000 + rand() % 10000;
			}
		}
		if (rr && ((rr & 0xFF) == PT_ELEC || (rr & 0xFF) == PT_E186) && !(rand()%80) && ((stmp - 11) < rand() % 10))
		{
			s = -1;
			if (rand() % 10)
			{
				if (!sctype)
					s = sim->create_part(-3, x, y, PT_ELEC);
				else if (sim->elements[sctype].Properties & TYPE_ENERGY)
					s = sim->create_part(-3, x, y, sctype);
			}
			else
				s = sim->create_part(-3, x, y, PT_E186);
			parts[i].life = cooldown;
			parts[i].tmp ++;
			if (parts[i].temp < 520.0f)
				parts[i].temp += 5.0f;

			parts[rr>>8].temp = parts[i].temp;
			if (s >= 0)
			{
				parts[s].ctype = sctype;
				parts[s].temp = parts[i].temp;
				float veloc_multipler = (rand()%102 + 9950.0f) / 10000.0f;
				parts[s].vx = parts[rr>>8].vx * veloc_multipler;
				parts[s].vy = parts[rr>>8].vy * veloc_multipler;
				if (sctype == PT_GRVT)
					parts[s].tmp = 0;
			}
		}
	}
	if (!(rand() % 10))
	{
		int rndstore, b = 0;
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r) continue;
					switch (r & 0xFF)
					{
					case PT_PLUT:
						if (parts[r>>8].tmp2 >= 10)
						{
							parts[r>>8].tmp = 0;
							parts[r>>8].tmp2 = 0;
							sim->part_change_type(r>>8, x+rx, y+ry, PT_POLO);
						}
						break;
					case PT_POLO:
						if (!b)
							b = 4, rndstore = rand();
						else 
							b--;
						parts[i].temp *= 0.98;
						if (!(rndstore&7))
						{
							parts[r>>8].tmp3 = 20;
						}
						rndstore >>= 3;
						break;
					case PT_POLC: // don't interacting itself
						break;
					}
				}
	}
	return 0;
}


//#TPT-Directive ElementHeader Element_POLC static int graphics(GRAPHICS_FUNC_ARGS)
int Element_POLC::graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp >= 20) {
		*colr = 0x70;
		*colg = 0x70;
		*colb = 0x70;
	} else if (cpart->tmp >= 10) {
		*colr = (0x71 + *colr) / 2;
		*colg = (0x71 + *colg) / 2;
		*colb = (0x71 + *colb) / 2;
	}
	return 0;
}

Element_POLC::~Element_POLC() {}
