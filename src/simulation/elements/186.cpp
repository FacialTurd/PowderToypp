#include "simulation/Elements.h"
//Temp particle used for graphics
Particle tpart_phot;

//#TPT-Directive ElementClass Element_E186 PT_E186 186
Element_E186::Element_E186()
{
	Identifier = "DEFAULT_PT_E186";
	Name = "E186";
	Colour = PIXPACK(0xDFEFFF);
	MenuVisible = 0;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = -0.99f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = -1;

	Temperature = R_TEMP+200.0f+273.15f;
	HeatConduct = 251;
	Description = "Experimental element.";

	Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_RADIOACTIVE|PROP_LIFE_KILL_DEC;
	Properties2 |= PROP_NOWAVELENGTHS | PROP_CTYPE_SPEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_E186::update;
	Graphics = &Element_E186::graphics;
	
	memset(&tpart_phot, 0, sizeof(Particle));
}

//#TPT-Directive ElementHeader Element_E186 static int update(UPDATE_FUNC_ARGS)
int Element_E186::update(UPDATE_FUNC_ARGS)
{
	int r, s, sctype;
	float r2, r3;
	sctype = parts[i].ctype;
	if (sctype >= 0x100) // don't create SPC_AIR particle
	{
		r = pmap[y][x];
		switch (sctype - 0x100)
		{
		case 0:
			if (!(parts[i].tmp2&0x3FFFFFFF))
			{
				sim->kill_part(i);
				return 1;
			}
			else if ((r&0xFF) == ELEM_MULTIPP && parts[r>>8].life == 34)
			{
				transportPhotons(sim, i, x, y, &parts[i], &parts[r>>8]);
				return 1;
			}
			break;
		case 1:
			if ((r&0xFF) != ELEM_MULTIPP)
			{
				switch (rand()%4)
				{
					case 0:
						sim->part_change_type(i, x, y, PT_PHOT);
						parts[i].ctype = 0x3FFFFFFF;
						break;
					case 1:
						sim->part_change_type(i, x, y, PT_ELEC);
						break;
					case 2:
						sim->part_change_type(i, x, y, PT_NEUT);
						break;
					case 3:
						sim->part_change_type(i, x, y, PT_GRVT);
						parts[i].tmp = 0;
						break;
				}
			}
			break;
		case 2:
			if (parts[i].tmp2)
				parts[i].tmp2--;
			else
			{
				parts[i].ctype = parts[i].tmp & 0x3FFFFFFF;
				parts[i].tmp = (unsigned int)(parts[i].tmp) >> 30;
				sim->part_change_type(i, x, y, PT_PHOT);
			}
			return 1; // 1 means no movement
		case 3:
			{
				int k1 = parts[i].tmp;
				int k2 = parts[i].tmp2 & 3;
				int k3, k4;
				while (k2)
				{
					k4 = k2 & -k2, k2 &= ~k4;
					k3 = (k4 == 1 ? 1 : -1);
					s = sim->create_part(-1, x, y, PT_PHOT);
					if (s >= 0)
					{
						parts[s].vx =  k3*parts[i].vy;
						parts[s].vy = -k3*parts[i].vx;
						parts[s].temp = parts[i].temp;
						parts[s].life = parts[i].life;
						parts[s].ctype = k1;
						if (s > i)
							parts[s].flags |= FLAG_SKIPMOVE;
					}
				}
				if ((r&0xFF) == ELEM_MULTIPP && parts[r>>8].life == 10)
				{
					sim->part_change_type(i, x, y, PT_PHOT);
					parts[i].ctype = k1;
					parts[i].tmp = 0;
					return 1;
				}
			}
			break;
		}
		return 0;
	}
	if (sim->elements[PT_POLC].Enabled)
	{
		if (!(rand()%60))
		{
			if (!sctype || sctype == PT_E186)
				s = sim->create_part(-3, x, y, PT_ELEC);
			else
				s = sim->create_part(-1, x, y, sctype);
			if(s >= 0)
			{
				parts[i].temp += 400.0f;
				parts[s].temp = parts[i].temp;
				sim->pv[y/CELL][x/CELL] += 1.5f;
				if (sctype == PT_GRVT)
					parts[s].tmp = 0;
				else if (sctype == PT_WARP)
					parts[s].tmp2 = 3000 + rand() % 10000;
			}
		}
		r = pmap[y][x];
		if (r)
		{
			int slife;
			switch (r&0xFF)
			{
			case PT_CAUS:
				sim->part_change_type(r>>8, x, y, PT_RFRG); // probably inverse for NEUT???
				parts[r>>8].tmp = * (int*) &(sim->pv[y/CELL][x/CELL]); // floating point hacking
				break;
			case PT_FILT:
				sim->part_change_type(i, x, y, PT_PHOT);
				parts[i].ctype = 0x3FFFFFFF;
				break;
			case PT_EXOT:
				if (!(rand()%3))
				{
					sim->part_change_type(r>>8, x, y, PT_WARP);
					parts[r>>8].life = 1000;
					parts[r>>8].tmp2 = 10000;
					parts[r>>8].temp = parts[i].temp = MAX_TEMP;
				}
				break;
			case PT_ISOZ:
			case PT_ISZS:
				if (!(rand()%40))
				{
					slife = parts[i].life;
					if (slife)
						parts[i].life = slife + 50;
					else
						parts[i].life = 0;

					if (rand()%20)
					{
						s = r>>8;
						sim->create_part(s, x, y, PT_PHOT);
						r2 = (rand()%228+128)/127.0f;
						r3 = (rand()%360)*3.14159f/180.0f;
						parts[s].vx = r2*cosf(r3);
						parts[s].vy = r2*sinf(r3);
					}
					else
					{
						sim->create_part(r>>8, x, y, PT_E186);
					}
				}
				break;
			case PT_INVIS:
				if (!parts[r>>8].tmp2)
					parts[i].ctype = PT_NEUT;
				break;
			case PT_VIRS:
			case PT_VRSS:
			case PT_VRSG:
				parts[r>>8].tmp4 = PT_NONE;
				break;
			case PT_LAVA:
				if (parts[r>>8].ctype == PT_POLO && !(rand()&0xFF))
					parts[r>>8].ctype = PT_POLC;
				break;
			default:
				break;
			}
		}
		else
		{
			int rx = rand()%3 - 1;
			int ry = rand()%3 - 1;
			if (BOUNDS_CHECK && 2 > rand()%5)
			{
				r = sim->photons[y + ry][x + rx];
/*
				s = parts[i].ctype;
				if ((r&0xFF) == PT_NEUT && (s == PT_PROT || s == PT_GRVT))
				{
					sim->part_change_type(r>>8, x, y, s);
					parts[i].ctype = PT_NEUT;
				}
*/
				if ((r&0xFF) == PT_PROT && !pmap[y+ry][x+rx])
				{
					if (parts[r>>8].tmp > 250)
					{
						int element = PT_POLC;
						if (parts[r>>8].tmp > 1000 && (fabsf(parts[r>>8].vx) + fabsf(parts[r>>8].vy)) > 8)
							element = PT_BOMB;
						sim->part_change_type(r>>8, x, y, element);
						parts[r>>8].tmp = 0;
						return 0;
					}
				}
			}
		}
	}
	
	return 0;
}

