#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_PROT PT_PROT 173
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

	HeatConduct = 61;
	Description = "Protons. Transfer heat to materials, and removes sparks.";

	Properties = TYPE_ENERGY;
	Properties2 |= TYPE_ENERGY | PROP_PASSTHROUGHALL;

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

namespace
{
const int FLAG_HEAT_CONDUCT = 1;
const int FLAG_COLLIDE = 2;

int PROT_interaction_1 (Simulation * sim, int i, int x, int y, int uID, int func, int & flags)
{
	Particle &p_i = sim->parts[i], &p_uID = sim->parts[uID];

	switch (func)
	{
	case 6:
		p_i.temp = p_uID.temp;
		flags &= ~FLAG_HEAT_CONDUCT;
		break;
	case 10:
		if (p_uID.temp > 1273.0f)
			return 0;
		if (p_uID.temp < 73.15f)
		{
			p_i.tmp2 &= ~1;
			p_i.tmp2 |= (p_uID.tmp >> 9) & 1;
			flags &= ~FLAG_HEAT_CONDUCT;
		}
		break;
	case 11:
		if (p_uID.tmp2 == 1)
		{
			p_i.x = x;
			p_i.y = y;
			p_i.life *= 2;
			p_i.ctype = 0x3FFFFFFF;
			return PT_PHOT;
		}
		break;
	case 12:
		if ((p_uID.tmp & 7) == 2)
		{
			bool vibr_found = false;
			int velSqThreshold = (int)(p_uID.temp - (273.15f - 0.5f));
			if (velSqThreshold < 0)
				velSqThreshold = 0;
			else if (p_uID.tmp == 10)
				velSqThreshold *= 100;
			int r, rx, ry, trade;

			for (trade = 0; trade < 5; trade++)
			{
				rx = rand()%5-2;
				ry = rand()%5-2;
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = sim->pmap[y+ry][x+rx];
					switch (TYP(r))
					{
					case PT_BVBR:
					case PT_VIBR:
						p_uID.tmp2 += sim->partsi(r).tmp;
						sim->partsi(r).tmp = 0;
						break;
					}
				}
			}

			if (vibr_found && sim->pv[y/CELL][x/CELL] > 0.0f)
			{
				p_uID.tmp2 += sim->pv[y/CELL][x/CELL] * 7.0f,
				sim->pv[y/CELL][x/CELL] = 0.0f;
			}
			if (p_uID.tmp2 < velSqThreshold)
			{
				p_uID.tmp2 += p_i.tmp + (int)(p_i.vx * p_i.vx + p_i.vy * p_i.vy + 0.5f);
				return 0;
			}
			p_i.tmp += velSqThreshold;
			p_uID.tmp2 -= velSqThreshold;
			flags &= ~(FLAG_HEAT_CONDUCT | FLAG_COLLIDE);
		}
		break;
	case 16:
		if (p_uID.ctype == 5)
		{
			if (p_uID.tmp >= 0x40)
			{
				p_i.tmp2 &= ~2;
				((p_uID.tmp & 0x40) && (p_i.tmp2 |= 2));
			}
		}
		break;
	case 22:
		if ((p_uID.tmp>>3) == 2)
			flags &= ~FLAG_HEAT_CONDUCT;
		break;
	case 33:
		{
			const float dtable[7] = {0.0f, 100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, (MAX_TEMP-MIN_TEMP)};
			float change;
			int temp = (int)(p_i.temp - 323.15f) / 50;
			bool sign1 = (temp < 0); sign1 && (temp = -temp); temp > 6 && (temp = 6);
			change = dtable[temp] * (sign1 ? -1 : 1);
			p_uID.temp = restrict_flt(p_uID.temp + change, MIN_TEMP, MAX_TEMP);
		}
		flags &= ~FLAG_HEAT_CONDUCT;
	}
	return -1;
}
}

