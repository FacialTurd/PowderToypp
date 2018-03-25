// #include <stdint.h>
#include "simulation/Elements.h"
//Temp particle used for graphics
Particle tpart_phot;

#define TYPMAP(x,y) TYP(pmap[y][x])
#define ID part_ID
#define _PHOT_mov_rel_i(i,sx,sy,nt) transportPhotons(sim,i,x,y,(int)(x+(sx)),(int)(y+(sy)),(nt),&parts[i])
#define _PHOT_mov_rel_f(i,sx,sy,nt) transportPhotons(sim,i,x,y,(float)(parts[i].x+(sx)),(float)(parts[i].y+(sy)),(nt),&parts[i])

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
	Properties2 |= PROP_NOWAVELENGTHS | PROP_CTYPE_SPEC | PROP_NEUTRONS_LIKE | PROP_ALLOWS_WALL | PROP_PASSTHROUGHALL;

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
	int r, rr, s, sctype, rf;
	int dx, dy;
	float r2, r3;
	Particle * under;
	sctype = parts[i].ctype;
	if (sctype >= PMAPID(1)) // don't create SPC_AIR particle
	{
		r = pmap[y][x];
		switch (sctype - PMAPID(1))
		{
		case 0: // TODO: move into another element
			if (!(parts[i].tmp2&0x3FFFFFFF))
			{
				sim->kill_part(i);
				return 1;
			}
			else if (TYP(r) == ELEM_MULTIPP)
			{
				under = &partsi(r);
				if (under->life == 34)
				{
					parts[i].ctype = parts[i].tmp2;
					parts[i].tmp2 = 0;
					_PHOT_mov_rel_i(i, under->tmp, under->tmp2, PT_PHOT);
					return 1;
				}
				else if (under->life == 16 && under->ctype == 29)
				{
					rr = under->tmp;
					rr = (rand() % (2 * rr + 1)) - rr;
					if (under->tmp2 > 0)
						rr *= under->tmp2;
					rf = under->tmp3;
					dx = 0, dy = 0;
					(rf & 1) ? (dy = rr) : (dx = rr);
					parts[i].ctype = (rf & 2) ? parts[i].tmp2 : 0x3FFFFFFF;
					_PHOT_mov_rel_i(i, dx, dy, PT_PHOT);
					return 1;
				}
			}
			_PHOT_mov_rel_f(i, parts[i].vx, parts[i].vy, parts[i].type);
			x = (int)(parts[i].x + 0.5f); y = (int)(parts[i].y + 0.5f);
			if (TYPMAP(x, y) != ELEM_MULTIPP)
			{
				parts[i].ctype = parts[i].tmp2;
				parts[i].tmp2 = 0;
				sim->part_change_type(i, x, y, PT_PHOT);
			}
			return 1;
		case 1:
			if (TYP(r) != ELEM_MULTIPP)
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
					k3 = k2 & -k2, k2 &= ~k3;
					k4 = (k3 == 1 ? 1 : -1);
					s = sim->create_part(-1, x, y, PT_PHOT);
					if (s >= 0)
					{
						parts[s].vx =  k4*parts[i].vy;
						parts[s].vy = -k4*parts[i].vx;
						parts[s].temp = parts[i].temp;
						parts[s].life = parts[i].life;
						parts[s].ctype = k1;
						if (s > i)
							parts[s].flags |= FLAG_SKIPMOVE;
					}
				}
				if (CHECK_EXTEL(r, 10))
				{
					parts[i].ctype = k1;
					parts[i].tmp = 0;
					sim->part_change_type(i, x, y, PT_PHOT);
					// _PHOT_mov_rel_f(i, parts[i].vx, parts[i].vy, PT_PHOT);
					return 1;
				}
			}
			break;
		case 4:
			parts[i].temp = * (float*) &parts[i].tmp;
			break;
		case 5: // pseudo-neutrino movement
			if (parts[i].flags & FLAG_SKIPMOVE)
			{
				parts[i].flags &= ~FLAG_SKIPMOVE;
				return 1;
			}
			_PHOT_mov_rel_f(i, parts[i].vx, parts[i].vy, parts[i].type);
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
		bool u2pu = false, isbray = (sctype == PT_BRAY);
		r = pmap[y][x];
		
		if (parts[i].flags & FLAG_SKIPCREATE)
		{
			if (TYP(r) != ELEM_MULTIPP)
				parts[i].flags &= ~FLAG_SKIPCREATE;
			// goto skip1a;
		}
		else if (isbray || !(rand()%60))
		{
			int rt = TYP(r), itmp;
			if (!sctype || sctype == PT_E186)
				s = sim->create_part(-3, x, y, PT_ELEC);
			else if (
				(sctype != PT_PROT || (rt != PT_URAN && rt != PT_PLUT && rt != PT_FILT)) &&
				(sctype != PT_NEUT || rt != PT_GOLD))
				s = sim->create_part(-1, x, y, sctype);
			else
			{
				s = -1;
				if (sctype == PT_PROT)
					u2pu = true;
			}
			if(s >= 0)
			{
				isbray || (parts[i].temp += 400.0f);
				parts[s].temp = parts[i].temp;
				isbray || (sim->pv[y/CELL][x/CELL] += 1.5f);
				switch (sctype)
				{
				case PT_GRVT:
					parts[s].tmp = 0;
					break;
				case PT_WARP:
					parts[s].tmp2 = 3000 + rand() % 10000;
					break;
				case PT_LIGH:
					parts[s].tmp = rand()%360;
					break;
				case PT_BRAY:
					itmp = parts[i].tmp;
					if (itmp >= 1 && itmp <= 26)
						parts[s].ctype = 0x1F << (itmp - 1);
				}
			}
		}
	skip1a:
		if (r)
		{
			int slife;
			switch (TYP(r))
			{
			case PT_CAUS:
				if (!(isbray || sctype == PT_CAUS || sctype == PT_NEUT))
				{
					sim->part_change_type(ID(r), x, y, PT_RFRG); // probably inverse for NEUT???
					partsi(r).tmp = * (int*) &(sim->pv[y/CELL][x/CELL]); // floating point hacking
				}
				break;
			case PT_FILT:
				if (isbray)
				{
					partsi(r).life = 4;
					int filtmode = partsi(r).tmp;
					switch (filtmode)
					{
					case 6: break; // no operation
					case 9: // random wavelength
						parts[i].tmp = rand() % 26 + 1;
						break;
					case 13:
						parts[i].tmp && (parts[i].tmp = 27 - parts[i].tmp);
						break;
					default:
						{
							int shift = (int)((partsi(r).temp - 273.0f) * 0.025f);
							shift < 0 ? (shift = 0) : shift > 25 && (shift = 25);
							int wl = parts[i].tmp;
							if (filtmode == 4 || filtmode == 5)
							{
								shift == 0 && (shift = 1);
								if (filtmode == 4)
									wl && (wl += shift, wl > 26 && (wl = 26));
								else
									wl && (wl -= shift, wl <  1 && (wl =  1));
							}
							else if (partsi(r).ctype != 0x3FFFFFFF)
								wl = shift + 1;
							else
								wl = 0;
							parts[i].tmp = wl;
						}
					}
				}
				else
					sim->part_change_type(i, x, y, PT_PHOT),
					parts[i].ctype = 0x3FFFFFFF;
				break;
			case PT_EXOT:
				if (partsi(r).ctype != parts[i].type && !(rand()%3))
				{
					partsi(r).ctype = parts[i].type;
					partsi(r).tmp2 = rand()%50 + 120;
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
						s = ID(r);
						sim->create_part(s, x, y, PT_PHOT);
						r2 = (rand()%228+128)/127.0f;
						r3 = (rand()%360)*3.14159f/180.0f;
						parts[s].vx = r2*cosf(r3);
						parts[s].vy = r2*sinf(r3);
					}
					else
					{
						sim->create_part(ID(r), x, y, PT_E186);
					}
				}
				break;
			case PT_INVIS:
				if (!(partsi(r).tmp2 || isbray))
					parts[i].ctype = PT_NEUT;
				break;
			case PT_BIZR: case PT_BIZRG: case PT_BIZRS:
				if (!isbray)
					parts[i].ctype = 0;
				break;
			case PT_URAN:
				if (u2pu)
				{
					sim->part_change_type(ID(r), x, y, PT_PLUT);
				}
			case PT_PLUT:
				if (parts[i].ctype == PT_PROT)
				{
					parts[i].vx += 0.01f*(rand()/(0.5f*RAND_MAX)-1.0f);
					parts[i].vy += 0.01f*(rand()/(0.5f*RAND_MAX)-1.0f);
				}
				break;
			case PT_BRAY:
				if (isbray && (partsi(r).tmp == 0 || partsi(r).tmp == 1))
					partsi(r).life = 1020,
					partsi(r).tmp = 1;
				break;
/*
			case PT_STOR:
				if (partsi(r).ctype > 0 && partsi(r).ctype < PT_NUM)
					parts[i].ctype = partsi(r).ctype;
				break;
			case PT_LAVA:
				switch (partsi(r).ctype)
				{
				}
				break;
*/
			default:
				break;
			}
		}
		
		int ahead = sim->photons[y][x];

		if (TYP(ahead) == PT_PROT)
		{
			partsi(ahead).tmp2 |= 2;
			if (!r && partsi(ahead).tmp > 2000 && (fabsf(partsi(ahead).vx) + fabsf(partsi(ahead).vy)) > 12)
			{
				sim->part_change_type(ID(ahead), x, y, PT_BOMB);
				partsi(ahead).tmp = 0;
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
	if (ID(sim->photons[y][x]) == i)
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
	sim->photons[ny][nx] = PMAP(i, t);
	// return;
}

//#TPT-Directive ElementHeader Element_E186 static void transportPhotons(Simulation* sim, int i, int x, int y, float nxf, float nyf, int t, Particle *phot)
void Element_E186::transportPhotons(Simulation* sim, int i, int x, int y, float nxf, float nyf, int t, Particle *phot)
{
	int nx, ny;
	if (ID(sim->photons[y][x]) == i)
		sim->photons[y][x] = 0;
	if (sim->edgeMode == 2)
	{
		nxf = sim->remainder_p(nxf-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
		nyf = sim->remainder_p(nyf-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
	}

	nx = (int)(nxf + 0.5f), ny = (int)(nyf + 0.5f);

	if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
	{
		sim->kill_part(i);
		return;
	}

	phot->x = nxf;
	phot->y = nyf;
	phot->type = t;
	sim->photons[ny][nx] = PMAP(i, t);
	// return;
}


Element_E186::~Element_E186() {}
