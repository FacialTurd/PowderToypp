#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_E187 PT_E187 187
Element_E187::Element_E187()
{
	Identifier = "DEFAULT_PT_E187";
	Name = "E187";
	Colour = PIXPACK(0xB038D8);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
#if (defined(DEBUG) || defined(SNAPSHOT)) && MOD_ID == 0
	Enabled = 1;
#else
	Enabled = 0;
#endif

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 24;

	Temperature = R_TEMP-2.0f	+273.15f;
	HeatConduct = 29;
	Description = "Experimental element. acts like ISOZ.";

	Properties = TYPE_LIQUID | PROP_LIFE_DEC | PROP_NOSLOWDOWN | PROP_TRANSPARENT;
	Properties2 = PROP_CTYPE_INTG;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_E187::update;
	Graphics = &Element_E187::graphics;
}

//#TPT-Directive ElementHeader Element_E187 static int update(UPDATE_FUNC_ARGS)
int Element_E187::update(UPDATE_FUNC_ARGS)
{ // for both 'E187' and 'E188'
	int r, rx, ry, stmp, stmp2, rt;
	switch (parts[i].ctype) {
	case 0:
		{
			stmp = parts[i].tmp;
			if (!parts[i].life)
			{
				if (!(rand()%10000) && !(stmp & 1))
				{
					Element_E187::createPhotons(sim, i, x, y, stmp, parts);
				}
				r = sim->photons[y][x];
				if ((r & 0xFF) == PT_PHOT && !(rand()%100))
				{
					Element_E187::createPhotons(sim, i, x, y, stmp, parts);
				}
			}
			if (stmp & 4)
			{
				parts[i].vx = 0; parts[i].vy = 0;
				if (parts[i].temp >= 300.0f)
					parts[i].tmp &= ~0x4;
			}
			else
			{
				if (parts[i].temp < 160.0f)
					parts[i].tmp |= 0x4;
			}
		}
		break;
	case 1:
		if (parts[i].tmp)
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
					if (BOUNDS_CHECK)
					{
						r = pmap[y+ry][x+rx];
						if (!r)
							break;
						switch (r & 0xFF)
						{
						case PT_GLOW:
							if (parts[r>>8].life > 0 && !(sim->elements[PT_GLOW].Properties & PROP_LIFE_DEC))
								parts[r>>8].life --;
							break;
						case PT_E187:
							if (!parts[r>>8].ctype && !(rand()%200))
							{
								parts[r>>8].tmp &= 0xFFFFFFFE;
								sim->pv[y/CELL][x/CELL] += 3.0f;
							}
							break;
						default:
							break;
						}
					}
		break;
	default:
		break;
	}
	return 0;
}

//#TPT-Directive ElementHeader Element_E187 static int graphics(GRAPHICS_FUNC_ARGS)
int Element_E187::graphics(GRAPHICS_FUNC_ARGS)
{
	switch(cpart->ctype) {
	case 1:
		*colr = 0xFF;
		*colg = 0x80;
		*colb = 0xBB;
		break;
	case 0:
	default:
		break;
	}

	return 0;
}

//#TPT-Directive ElementHeader Element_E187 static int createPhotons(Simulation* sim, int i, int x, int y, int tmp, Particle *parts)
int Element_E187::createPhotons(Simulation* sim, int i, int x, int y, int tmp, Particle *parts)
{
	if (sim->pfree < 0)
		return 0;
	int np = sim->pfree;
	sim->pfree = parts[np].life;
	if (np > sim->parts_lastActiveIndex)
		sim->parts_lastActiveIndex = np;
	float r2, r3;
	const int cooldown = 15;

	parts[i].life = cooldown;
	parts[i].tmp |= 0x1;

	r2 = (rand()%128+128)/127.0f;
	r3 = (rand()%360)*3.1415926f/180.0f;

	// write particle data
	// tmp = 0 or 1 emits white PHOT
	// tmp = 2 or 3 emits rainbow-colored PHOT
	parts[np].type = PT_PHOT;
	parts[np].life = rand()%480+480;
	parts[np].ctype = tmp & 2 ? 0x1F<<(rand()%26) : 0x3FFFFFFF;
	parts[np].x = (float)x;
	parts[np].y = (float)y;
	parts[np].vx = r2*cosf(r3);
	parts[np].vy = r2*sinf(r3);
	parts[np].temp = parts[i].temp + 20;
	parts[np].tmp = 0x1;
	parts[np].pavg[0] = parts[np].pavg[1] = 0.0f;
	parts[np].dcolour = 0; // clear deco color
	
	sim->photons[y][x] = PT_PHOT | (np<<8);
	return 0;
}

Element_E187::~Element_E187() {}
