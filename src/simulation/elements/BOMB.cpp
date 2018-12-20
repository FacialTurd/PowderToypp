#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_BOMB PT_BOMB 129
Element_BOMB::Element_BOMB()
{
	Identifier = "DEFAULT_PT_BOMB";
	Name = "BOMB";
	Colour = PIXPACK(0xFFF288);
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 30;

	Temperature = R_TEMP-2.0f	+273.15f;
	HeatConduct = 29;
	Description = "Bomb. Explodes and destroys all surrounding particles when it touches something.";

	Properties = TYPE_PART|PROP_SPARKSETTLE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_BOMB::update;
	Graphics = &Element_BOMB::graphics;
}

//#TPT-Directive ElementHeader Element_BOMB static int update(UPDATE_FUNC_ARGS)
int Element_BOMB::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, nb;

	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);
				if (rt!=PT_BOMB && rt!=PT_EMBR && !(sim->elements[rt].Properties2 & (PROP_NODESTRUCT|PROP_CLONE)) && rt!=PT_VIBR
					&& (rt!=PT_SPRK || !(sim->elements[partsi(r).ctype].Properties2 & PROP_NODESTRUCT)))
				{
					int rad = 8, nt;
					int nxi, nxj;
					pmap[y][x] = 0;
					for (nxj=-rad; nxj<=rad; nxj++)
						for (nxi=-rad; nxi<=rad; nxi++)
							if ((pow((float)nxi,2))/(pow((float)rad,2))+(pow((float)nxj,2))/(pow((float)rad,2))<=1)
							{
								int ynxj = y + nxj, xnxi = x + nxi;
								
								if ((ynxj < 0) || (ynxj >= YRES) || (xnxi <= 0) || (xnxi >= XRES))
									continue;
								
								int rr = pmap[ynxj][xnxi];
								nt = TYP(rr);
								if (!(sim->elements[ nt ].Properties2 & (PROP_NODESTRUCT|PROP_CLONE)) && nt!=PT_VIBR && (nt!=ELEM_MULTIPP || (partsi(rr).life&~0x1)!=8)
									&& (nt!=PT_SPRK || !(sim->elements[partsi(rr).ctype].Properties2 & PROP_NODESTRUCT)))
								{
									if (nt)
										sim->kill_part(ID(rr));
									sim->pv[(ynxj)/CELL][(xnxi)/CELL] += 0.1f;
									nb = sim->create_part(-3, xnxi, ynxj, PT_EMBR);
									if (nb!=-1)
									{
										parts[nb].tmp = 2;
										parts[nb].life = 2;
										parts[nb].temp = MAX_TEMP;
									}
								}
							}
					for (nxj=-(rad+1); nxj<=(rad+1); nxj++)
						for (nxi=-(rad+1); nxi<=(rad+1); nxi++)
							if ((pow((float)nxi,2))/(pow((float)(rad+1),2))+(pow((float)nxj,2))/(pow((float)(rad+1),2))<=1 && !TYP(pmap[y+nxj][x+nxi]))
							{
								nb = sim->create_part(-3, x+nxi, y+nxj, PT_EMBR);
								if (nb!=-1)
								{
									parts[nb].tmp = 0;
									parts[nb].life = 50;
									parts[nb].temp = MAX_TEMP;
									parts[nb].vx = rand()%40-20;
									parts[nb].vy = rand()%40-20;
								}
							}
					sim->kill_part(i);
					return 1;
				}
			}
	return 0;
}


//#TPT-Directive ElementHeader Element_BOMB static int graphics(GRAPHICS_FUNC_ARGS)
int Element_BOMB::graphics(GRAPHICS_FUNC_ARGS)

{
	*pixel_mode |= PMODE_FLARE;
	return 1;
}


Element_BOMB::~Element_BOMB() {}
