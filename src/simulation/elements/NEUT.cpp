#include "simulation/Elements.h"
#define ID part_ID
#define SETTRANS(t, e1, e2, m, p) (\
	t[2*e1] = PMAP(m, e2), \
	t[2*e1+1] = p * (RAND_MAX / 1000.0))

//#TPT-Directive ElementClass Element_NEUT PT_NEUT 18
Element_NEUT::Element_NEUT()
{
	Identifier = "DEFAULT_PT_NEUT";
	Name = "NEUT";
	Colour = PIXPACK(0x20E0FF);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = -0.99f;
	Gravity = 0.0f;
	Diffusion = 0.01f;
	HotAir = 0.002f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = -1;

	Temperature = R_TEMP+4.0f	+273.15f;
	HeatConduct = 60;
	Description = "Neutrons. Interact with matter in odd ways.";

	Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;
	Properties2 |= PROP_ENERGY_PART | PROP_NEUTRONS_LIKE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_NEUT::update;
	Graphics = &Element_NEUT::graphics;
	
	if (Element_NEUT::TransitionTable == NULL)
	{
		int * tt = (int*)calloc(2 * PT_NUM, sizeof(int));
		Element_NEUT::TransitionTable = tt;
		if (tt != NULL)
		{
			// old type, new type, method, probability
			SETTRANS(tt, PT_GUNP, PT_DUST, 1,   15);
			SETTRANS(tt, PT_DYST, PT_YEST, 1,   15);
			SETTRANS(tt, PT_YEST, PT_DYST, 1, 1000);
			SETTRANS(tt, PT_PLEX, PT_GOO , 1,   15);
			SETTRANS(tt, PT_NITR, PT_DESL, 1,   15);
			SETTRANS(tt, PT_PLNT, PT_WOOD, 2,   50);
			SETTRANS(tt, PT_DESL, PT_GAS , 1,   15);
			SETTRANS(tt, PT_OIL , PT_GAS , 1,   15);
			SETTRANS(tt, PT_COAL, PT_WOOD, 2,   50);
			SETTRANS(tt, PT_BCOL, PT_SAWD, 2,   50);
			SETTRANS(tt, PT_DUST, PT_FWRK, 1,   50);
			SETTRANS(tt, PT_ACID, PT_ISOZ, 2,   50);
		}
	}
}

//#TPT-Directive ElementHeader Element_NEUT static int * TransitionTable
int * Element_NEUT::TransitionTable = NULL;

