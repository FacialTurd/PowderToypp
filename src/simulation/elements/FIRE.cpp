#include "common/tpt-minmax.h"
#include "simulation/Elements.h"
#define ID part_ID

//#TPT-Directive ElementClass Element_FIRE PT_FIRE 4
// in real life, burning is actually a reaction of oxygen and the burning material.
Element_FIRE::Element_FIRE()
{
	Identifier = "DEFAULT_PT_FIRE";
	Name = "FIRE";
	Colour = PIXPACK(0xFF1000);
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.1f;
	Diffusion = 0.00f;
	HotAir = 0.001f  * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 2;

	Temperature = R_TEMP+400.0f+273.15f;
	HeatConduct = 88;
	Description = "Ignites flammable materials. Heats air.";

	Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2773.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &Element_FIRE::update;
	Graphics = &Element_FIRE::graphics;
}

//#TPT-Directive ElementHeader Element_FIRE static int update(UPDATE_FUNC_ARGS)
int Element_FIRE::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, t = parts[i].type;
	switch (t)
	{
	case PT_PLSM:
		if (parts[i].life <=1)
		{
			if (parts[i].ctype == PT_NBLE)
			{
				sim->part_change_type(i,x,y,PT_NBLE);
				parts[i].life = 0;
			}
			else if ((parts[i].tmp&0x3) == 3){
				sim->part_change_type(i,x,y,PT_DSTW);
				parts[i].life = 0;
				parts[i].ctype = PT_FIRE;
			}
		}
		break;
	case PT_FIRE:
		if (parts[i].life <=1)
		{
			if ((parts[i].tmp&0x3) == 3){
				sim->part_change_type(i,x,y,PT_DSTW);
				parts[i].life = 0;
				parts[i].ctype = PT_FIRE;
			}
			else if (parts[i].temp<625)
			{
				sim->part_change_type(i,x,y,PT_SMKE);
				parts[i].life = rand()%20+250;
			}
		}
		break;
	default:
		break;
	}
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);
				
				//THRM burning
				if (rt==PT_THRM && (t==PT_FIRE || t==PT_PLSM || t==PT_LAVA))
				{
					if (!(rand()%500)) {
						sim->part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
						partsi(r).ctype = PT_BMTL;
						partsi(r).temp = 3500.0f;
						sim->pv[(y+ry)/CELL][(x+rx)/CELL] += 50.0f;
					} else {
						sim->part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
						partsi(r).life = 400;
						partsi(r).ctype = PT_THRM;
						partsi(r).temp = 3500.0f;
						partsi(r).tmp = 20;
					}
					continue;
				}

				if ((rt==PT_COAL) || (rt==PT_BCOL))
				{
					if ((t==PT_FIRE || t==PT_PLSM))
					{
						if (partsi(r).life>100 && !(rand()%500)) {
							partsi(r).life = 99;
						}
					}
				}

				if (t == PT_LAVA)
				{
					int ri = ID(r);
					switch (parts[i].ctype)
					{
					case PT_IRON:
						Element_IRON::makeAlloy(sim, i, rt, ri);
						break;
					case PT_QRTZ: // LAVA(CLST) + LAVA(PQRT) + high enough temp = LAVA(CRMC) + LAVA(CRMC)
						if (rt == PT_LAVA && parts[ri].ctype == PT_CLST)
						{
							float pres = std::max(sim->pv[y/CELL][x/CELL]*10.0f, 0.0f);
							if (parts[i].temp >= pres+sim->elements[PT_CRMC].HighTemperature+50.0f)
							{
								parts[i].ctype = PT_CRMC;
								parts[ri].ctype = PT_CRMC;
							}
						}
						break;
					case PT_HEAC:
						if (rt == PT_HEAC)
						{
							if (parts[ri].temp > sim->elements[PT_HEAC].HighTemperature && rand()%200)
							{
								sim->part_change_type(ri, x+rx, y+ry, PT_LAVA);
								parts[ri].ctype = PT_HEAC;
							}
						}
					case PT_POLO:
						if (rt == PT_LAVA && parts[ri].ctype == PT_POLC)
						{
							if (!sim->legacy_enable)
							{
								parts[ri].temp -= 0.3f;
								parts[i].temp -= 0.3f;
							}
						}
						break;
					}
				}

				if ((surround_space || sim->elements[rt].Explosive) &&
				    sim->elements[rt].Flammable && (sim->elements[rt].Flammable + (int)(sim->pv[(y+ry)/CELL][(x+rx)/CELL] * 10.0f)) > (rand()%1000) &&
				    //exceptions, t is the thing causing the spark and rt is what's burning
				    (t != PT_SPRK || (rt != PT_RBDM && rt != PT_LRBD && rt != PT_INSL)) &&
				    (t != PT_PHOT || rt != PT_INSL) &&
				    (rt != PT_SPNG || partsi(r).life == 0))
				{
					sim->part_change_type(ID(r), x+rx, y+ry, PT_FIRE);
					partsi(r).temp = restrict_flt(sim->elements[PT_FIRE].Temperature + (sim->elements[rt].Flammable/2), MIN_TEMP, MAX_TEMP);
					partsi(r).life = rand()%80+180;
					partsi(r).tmp = partsi(r).ctype = 0;
					if (sim->elements[rt].Explosive)
						sim->pv[y/CELL][x/CELL] += 0.25f * CFDS;
				}
			}
	if (sim->legacy_enable && t!=PT_SPRK) // SPRK has no legacy reactions
		updateLegacy(UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

#define ISPOLO(x) (x == PT_POLO || x == PT_POLC)
#define PFLAG_MIXTURE_FOUND	0x10
#define PFLAG_FREEZING		0x20
						
//#TPT-Directive ElementHeader Element_FIRE static int updateLegacy(UPDATE_FUNC_ARGS)
int Element_FIRE::updateLegacy(UPDATE_FUNC_ARGS) {
	int r, rx, ry, rt, lpv, t = parts[i].type;
	int t1, t2, nflags, rndstore, mt;
	t1 = parts[i].ctype;
	if (t == PT_LAVA)
	{
		if (t1 == PT_PLUT) parts[i].ctype = 0;
		else if (!ISPOLO(t1))
			goto update_fire;
		nflags = parts[i].flags;
		parts[i].flags &= ~PFLAG_MIXTURE_FOUND;
		rndstore = rand();
		bool rndc = false;
		if (parts[i].life <= 0)
		{
			rndc = !(rndstore & 0x7F);
			if (rndc && (nflags & PFLAG_MIXTURE_FOUND))
			{
				lava_freeze(sim, parts[i], i, x, y);
				return 1;
			}
			// otherwise, MIXTURE_NOT_FOUND
		}
		rx = rndstore % 5 - 2;
		rndstore >>= 7;
		ry = rndstore % 5 - 2;
		r = pmap[y+ry][x+rx];
		t2 = partsi(r).ctype;
		if (TYP(r) == PT_LAVA && ISPOLO(t2))
		{
			if (rndc && t1 != t2)
				parts[i].flags |= PFLAG_FREEZING | PFLAG_MIXTURE_FOUND;
			partsi(r).flags |= (parts[i].flags & PFLAG_FREEZING);
		}
		if (nflags & PFLAG_FREEZING && parts[i].life > 0)
			return 1;
	}
update_fire:
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (sim->bmap[(y+ry)/CELL][(x+rx)/CELL] && sim->bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_STREAM)
					continue;
				rt = TYP(r);

				lpv = (int)sim->pv[(y+ry)/CELL][(x+rx)/CELL];
				if (lpv < 1) lpv = 1;
				mt = sim->elements[rt].Meltable;
				if (mt && ((t!=PT_FIRE&&t!=PT_PLSM) || (rt!=PT_METL && rt!=PT_IRON && rt!=PT_ETRD && rt!=PT_PSCN && rt!=PT_NSCN && rt!=PT_NTCT && rt!=PT_PTCT && rt!=PT_BMTL && rt!=PT_BRMT && rt!=PT_SALT && rt!=PT_INWR)) && mt*lpv>(rand()%1000))
				{
					if (t!=PT_LAVA || parts[i].life>0)
					{
						if (rt==PT_BRMT)
							partsi(r).ctype = PT_BMTL;
						else if (rt==PT_SAND)
							partsi(r).ctype = PT_GLAS;
						else
							partsi(r).ctype = rt;
						sim->part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
						partsi(r).life = rand()%120+240;
					}
					else
					{
						lava_freeze(sim, parts[i], i, x, y);
						return 1;
					}
				}
				if (rt==PT_ICEI || rt==PT_SNOW)
				{
					sim->part_change_type(ID(r), x+rx, y+ry, PT_WATR);
					if (t==PT_FIRE)
					{
						sim->kill_part(i);
						return 1;
					}
					if (t==PT_LAVA)
					{
						parts[i].life = 0;
						sim->part_change_type(i,x,y,PT_STNE);
					}
				}
				if (rt==PT_WATR || rt==PT_DSTW || rt==PT_SLTW)
				{
					sim->kill_part(ID(r));
					if (t==PT_FIRE)
					{
						sim->kill_part(i);
						return 1;
					}
					if (t==PT_LAVA)
					{
						lava_freeze(sim, parts[i], i, x, y);
					}
				}
			}
	return 0;
}

//#TPT-Directive ElementHeader Element_FIRE static void lava_freeze(Simulation *sim, Particle &p, int i, int x, int y)
void Element_FIRE::lava_freeze(Simulation *sim, Particle &p, int i, int x, int y)
{
	p.life = 0;
	sim->part_change_type(i, x, y, p.ctype ? p.ctype : PT_STNE);
	p.ctype = 0;
	p.flags &= ~PFLAG_FREEZING;
}

//#TPT-Directive ElementHeader Element_FIRE static int graphics(GRAPHICS_FUNC_ARGS)
int Element_FIRE::graphics(GRAPHICS_FUNC_ARGS)
{
	int caddress = restrict_flt(restrict_flt((float)cpart->life, 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
	*colr = (unsigned char)ren->flm_data[caddress];
	*colg = (unsigned char)ren->flm_data[caddress+1];
	*colb = (unsigned char)ren->flm_data[caddress+2];

	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode = PMODE_NONE; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_ADD;
	//Returning 0 means dynamic, do not cache
	return 0;
}

Element_FIRE::~Element_FIRE() {}
