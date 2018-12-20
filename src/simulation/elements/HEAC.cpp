#include <algorithm>
#include <functional>
#include "simulation/Elements.h"
#include "simulation/Air.h"

//#TPT-Directive ElementClass Element_HEAC PT_HEAC 180
Element_HEAC::Element_HEAC()
{
	Identifier = "DEFAULT_PT_HEAC";
	Name = "HEAC";
	Colour = PIXPACK(0xCB6351);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
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
	Meltable = 1;
	Hardness = 0;

	Weight = 100;

	Temperature = R_TEMP+273.15f;
	HeatConduct = 251;
	Description = "Rapid heat conductor.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// can't melt by normal heat conduction, this is used by other elements for special melting behavior
	HighTemperature = 1887.15f;
	HighTemperatureTransition = NT;

	Update = &Element_HEAC::update;
}

//#TPT-Directive ElementHeader Element_HEAC struct IsInsulator
struct Element_HEAC::IsInsulator : public std::binary_function<Simulation*,int,bool> {
  bool operator() (Simulation* a, int b)
  {
	  return b && !a->GetHeatConduct(ID(b), TYP(b)) && !(TYP(b) == ELEM_MULTIPP && a->parts[ID(b)].life == 6);
  }
};
//#TPT-Directive ElementHeader Element_HEAC static IsInsulator isInsulator
Element_HEAC::IsInsulator Element_HEAC::isInsulator = Element_HEAC::IsInsulator();

// If this is used elsewhere (GOLD), it should be moved into Simulation.h
//#TPT-Directive ElementHeader Element_HEAC template<class BinaryPredicate> static bool CheckLine(Simulation* sim, int x1, int y1, int x2, int y2, BinaryPredicate func)
template<class BinaryPredicate>
bool Element_HEAC::CheckLine(Simulation* sim, int x1, int y1, int x2, int y2, BinaryPredicate func)
{
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy;
	float e, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
		{
			if (func(sim, sim->pmap[x][y])) return true;
		}
		else
		{
			if (func(sim, sim->pmap[y][x])) return true;
		}
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((y1<y2) ? (y<=y2) : (y>=y2))
			{
				if (reverseXY)
				{
					if (func(sim, sim->pmap[x][y])) return true;
				}
				else
				{
					if (func(sim, sim->pmap[y][x])) return true;
				}
			}
			e -= 1.0f;
		}
	}
	return false;
}

//#TPT-Directive ElementHeader Element_HEAC static int update(UPDATE_FUNC_ARGS)
int Element_HEAC::update(UPDATE_FUNC_ARGS)
{
	const int rad = 4;
	int rry, rrx, r, ri, count = 0;
	float tempAgg = 0;
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			rry = ry * rad;
			rrx = rx * rad;
			if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !Element_HEAC::CheckLine<Element_HEAC::IsInsulator>(sim, x, y, x+rrx, y+rry, isInsulator))
			{
				r = pmap[y+rry][x+rrx];
				if (r)
  				{
					ri = ID(r);
					int rt = TYP(r);
					if (sim->GetHeatConduct(ri, rt))
					{
						count++;
						tempAgg += parts[ri].temp;
					}
					else if (rt == ELEM_MULTIPP && parts[ri].life == 6)
					{
						parts[i].temp = parts[ri].temp;
						goto conductFromHeater;
					}
  				}
				r = sim->photons[y+rry][x+rrx];
				ri = ID(r);
				if (r && sim->GetHeatConduct(ri, TYP(r)))
				{
					count++;
					tempAgg += parts[ID(r)].temp;
				}
			}
		}
	}

	if (count > 0)
	{
		parts[i].temp = tempAgg/count;

	conductFromHeater:
		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				rry = ry * rad;
				rrx = rx * rad;
				if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !Element_HEAC::CheckLine<Element_HEAC::IsInsulator>(sim, x, y, x+rrx, y+rry, isInsulator))
					sim->pmap_heatconduct(x+rrx, y+rry, parts[i].temp);
			}
		}
	}

	return 0;
}


Element_HEAC::~Element_HEAC() {}