//#TPT-Directive ElementHeader Element_PROT static int update(UPDATE_FUNC_ARGS)
int Element_PROT::update(UPDATE_FUNC_ARGS)
{
	sim->pv[y/CELL][x/CELL] -= .003f;
	int under = pmap[y][x], ahead;
	if (TYP(under) == PT_PINVIS)
		under = partsi(under).tmp4;
	int utype = TYP(under), aheadT;
	int uID = ID(under), aheadI;
	int sparked, flags = -1;

	switch (utype)
	{
	case PT_SPRK:
		//remove active sparks
		sparked = parts[uID].ctype;
		if (sparked == ELEM_MULTIPP) // never appearing SPRK (ELEM_MULTIPP)
			sparked = 0;
		if (!sim->part_change_type(uID, x, y, sparked))
		{
			parts[uID].life = 44 + parts[uID].life;
			parts[uID].ctype = 0;
		}
		else
			flags &= ~FLAG_HEAT_CONDUCT;
		break;
	case PT_DEUT:
		if ((-((int)sim->pv[y / CELL][x / CELL] - 4) + (parts[uID].life / 100)) > rand() % 200)
		{
			DeutImplosion(sim, parts[uID].life, x, y, restrict_flt(parts[uID].temp + parts[uID].life * 500, MIN_TEMP, MAX_TEMP), PT_PROT);
			sim->kill_part(uID);
		}
		break;
	case PT_LCRY:
		//Powered LCRY reaction: PROT->PHOT
		if (parts[uID].life > 5 && !(rand() % 10))
		{
			sim->part_change_type(i, x, y, PT_PHOT);
			parts[i].life *= 2;
			parts[i].ctype = 0x3FFFFFFF;
		}
		break;
	case PT_EXOT:
		if (parts[uID].ctype != PT_E195)
			parts[uID].ctype = PT_PROT;
		break;
	case PT_WIFI:
		float change;
		if (parts[i].temp < 173.15f) change = -1000.0f;
		else if (parts[i].temp < 273.15f) change = -100.0f;
		else if (parts[i].temp > 473.15f) change = 1000.0f;
		else if (parts[i].temp > 373.15f) change = 100.0f;
		else change = 0.0f;
		parts[uID].temp = restrict_flt(parts[uID].temp + change, MIN_TEMP, MAX_TEMP);
		flags &= ~FLAG_HEAT_CONDUCT;
		break;
	case ELEM_MULTIPP:
		{
			int t = PROT_interaction_1(sim, i, x, y, uID, parts[uID].life, flags);
			if (t >= 0)
			{
				sim->part_change_type(i, x, y, t);
				return 1;
			}
		}
		break;
	case PT_NONE:
		//slowly kill if it's not inside an element
		if (parts[i].life)
		{
			if (!--parts[i].life)
				sim->kill_part(i);
		}
		flags &= ~FLAG_HEAT_CONDUCT;
		break;
	default:
		//set off explosives (only when hot because it wasn't as fun when it made an entire save explode)
		if (parts[i].temp > 273.15f + 500.0f && (sim->elements[utype].Flammable || sim->elements[utype].Explosive || utype == PT_BANG))
		{
			sim->create_part(uID, x, y, PT_FIRE);
			parts[uID].temp += restrict_flt(sim->elements[utype].Flammable * 5, MIN_TEMP, MAX_TEMP);
			sim->pv[y / CELL][x / CELL] += 1.00f;
		}
		//prevent inactive sparkable elements from being sparked
		else if ((sim->elements[utype].Properties & PROP_CONDUCTS) && parts[uID].life <= 4)
		{
			parts[uID].life = 40 + parts[uID].life;
		}
		break;
	}
	//make temp of other things closer to it's own temperature. This will change temp of things that don't conduct, and won't change the PROT's temperature
	if (flags & FLAG_HEAT_CONDUCT)
		parts[uID].temp = restrict_flt(parts[uID].temp-(parts[uID].temp-parts[i].temp)/4.0f, MIN_TEMP, MAX_TEMP);
 
	//if this proton has collided with another last frame, change it into a heavier element
	ahead = sim->photons[y][x];
	aheadI = ID(ahead);
	aheadT = TYP(ahead);
	
	if (parts[aheadI].ctype < PMAPID(1) && aheadT == PT_E195)
	{
		parts[i].tmp2 |= 2;
	}
	
	if (!(flags & FLAG_COLLIDE))
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
		else if (myCollision && (parts[i].tmp > 150))
			element = PT_SLCN;
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
		float energy = CollisionEnergy(&parts[i], &parts[aheadI]);
		if (energy > 10.0f)
		{
			parts[aheadI].tmp += (int)energy;
			parts[aheadI].tmp2 |= (parts[i].tmp2 & 2);
			sim->kill_part(i);
			return 1;
		}
	}
	return 0;
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

//#TPT-Directive ElementHeader Element_PROT static float CollisionEnergy(const Particle* cpart1, const Particle* cpart2, float angle = -0.015f)
float Element_PROT::CollisionEnergy(const Particle* cpart1, const Particle* cpart2, float angle)
{
	float velocity1 = powf(cpart1->vx, 2.0f) + powf(cpart1->vy, 2.0f);
	float velocity2 = powf(cpart2->vx, 2.0f) + powf(cpart2->vy, 2.0f);
	float a = angle * (cpart1->vx * cpart2->vx + cpart1->vy * cpart2->vy);
	float b = cpart1->vy * cpart2->vx - cpart1->vx * cpart2->vy;
	if (a > b && a > -b)
		return velocity1 + velocity2;
	return 0.0f;
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
