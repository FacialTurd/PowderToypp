// #include <stdint.h>
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
	Properties2 |= PROP_NOWAVELENGTHS | PROP_CTYPE_SPEC | PROP_NEUTRONS_LIKE | PROP_ALLOWS_WALL;

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
	int r, rr, s, sctype;
	int nx, ny;
	float r2, r3;
	Particle * under;
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
			else if ((r&0xFF) == ELEM_MULTIPP)
			{
				under = &parts[r>>8];
				if (under->life == 34)
				{
					parts[i].ctype = parts[i].tmp2;
					parts[i].tmp2 = 0;
					transportPhotons(sim, i, x, y, x+under->tmp, y+under->tmp2, PT_PHOT, &parts[i]);
					return 1;
				}
				else if (under->life == 16 && under->ctype == 29)
				{
					rr = under->tmp;
					rr = (rand() % (2 * rr + 1)) - rr;
					if (under->tmp2 > 0)
						rr *= under->tmp2;
					if (under->tmp3 & 1)
						parts[i].y = ny = y + rr, nx = x;
					else
						parts[i].x = nx = x + rr, ny = y;
					parts[i].ctype = 0x3FFFFFFF;
					transportPhotons(sim, i, x, y, nx, ny, PT_PHOT, &parts[i]);
					return 1;
				}
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
			{
				if (!--parts[i].tmp2)
				{
					parts[i].ctype = parts[i].tmp & 0x3FFFFFFF;
					parts[i].tmp = (unsigned int)(parts[i].tmp) >> 30;
					sim->part_change_type(i, x, y, PT_PHOT);
				}
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
					parts[i].ctype = k1;
					parts[i].tmp = 0;
					sim->part_change_type(i, x, y, PT_PHOT);
					// parts[i].x += parts[i].vx;
					// parts[i].y += parts[i].vy;
					// parts[i].type = PT_PHOT;
					// sim->photons[(int)(parts[i].y+0.5f)][(int)(parts[i].x+0.5f)] = PT_PHOT|(i<<8);
					// if ((sim->photons[y][x] >> 8) == i)
					// 	sim->photons[y][x] = 0;
					return 1;
				}
			}
			break;
		case 4:
			parts[i].temp = * (float*) &parts[i].tmp;
			break;
		case 5:
			if (parts[i].flags & FLAG_SKIPMOVE)
			{
				parts[i].flags &= ~FLAG_SKIPMOVE;
				return 1;
			}
			{
				int incx, incy;
				incx = floor(parts[i].vx + .5f);
				incy = floor(parts[i].vy + .5f);
				transportPhotons(sim, i, x, y, x + incx, y + incy, parts[i].type, &parts[i]);
			}
			return 1;
		}
		return 0;
	}
	if (sim->elements[PT_POLC].Enabled)
	{
		if (sim->bmap[y/CELL][x/CELL] == WL_DESTROYALL)
		{
			sim->kill_part(i);
			return 1;
		}
		bool u2pu = false;
		if (parts[i].flags & FLAG_SKIPCREATE)
		{
/*
			int nx, ny;
			nx = (parts[i].x + parts[i].vx) + .5f;
			ny = (parts[i].y + parts[i].vy) + .5f;
			if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES || (pmap[ny][nx]&0xFF != ELEM_MULTIPP) || parts[pmap[ny][nx]>>8].life != 5)
*/
			parts[i].flags &= ~FLAG_SKIPCREATE;
			goto skip1a;
		}
		if (!(rand()%60))
		{
			int rt = pmap[y][x] & 0xFF;
			if (!sctype || sctype == PT_E186)
				s = sim->create_part(-3, x, y, PT_ELEC);
			else if (sctype != PT_PROT || (rt != PT_URAN && rt != PT_PLUT && rt != PT_FILT))
				s = sim->create_part(-1, x, y, sctype);
			else
				s = -1, u2pu = true;
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
	skip1a:
		r = pmap[y][x];
		if (r)
		{
			int slife;
			switch (r&0xFF)
			{
			case PT_CAUS:
				if (sctype != PT_CAUS && sctype != PT_NEUT)
				{
					sim->part_change_type(r>>8, x, y, PT_RFRG); // probably inverse for NEUT???
					parts[r>>8].tmp = * (int*) &(sim->pv[y/CELL][x/CELL]); // floating point hacking
				}
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
			case PT_BIZR: case PT_BIZRG: case PT_BIZRS:
				parts[i].ctype = 0;
				break;
			case PT_VIRS:
			case PT_VRSS:
			case PT_VRSG:
				parts[r>>8].tmp4 = PT_NONE;
				break;
			case PT_URAN:
				if (u2pu)
				{
					sim->part_change_type(r>>8, x, y, PT_PLUT);
				}
			case PT_PLUT:
				if (parts[i].ctype == PT_PROT)
				{
					parts[i].vx += 0.01f*(rand()/(0.5f*RAND_MAX)-1.0f);
					parts[i].vy += 0.01f*(rand()/(0.5f*RAND_MAX)-1.0f);
				}
				break;
/*
			case PT_STOR:
				if (parts[r>>8].ctype > 0 && parts[r>>8].ctype < PT_NUM)
					parts[i].ctype = parts[r>>8].ctype;
				break;
			case PT_LAVA:
				switch (parts[r>>8].ctype)
				{
				}
				break;
*/
			default:
				break;
			}
		}
		
		int ahead = sim->photons[y][x];

		if ((ahead & 0xFF) == PT_PROT)
		{
			parts[ahead >> 8].tmp2 |= 2;
			if (!r && parts[ahead>>8].tmp > 2000 && (fabsf(parts[ahead>>8].vx) + fabsf(parts[ahead>>8].vy)) > 12)
			{
				sim->part_change_type(ahead>>8, x, y, PT_BOMB);
				parts[r>>8].tmp = 0;
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

//#TPT-Directive ElementHeader Element_E186 static void transportPhotons(Simulation* sim, int i, int x, int y, int nx, int ny, int t, Particle *phot)
void Element_E186::transportPhotons(Simulation* sim, int i, int x, int y, int nx, int ny, int t, Particle *phot)
{
	if ((sim->photons[y][x]>>8) == i)
		sim->photons[y][x] = 0;
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
	phot->type = t;
	sim->photons[ny][nx] = t|(i<<8);
	// return;
}


Element_E186::~Element_E186() {}