//#TPT-Directive ElementHeader Element_NEUT static int update(UPDATE_FUNC_ARGS)
int Element_NEUT::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, target_r = -1;
	int iX = 0, iY = 0;
	int pressureFactor = 3 + (int)sim->pv[y/CELL][x/CELL];
	int DMG_count = 0;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK)
			{
				r = pmap[y+ry][x+rx];
				rt = TYP(r);
				switch (rt)
				{
				case PT_WATR:
					if (3>(rand()%20))
						sim->part_change_type(ID(r),x+rx,y+ry,PT_DSTW);
				case PT_ICEI:
				case PT_SNOW:
					parts[i].vx *= 0.995;
					parts[i].vy *= 0.995;
					break;
				case PT_PLUT:
					if (pressureFactor>(rand()%1000))
					{
						if (!(rand()%3))
						{
							sim->create_part(ID(r), x+rx, y+ry, rand()%3 ? PT_LAVA : PT_URAN);
							parts[ID(r)].temp = MAX_TEMP;
							if (partsi(r).type==PT_LAVA) {
								partsi(r).tmp = 100;
								partsi(r).ctype = PT_PLUT;
							}
						}
						else
						{
							sim->create_part(ID(r), x+rx, y+ry, PT_NEUT);
							partsi(r).vx = 0.25f*partsi(r).vx + parts[i].vx;
							partsi(r).vy = 0.25f*partsi(r).vy + parts[i].vy;
						}
						sim->pv[y/CELL][x/CELL] += 10.0f * CFDS; //Used to be 2, some people said nukes weren't powerful enough
						Element_FIRE::update(UPDATE_FUNC_SUBCALL_ARGS);
					}
					break;
#ifdef SDEUT
				case PT_DEUT:
					if ((pressureFactor+1+(partsi(r).life/100))>(rand()%1000))
					{
						DeutExplosion(sim, partsi(r).life, x+rx, y+ry, restrict_flt(partsi(r).temp + partsi(r).life*500.0f, MIN_TEMP, MAX_TEMP), PT_NEUT);
						sim->kill_part(ID(r));
					}
					break;
#else
				case PT_DEUT:
					if ((pressureFactor+1)>(rand()%1000))
					{
						create_part(ID(r), x+rx, y+ry, PT_NEUT);
						partsi(r).vx = 0.25f*partsi(r).vx + parts[i].vx;
						partsi(r).vy = 0.25f*partsi(r).vy + parts[i].vy;
						partsi(r).life --;
						partsi(r).temp = restrict_flt(partsi(r).temp + partsi(r).life*17.0f, MIN_TEMP, MAX_TEMP);
						sim->pv[y/CELL][x/CELL] += 6.0f * CFDS;
					}
					break;
#endif
				case PT_FWRK:
					if (!(rand()%20))
						partsi(r).ctype = PT_DUST;
					break;
				case PT_TTAN:
					if (!(rand()%20))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_EXOT:
					if (partsi(r).ctype != PT_E195 && !(rand()%20))
						partsi(r).life = 1500;
					break;
				case PT_RFRG:
					if (rand()%2)
						sim->create_part(ID(r), x+rx, y+ry, PT_GAS);
					else
						sim->create_part(ID(r), x+rx, y+ry, PT_CAUS);
					break;
				case PT_POLC:
					if (partsi(r).tmp && !(rand()%80))
						partsi(r).tmp--;
					break;
				case PT_DMG:
					DMG_count++;
					break;
				case ELEM_MULTIPP:
					{
					int rr, j, nr;
					if (partsi(r).life == 22)
					{
						switch (partsi(r).tmp >> 3)
						{
						case 1:
							parts[i].vx = 0, parts[i].vy = 0;
							break;
						case 2:
							if (partsi(r).temp > 8000)
							{
								int temp = (int)(partsi(r).temp - 8272.65f);
								if (!(rx || ry) && temp > (int)(rand() * 1000.0f / (RAND_MAX+1.0f)))
								{
									sim->kill_part(i);
									return 1;
								}
							}
							else if (!(rand()%25))
							{
								rr = sim->create_part(-1, x, y, PT_ELEC);
								if (rr >= 0)
								{
									parts[i].tmp2 = 1;
									sim->part_change_type(i, x, y, PT_PROT);
								}
							}
							break;
						case 3:
							if (!(partsi(r).tmp2 || rand()%500))
							{
								int np = sim->create_part(-1, x, y, PT_NEUT);
								if (np < 0) parts[np].temp = parts[i].temp;
								partsi(r).tmp2 = 500;
							}
							if (!(rx || ry))
							{
								sim->ineutcount++;
							}
							break;
						case 4:
							parts[i].vx *= 0.995;
							parts[i].vy *= 0.995;
							break;
						}
					}
					else if (partsi(r).life <= 8 && !(rx || ry))
					{
						if (partsi(r).life == 5 && !partsi(r).tmp)
							target_r = ID(r);
						if (partsi(r).life != 8)
							continue;
						parts[i].vx = 0, parts[i].vy = 0;
						for (j = 0; j < 5; j++)
						{
							iX = rand() % (ISTP * 2 + 1) - ISTP;
							iY = rand() % (ISTP * 2 + 1) - ISTP;
							rr = pmap[y+iY][x+iX];
							if (CHECK_EXTEL(rr, 8))
								break;
						}
						if (j == 5)
							iY = 0, iX = 0;
					}
					else if (partsi(r).life == 16 && partsi(r).ctype == 25 && Element_MULTIPP::Arrow_keys != NULL)
					{
						int tmp2 = partsi(r).tmp2;
						int multiplier = (tmp2 >> 4) + 1;
						tmp2 &= 0x0F;
						if (Element_MULTIPP::Arrow_keys[0] & 0x10 && tmp2 >= 1 && tmp2 <= 8)
						{
							iX += multiplier*sim->portal_rx[tmp2-1];
							iY += multiplier*sim->portal_ry[tmp2-1];
						}
					}
					}
					break;
				case PT_NONE:
					break;
				default:
					if (TransitionTable != NULL)
					{
						int t = TransitionTable[2 * rt];
						int m = ID(t);
						if (m <= 0)
							break;
						t = TYP(t);
						if (rand() <= TransitionTable[2 * rt + 1])
							if (m == 1)
								sim->part_change_type(ID(r), x+rx, y+ry, t);
							else
								sim->create_part(ID(r), x+rx, y+ry, t);
					}
					break;
				}
			}

	if (DMG_count)
	{
		bool b = false;
		for (int xx = -2; xx < 3; xx++)
			for (int yy = -2; yy < 3; yy++)
				if (x/CELL+xx >= 0 && x/CELL+xx < XRES/CELL &&
					y/CELL+yy >= 0 && y/CELL+yy < YRES/CELL &&
					sim->wtypes[ sim->bmap[y/CELL+yy][x/CELL+xx] ].PressureTransition >= 0)
				{
					sim->bmap_brk[y/CELL+yy][x/CELL+xx] |= 1;
					sim->pv[y/CELL+yy][x/CELL+xx] += DMG_count*1.0f;
					b = true;
				}
		if (b)
			sim->pv[y/CELL][x/CELL] += DMG_count*10.0f;
	}

	if (target_r >= 0)
	{
		ChangeDirection(sim, i, x, y, &parts[i], &parts[target_r]);
	}
	else
	{
		parts[i].vx += (float)iX;
		parts[i].vy += (float)iY;
	}
	return 0;
}