//#TPT-Directive ElementHeader Element_E186 static int graphics(GRAPHICS_FUNC_ARGS)
int Element_E186::graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->ctype == 0x100)
	{
		// Emulate the PHOT graphics
		tpart_phot.ctype = cpart->tmp2;
		tpart_phot.flags = cpart->flags;
		Element_PHOT::graphics(ren, &tpart_phot, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
		return 0;
	}
	*firea = 70;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode |= FIRE_ADD;
	return 0;
}

//#TPT-Directive ElementHeader Element_E186 static void transportPhotons(Simulation* sim, int i, int x, int y, Particle *phot, Particle *other1)
void Element_E186::transportPhotons(Simulation* sim, int i, int x, int y, Particle *phot, Particle *other1)
{
	if ((sim->photons[y][x]>>8) == i)
		sim->photons[y][x] = 0;
	int nx = x + other1->tmp, ny = y + other1->tmp2;
	if (sim->edgeMode == 2)
	{
		// maybe sim->remainder_p ?
		nx = (nx-CELL) % (XRES-2*CELL) + CELL;
		(nx < CELL) && (nx += (XRES-2*CELL));
		ny = (ny-CELL) % (YRES-2*CELL) + CELL;
		(ny < CELL) && (ny += (YRES-2*CELL));
	}
	else if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
	{
		sim->kill_part(i);
		return;
	}
	phot->x = (float)nx;
	phot->y = (float)ny;
	phot->type = PT_PHOT;
	phot->ctype = phot->tmp2;
	phot->tmp2 = 0;
	sim->photons[ny][nx] = PT_PHOT|(i<<8);
	// return;
}


Element_E186::~Element_E186() {}
