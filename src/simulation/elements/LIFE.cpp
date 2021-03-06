#include "simulation/Elements.h"

bool Element_GOL_colourInit = false;
// pixel Element_GOL_colour[NGOL];

//#TPT-Directive ElementClass Element_LIFE PT_LIFE 78
Element_LIFE::Element_LIFE()
{
	Identifier = "DEFAULT_PT_LIFE";
	Name = "LIFE";
	Colour = PIXPACK(0x0CAC00);
	MenuVisible = 0;
	MenuSection = SC_LIFE;
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
	Hardness = 0;

	Weight = 100;

	DefaultProperties.temp = 9000.0f;
	HeatConduct = 40;
	Description = "Game Of Life! B3/S23";

	Properties = TYPE_SOLID|PROP_LIFE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_LIFE::update;
	Graphics = &Element_LIFE::graphics;

	if(!Element_GOL_colourInit)
	{
		Element_GOL_colourInit = true;

		int golMenuCount;
		gol_menu * golMenuT = LoadGOLMenu(golMenuCount);
		for(int i = 0; i < golMenuCount && i < NGOL; i++)
		{
			Element_LIFE::Element_GOL_colour[i] = golMenuT[i].colour;
		}
		free(golMenuT);
	}
}

//#TPT-Directive ElementHeader Element_LIFE static pixel Element_GOL_colour[NGOL];
pixel Element_LIFE::Element_GOL_colour[NGOL];

//#TPT-Directive ElementHeader Element_LIFE static pixel customColorGradF;
pixel Element_LIFE::customColorGradF = PIXPACK(0xFF00FF);
//#TPT-Directive ElementHeader Element_LIFE static pixel customColorGradT;
pixel Element_LIFE::customColorGradT = PIXPACK(0x330033);

//#TPT-Directive ElementHeader Element_LIFE static int update(UPDATE_FUNC_ARGS)
int Element_LIFE::update(UPDATE_FUNC_ARGS)
{
	parts[i].temp = restrict_flt(parts[i].temp-50.0f, MIN_TEMP, MAX_TEMP);
	return 0;
}

//#TPT-Directive ElementHeader Element_LIFE static int graphics(GRAPHICS_FUNC_ARGS)
int Element_LIFE::graphics(GRAPHICS_FUNC_ARGS)

{
	pixel pc;
	int golstates, currstate;
	if (cpart->ctype==NGT_LOTE)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(255, 128, 0);
		else if (cpart->tmp==1)
			pc = PIXRGB(255, 255, 0);
		else
			pc = PIXRGB(255, 0, 0);
	}
	else if (cpart->ctype==NGT_FRG2)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 50);
		else
			pc = PIXRGB(0, 255, 90);
	}
	else if (cpart->ctype==NGT_STAR)//colors for life states
	{
		if (cpart->tmp==4)
			pc = PIXRGB(0, 0, 128);
		else if (cpart->tmp==3)
			pc = PIXRGB(0, 0, 150);
		else if (cpart->tmp==2)
			pc = PIXRGB(0, 0, 190);
		else if (cpart->tmp==1)
			pc = PIXRGB(0, 0, 230);
		else
			pc = PIXRGB(0, 0, 70);
	}
	else if (cpart->ctype==NGT_FROG)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 0);
		else
			pc = PIXRGB(0, 255, 0);
	}
	else if (cpart->ctype==NGT_BRAN)//colors for life states
	{
		if (cpart->tmp==1)
			pc = PIXRGB(150, 150, 0);
		else
			pc = PIXRGB(255, 255, 0);
	}
	else if (cpart->ctype==NGT_CUSTOM)
	{
		golstates = ren->sim->grule[cpart->ctype+1][9];
		currstate = cpart->tmp;
		if (currstate >= golstates - 1)
			goto default_part;
		pc = customColorGradF;
		*colr = PIXR(pc); *colg = PIXG(pc); *colb = PIXB(pc);
		if (golstates > 3)
		{
			float multiplier = (float)(golstates - 2 - currstate) / (float)(golstates - 3);
			pc = customColorGradT;
			*colr -= (int)(*colr - PIXR(pc)) * multiplier;
			*colg -= (int)(*colg - PIXG(pc)) * multiplier;
			*colb -= (int)(*colb - PIXB(pc)) * multiplier;
		}
		return 0;
	}
	else if (cpart->ctype >= 0 && cpart->ctype < NGOL)
	{
	default_part:
		pc = Element_GOL_colour[cpart->ctype];
	}
	else
		pc = ren->sim->elements[cpart->type].Colour;
	*colr = PIXR(pc);
	*colg = PIXG(pc);
	*colb = PIXB(pc);
	return 0;
}


Element_LIFE::~Element_LIFE() {}
