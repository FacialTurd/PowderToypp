#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_PROT PT_PROT 173
#define ID part_ID
Element_PROT::Element_PROT()
{
	Identifier = "DEFAULT_PT_PROT";
	Name = "PROT";
	Colour = PIXPACK(0x990000);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = -.99f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = -1;

	Temperature = R_TEMP+273.15f;
	HeatConduct = 61;
	Description = "Protons. Transfer heat to materials, and removes sparks.";

	Properties = TYPE_ENERGY;
	Properties2 |= PROP_ENERGY_PART | PROP_PASSTHROUGHALL;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_PROT::update;
	Graphics = &Element_PROT::graphics;
}

//#TPT-Directive ElementHeader Element_PROT static int update(UPDATE_FUNC_ARGS)
int Element_PROT::update(UPDATE_FUNC_ARGS)
{
	sim->pv[y/CELL][x/CELL] -= .003f;
	int under = pmap[y][x], ahead;
	if (TYP(under) == PT_PINVIS)
		under = partsi(under).tmp4;
	int utype = TYP(under), aheadT;
	int underI = ID(under), aheadI;
	bool find_E186_only = false;

	switch (utype)
	{
	case PT_SPRK:
	{
		//remove active sparks
		int sparked = parts[underI].ctype;
		if (sparked > 0 && sparked < PT_NUM && sim->elements[sparked].Enabled)
		{
			if (sparked == ELEM_MULTIPP) // never appearing SPRK (ELEM_MULTIPP)
				sparked = PT_METL;
			sim->part_change_type(underI, x, y, sparked);
			parts[underI].life = 44 + parts[underI].life;
			parts[underI].ctype = 0;
		}
		break;
	}
	case PT_DEUT:
		if ((-((int)sim->pv[y / CELL][x / CELL] - 4) + (parts[underI].life / 100)) > rand() % 200)
		{
			DeutImplosion(sim, parts[underI].life, x, y, restrict_flt(parts[underI].temp + parts[underI].life * 500, MIN_TEMP, MAX_TEMP), PT_PROT);
			sim->kill_part(underI);
		}
		break;
	case PT_LCRY:
		//Powered LCRY reaction: PROT->PHOT
		if (parts[underI].life > 5 && !(rand() % 10))
		{
			sim->part_change_type(i, x, y, PT_PHOT);
			parts[i].life *= 2;
			parts[i].ctype = 0x3FFFFFFF;
		}
		break;
	case PT_EXOT:
		if (parts[underI].ctype != PT_E186)
			parts[underI].ctype = PT_PROT;
		break;
	case PT_WIFI:
		float change;
		if (parts[i].temp < 173.15f) change = -1000.0f;
		else if (parts[i].temp < 273.15f) change = -100.0f;
		else if (parts[i].temp > 473.15f) change = 1000.0f;
		else if (parts[i].temp > 373.15f) change = 100.0f;
		else change = 0.0f;
		parts[underI].temp = restrict_flt(parts[underI].temp + change, MIN_TEMP, MAX_TEMP);
		break;
	case ELEM_MULTIPP:
		switch (parts[underI].life)
		{
		case 6:
			parts[i].temp = parts[underI].temp;
			goto no_temp_change;
		case 10:
			if (parts[underI].temp > 1273.0f)
			{
				sim->kill_part(i);
				return 1;
			}
			else if (parts[underI].temp < 73.15f)
			{
				parts[i].tmp2 &= ~1;
				parts[i].tmp2 |= (parts[underI].tmp >> 9) & 1;
				goto no_temp_change;
			}
			break;
		case 11:
			if (parts[underI].tmp2 == 1)
			{
				sim->part_change_type(i, x, y, PT_PHOT);
				parts[i].x = x;
				parts[i].y = y;
				parts[i].life *= 2;
				parts[i].ctype = 0x3FFFFFFF;
				return 1;
			}
			break;
		case 12:
			if ((parts[underI].tmp & 7) == 2)
			{
				bool vibr_found = false;
				int velSqThreshold = (int)(parts[underI].temp - (273.15f - 0.5f));
				if (velSqThreshold < 0)
					velSqThreshold = 0;
				else if (parts[underI].tmp == 10)
					velSqThreshold *= 100;
				int r, rx, ry, trade;

				for (trade = 0; trade < 5; trade++)
				{
					rx = rand()%5-2;
					ry = rand()%5-2;
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						switch (TYP(r))
						{
						case PT_BVBR:
						case PT_VIBR:
							parts[underI].tmp2 += partsi(r).tmp;
							partsi(r).tmp = 0;
							break;
						}
					}
				}

				if (vibr_found && sim->pv[y/CELL][x/CELL] > 0.0f)
				{
					parts[underI].tmp2 += sim->pv[y/CELL][x/CELL] * 7.0f,
					sim->pv[y/CELL][x/CELL] = 0.0f;
				}
				if (parts[underI].tmp2 < velSqThreshold)
				{
					parts[underI].tmp2 += parts[i].tmp + (int)(parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy + 0.5f);
					sim->kill_part(i);
					return 1;
				}
				parts[i].tmp += velSqThreshold;
				parts[underI].tmp2 -= velSqThreshold;
				goto finding_E186;
			}
			break;
		case 16:
			if (parts[underI].ctype == 5)
			{
				if (parts[underI].tmp >= 0x40)
				{
					parts[i].tmp2 &= ~2;
					((parts[underI].tmp & 0x40) && (parts[i].tmp2 |= 2));
				}
			}
			break;
		case 22:
			if ((parts[underI].tmp>>3) == 2)
				goto no_temp_change;
			break;
		case 33:
			{
				const float dtable[7] = {0.0f, 100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, (MAX_TEMP-MIN_TEMP)};
				float change;
				int temp = (int)(parts[i].temp - 323.15f) / 50;
				bool sign1 = (temp < 0); sign1 && (temp = -temp); temp > 6 && (temp = 6);
				change = dtable[temp] * (sign1 ? -1 : 1);
				parts[underI].temp = restrict_flt(parts[underI].temp + change, MIN_TEMP, MAX_TEMP);
			}
			goto no_temp_change;
		}
		break;
	case PT_NONE:
		//slowly kill if it's not inside an element
		if (parts[i].life)
		{
			if (!--parts[i].life)
				sim->kill_part(i);
		}
		break;
	default:
		//set off explosives (only when hot because it wasn't as fun when it made an entire save explode)
		if (parts[i].temp > 273.15f + 500.0f && (sim->elements[utype].Flammable || sim->elements[utype].Explosive || utype == PT_BANG))
		{
			sim->create_part(underI, x, y, PT_FIRE);
			parts[underI].temp += restrict_flt(sim->elements[utype].Flammable * 5, MIN_TEMP, MAX_TEMP);
			sim->pv[y / CELL][x / CELL] += 1.00f;
		}
		//prevent inactive sparkable elements from being sparked
		else if ((sim->elements[utype].Properties&PROP_CONDUCTS) && parts[underI].life <= 4)
		{
			parts[underI].life = 40 + parts[underI].life;
		}
		break;
	}
	//make temp of other things closer to it's own temperature. This will change temp of things that don't conduct, and won't change the PROT's temperature
	if (utype && utype != PT_WIFI)
		parts[underI].temp = restrict_flt(parts[underI].temp-(parts[underI].temp-parts[i].temp)/4.0f, MIN_TEMP, MAX_TEMP);
 

	//if this proton has collided with another last frame, change it into a heavier element
	no_temp_change:

	ahead = sim->photons[y][x];
	aheadI = ID(ahead);
	aheadT = TYP(ahead);
	
	if (parts[aheadI].ctype < PMAPID(1) && aheadT == PT_E186)
	{
		parts[i].tmp2 |= 2;
	}
	
	if (find_E186_only)
		return 0;

	if (parts[i].tmp)
	{
		int newID, element;
		bool myCollision = sim->isFromMyMod && (parts[i].tmp2 & 2);
		if (parts[i].tmp > 310)
		{
			if (parts[i].tmp > 500000)
				element = PT_SING; //particle accelerators are known to create earth-destroying black holes
			else if (myCollision)
				element = PT_POLC;
			else if (parts[i].tmp > 700)
				element = PT_PLUT;
			else if (parts[i].tmp > 420)
				element = PT_URAN;
			else // 310
				element = PT_POLO;
		}
		else if (parts[i].tmp > 250)
			element = PT_PLSM;
		else if (parts[i].tmp > 100)
			element = PT_O2;
		else if (parts[i].tmp > 50)
			element = PT_CO2;
		else
			element = PT_NBLE;
		product1:
		newID = sim->create_part(-1, x+rand()%3-1, y+rand()%3-1, element);
		if (newID >= 0)
			parts[newID].temp = restrict_flt(100.0f*parts[i].tmp, MIN_TEMP, MAX_TEMP);
		else if (myCollision)
		{
			// if (aheadT != PT_PROT || aheadI == i)
				return 0;
			// parts[aheadI].tmp += parts[i].tmp;
			// parts[aheadI].tmp2 |= 2;
		}
		sim->kill_part(i);
		return 1;
	}
	
	//collide with other protons to make heavier materials
	if (aheadI != i && aheadT == PT_PROT)
	{
		float velocity1 = powf(parts[i].vx, 2.0f)+powf(parts[i].vy, 2.0f);
		float velocity2 = powf(parts[aheadI].vx, 2.0f)+powf(parts[aheadI].vy, 2.0f);
		float direction1 = atan2f(-parts[i].vy, parts[i].vx);
		float direction2 = atan2f(-parts[aheadI].vy, parts[aheadI].vx);
		float difference = direction1 - direction2; if (difference < 0) difference += 6.28319f;

		if (difference > 3.12659f && difference < 3.15659f && velocity1 + velocity2 > 10.0f)
		{
			parts[aheadI].tmp += (int)(velocity1 + velocity2);
			parts[aheadI].tmp2 |= (parts[i].tmp2 & 2);
			sim->kill_part(i);
			return 1;
		}
	}
	return 0;
	
finding_E186:
	find_E186_only = true;
	goto no_temp_change;
}

//#TPT-Directive ElementHeader Element_PROT static int DeutImplosion(Simulation * sim, int n, int x, int y, float temp, int t)
int Element_PROT::DeutImplosion(Simulation * sim, int n, int x, int y, float temp, int t)
{
	int i;
	n = (n/50);
	if (n < 1)
		n = 1;
	else if (n > 340)
		n = 340;

	for (int c = 0; c < n; c++)
	{
		i = sim->create_part(-3, x, y, t);
		if (i >= 0)
			sim->parts[i].temp = temp;
		else if (sim->pfree < 0)
			break;
	}
	sim->pv[y/CELL][x/CELL] -= (6.0f * CFDS)*n;
	return 0;
}

//#TPT-Directive ElementHeader Element_PROT static int graphics(GRAPHICS_FUNC_ARGS)
int Element_PROT::graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 7;
	*firer = 250;
	*fireg = 170;
	*fireb = 170;

	*pixel_mode |= FIRE_BLEND;
	return 1;
}

Element_PROT::~Element_PROT() {}