//#TPT-Directive ElementHeader Element_NEUT static void ChangeDirection(Simulation* sim, int i, int x, int y, Particle* neut, Particle* under)
void Element_NEUT::ChangeDirection(Simulation* sim, int i, int x, int y, Particle* neut, Particle* under)
{
	if (under->tmp2 == 2)
	{
		neut->ctype = 0x100;
		neut->tmp2 = 0x3FFFFFFF;
		sim->part_change_type(i, x, y, PT_E195);
	}
	else if (under->tmp2 == 25)
	{
		int rr = under->ctype;
		float angle = rand() / (float)(RAND_MAX) - 0.5f;
		float radius = (float)(rr >> 4) / 16.0f;
		if (rr & 8) angle *= 0.5f;
		angle += (float)(rr & 7) / 4.0f;
		neut->vx = sinf(angle * M_PI) * radius;
		neut->vy = cosf(angle * M_PI) * radius;
	}
}

//#TPT-Directive ElementHeader Element_NEUT static int graphics(GRAPHICS_FUNC_ARGS)
int Element_NEUT::graphics(GRAPHICS_FUNC_ARGS)

{
	*firea = 120;
	*firer = 10;
	*fireg = 80;
	*fireb = 120;

	*pixel_mode |= FIRE_ADD;
	return 1;
}

//#TPT-Directive ElementHeader Element_NEUT static int DeutExplosion(Simulation * sim, int n, int x, int y, float temp, int t)
int Element_NEUT::DeutExplosion(Simulation * sim, int n, int x, int y, float temp, int t)//testing a new deut create part
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
	sim->pv[y/CELL][x/CELL] += (6.0f * CFDS)*n;
	return 0;
}

Element_NEUT::~Element_NEUT() {}
