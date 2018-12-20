#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_STOR PT_STOR 83
Element_STOR::Element_STOR()
{
	Identifier = "DEFAULT_PT_STOR";
	Name = "STOR";
	Colour = PIXPACK(0x50DFDF);
	MenuVisible = 1;
	MenuSection = SC_POWERED;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 0;
	Description = "Storage. Captures and stores a single particle. Releases when charged with PSCN, also passes to PIPE.";

	Properties = TYPE_SOLID|PROP_NOCTYPEDRAW;
	Properties2 = PROP_DRAWONCTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_STOR::update;
	Graphics = &Element_STOR::graphics;
}

//#TPT-Directive ElementHeader Element_STOR static int update(UPDATE_FUNC_ARGS)
int Element_STOR::update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, np, rx1, ry1;
	if (!sim->IsValidElement(parts[i].tmp))
		parts[i].tmp = 0;
	if(parts[i].life && !parts[i].tmp)
		parts[i].life--;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (ID(r) >= NPART || !r)
					continue;
				int rt = TYP(r);
				r >>= PMAPBITS;
				if (!parts[i].tmp && !parts[i].life && rt!=PT_STOR && !(sim->elements[rt].Properties&TYPE_SOLID) && (!parts[i].ctype || rt == parts[i].ctype))
				{
					if (rt == PT_SOAP)
						Element_SOAP::detach(sim, r);
					parts[i].tmp = parts[r].type;
					parts[i].temp = parts[r].temp;
					parts[i].tmp2 = parts[r].life;
					parts[i].tmp3 = parts[r].tmp;
					parts[i].tmp4 = parts[r].ctype;
					parts[i].pavg[0] = parts[r].tmp2;
					parts[i].pavg[1] = parts[r].tmp3;
					parts[i].cdcolour = parts[r].dcolour;
					sim->kill_part(r);
				}
				if (parts[i].tmp && rt == PT_SPRK && parts[r].ctype==PT_PSCN && parts[r].life>0 && parts[r].life<4)
				{
					for(ry1 = 1; ry1 >= -1; ry1--){
						for(rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1){ // Oscillate the X starting at 0, 1, -1, 3, -5, etc (Though stop at -1)
							np = sim->create_part(-1, x+rx1, y+ry1, TYP(parts[i].tmp));
							if (np!=-1)
							{
								parts[np].temp = parts[i].temp;
								parts[np].life = parts[i].tmp2;
								/* original code:
								parts[np].tmp = parts[i].pavg[0];
								parts[np].ctype = parts[i].pavg[1];
								parts[np].tmp2 = parts[i].tmp3;
								parts[np].tmp3 = parts[i].tmp4;
								*/
								parts[np].tmp = parts[i].tmp3;
								parts[np].ctype = parts[i].tmp4;
								parts[np].tmp2 = parts[i].pavg[0];
								parts[np].tmp3 = parts[i].pavg[1];
								parts[np].dcolour = parts[i].cdcolour;
								parts[i].tmp = 0;
								parts[i].life = 10;
								break;
							}
						}
					}
				}
			}
	return 0;
}


//#TPT-Directive ElementHeader Element_STOR static int graphics(GRAPHICS_FUNC_ARGS)
int Element_STOR::graphics(GRAPHICS_FUNC_ARGS)

{
	if(cpart->tmp){
		*pixel_mode |= PMODE_GLOW;
		*colr = 0x50;
		*colg = 0xDF;
		*colb = 0xDF;
	} else {
		*colr = 0x20;
		*colg = 0xAF;
		*colb = 0xAF;
	}
	return 0;
}


Element_STOR::~Element_STOR() {}
