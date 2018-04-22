#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_DMG PT_DMG 163
Element_DMG::Element_DMG()
{
	Identifier = "DEFAULT_PT_DMG";
	Name = "DMG";
	Colour = PIXPACK(0x88FF88);
	MenuVisible = 1;
	MenuSection = SC_FORCE;
	Enabled = 1;

	Advection = 0.0f;
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
	Description = "Generates damaging pressure and breaks any elements it hits.";

	Properties = TYPE_PART|PROP_SPARKSETTLE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_DMG::update;
	Graphics = &Element_DMG::graphics;
	
	if (brk_array_len == 0)
	{
		int i, j, k, v;
		for (i = 0; brk_sparse_arr[i][0] != 0; i++) { }
		brk_array_len = i;

		for (j = 0; j < i - 1; j ++) // 选择排序
		{
			for (k = j + 1; k < i; k ++)
			{
				if (brk_sparse_arr[j][0] > brk_sparse_arr[k][0])
				{
					v = brk_sparse_arr[j][0];
					brk_sparse_arr[j][0] = brk_sparse_arr[k][0];
					brk_sparse_arr[k][0] = v;
					v = brk_sparse_arr[j][1];
					brk_sparse_arr[j][1] = brk_sparse_arr[k][1];
					brk_sparse_arr[k][1] = v;
				}
			}
		}
	}
}

//#TPT-Directive ElementHeader Element_DMG static int brk_array_len
int Element_DMG::brk_array_len = 0;

//#TPT-Directive ElementHeader Element_DMG static int brk_sparse_arr[][2]
int Element_DMG::brk_sparse_arr[][2] = { // sparse array
	{PT_BMTL, PT_BRMT},
	{PT_GLAS, PT_BGLA},
	{PT_COAL, PT_BCOL},
	{PT_QRTZ, PT_PQRT},
	{PT_TUNG, PT_BRMT},
	{PT_WOOD, PT_SAWD},
	{PT_WIFI, PT_BRMT}, // Added WIFI + DMG -> BRMT for compatible with official TPT.
	{0, 0}
};

//#TPT-Directive ElementHeader Element_DMG static void BreakingElement(Simulation * sim, Particle * parts, int i, int x, int y, int t)
void Element_DMG::BreakingElement(Simulation * sim, Particle * parts, int i, int x, int y, int t)
{
	if (sim->elements[t].HighPressureTransition>-1 && sim->elements[t].HighPressureTransition<PT_NUM)
		sim->part_change_type(i, x, y, sim->elements[t].HighPressureTransition);
	else
	{
		int lt = 0, rt = brk_array_len;

		while (lt != rt) // 二分查找
		{
			int c = (lt + rt) >> 1;
			int st = brk_sparse_arr[c][0];
			if (t == st)
			{
				sim->part_change_type(i, x, y, brk_sparse_arr[c][1]);
				if (t == PT_TUNG)
					parts[i].ctype = PT_TUNG;
				break;
			}
			if (t < st)
				rt = c;
			else // t > st
				lt = c + 1;
		}
	}
}

//#TPT-Directive ElementHeader Element_DMG static int update(UPDATE_FUNC_ARGS)
int Element_DMG::update(UPDATE_FUNC_ARGS)
{
	int r, rr, rx, ry, nxi, nxj, t, dist;
	int rad = 25;
	float angle, fx, fy;

	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)!=PT_DMG && TYP(r)!=PT_EMBR && !(sim->elements[TYP(r)].Properties2 & (PROP_NODESTRUCT|PROP_CLONE)))
				{
					sim->kill_part(i);
					for (nxj=-rad; nxj<=rad; nxj++)
						for (nxi=-rad; nxi<=rad; nxi++)
							if (x+nxi>=0 && y+nxj>=0 && x+nxi<XRES && y+nxj<YRES && (nxi || nxj))
							{
								dist = sqrt(pow(nxi, 2.0f)+pow(nxj, 2.0f));//;(pow((float)nxi,2))/(pow((float)rad,2))+(pow((float)nxj,2))/(pow((float)rad,2));
								if (!dist || (dist <= rad))
								{
									rr = pmap[y+nxj][x+nxi]; 
									if (rr)
									{
										angle = atan2((float)nxj, nxi);
										fx = cos(angle) * 7.0f;
										fy = sin(angle) * 7.0f;
										partsi(rr).vx += fx;
										partsi(rr).vy += fy;
										sim->vx[(y+nxj)/CELL][(x+nxi)/CELL] += fx;
										sim->vy[(y+nxj)/CELL][(x+nxi)/CELL] += fy;
										sim->pv[(y+nxj)/CELL][(x+nxi)/CELL] += 1.0f;
										t = TYP(rr);
										if (t)
											Element_DMG::BreakingElement(sim, parts, part_ID(rr), x+nxi, y+nxj, t);
									}
								}
							}
					return 1;
				}
			}
	return 0;
}


//#TPT-Directive ElementHeader Element_DMG static int graphics(GRAPHICS_FUNC_ARGS)
int Element_DMG::graphics(GRAPHICS_FUNC_ARGS)

{
	*pixel_mode |= PMODE_FLARE;
	return 1;
}


Element_DMG::~Element_DMG() {}
