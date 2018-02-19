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

#define PFLAG_EMITTED			0x1
#define PFLAG_NO_SPLIT			0x1
#define PFLAG_EMIT_RAINBOW		0x2
#define PFLAG_SOLID_STATE		0x4
#define PFLAG_DILUTED_SHIFT		4
#define PFLAG_DILUTED_BITS		4
#define PFLAG_DILUTED_LEVELS	((1<<(PFLAG_DILUTED_BITS))-1)

//#TPT-Directive ElementHeader Element_E187 static int update(UPDATE_FUNC_ARGS)
int Element_E187::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, stmp, stmp2, rt;
	int rndstore;
	static int table1[8] = {-2,-1,-1,0,0,1,1,2};

	stmp = parts[i].tmp;
	if (!parts[i].life)
	{
		int pres = sim->pv[y/CELL][x/CELL];
		if ((pres < -5) && !(rand()%10000) && !(stmp & PFLAG_EMITTED))
		{
			Element_E187::createPhotons(sim, i, x, y, stmp, parts);
		}
		r = sim->photons[y][x];
		if (TYP(r) == PT_PHOT && ((pres < -5) ? -10 : 5 - pres) > (rand()%1000))
		{
			Element_E187::createPhotons(sim, i, x, y, stmp, parts);
		}
	}
	if (stmp & PFLAG_SOLID_STATE)
	{
		parts[i].vx = 0; parts[i].vy = 0;
		if (parts[i].temp >= 300.0f)
			parts[i].tmp &= ~PFLAG_SOLID_STATE;
	}
	else
	{
		if (parts[i].temp < 160.0f)
			parts[i].tmp |= PFLAG_SOLID_STATE;

		for (int trade = 0; trade < 5; trade++) // mixing this with GLOW/ISOZ
		{
			if (!(trade%2)) rndstore = rand();
			rx = table1[rndstore&7];
			rndstore >>= 3;
			ry = table1[rndstore&7];
			rndstore >>= 3;
			r = pmap[y+ry][x+rx];
			if (!(r && (rx || ry))) continue;
			rt = TYP(r);
			if (rt == PT_GLOW || rt == PT_ISOZ)
			{
				int diluted_level = (parts[i].tmp >> PFLAG_DILUTED_SHIFT) & PFLAG_DILUTED_LEVELS;
				if (rt == PT_ISOZ && (diluted_level < PFLAG_DILUTED_LEVELS) && !parts[i].life)
				{
					parts[i].tmp += 1 << PFLAG_DILUTED_SHIFT;
					parts[i].life = 20 + (rand() % 50);
					partsi(r).tmp = parts[i].tmp | PFLAG_EMITTED;
					partsi(r).life = 20 + (rand() % 50);
					sim->part_change_type(part_ID(r), x+rx, y+ry, parts[i].type);
					continue;
				}
				parts[i].x = partsi(r).x;
				parts[i].y = partsi(r).y;
				partsi(r).x = x;
				partsi(r).y = y;
				pmap[y][x] = r;
				pmap[y+ry][x+rx] = PMAP(i, parts[i].type);
				break;
			}
			else if (rt == parts[i].type)
			{
				int swaptmp = parts[i].tmp;
				parts[i].tmp = partsi(r).tmp;
				partsi(r).tmp = swaptmp;
				continue;
			}
		}
	}

	return 0;
}

//#TPT-Directive ElementHeader Element_E187 static int graphics(GRAPHICS_FUNC_ARGS)
int Element_E187::graphics(GRAPHICS_FUNC_ARGS)
{
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
	parts[i].tmp |= PFLAG_EMITTED;

	r2 = (rand()%128+128)/127.0f;
	r3 = (rand()%360)*3.1415926f/180.0f;

	// write particle data
	// tmp = 0 or 1 emits white PHOT
	// tmp = 2 or 3 emits rainbow-colored PHOT
	parts[np].type = PT_PHOT;
	parts[np].life = rand()%480+480;
	parts[np].ctype = (tmp & PFLAG_EMIT_RAINBOW) ? 0x1F<<(rand()%26) : 0x3FFFFFFF;
	parts[np].x = (float)x;
	parts[np].y = (float)y;
	parts[np].vx = r2*cosf(r3);
	parts[np].vy = r2*sinf(r3);
	parts[np].temp = parts[i].temp + 20;
	parts[np].tmp = PFLAG_NO_SPLIT;
	parts[np].pavg[0] = parts[np].pavg[1] = 0.0f;
	parts[np].dcolour = 0; // clear deco color
	
	sim->photons[y][x] = PMAP(i, PT_PHOT);
	return 0;
}

Element_E187::~Element_E187() {}
