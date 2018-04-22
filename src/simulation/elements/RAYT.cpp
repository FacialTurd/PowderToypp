#include "simulation/Elements.h"
#include <iostream>
#include "simulation/MULTIPPE_Update.h"
#ifndef ID
#define ID(r) part_ID(r)
#endif

//#TPT-Directive ElementClass Element_RAYT PT_RAYT 186
Element_RAYT::Element_RAYT()
{
	Identifier = "DEFAULT_PT_RAYT";
	Name = "RAYT";
	Colour = PIXPACK(0x66ff66);
	MenuVisible = 1;
	MenuSection = SC_SENSOR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.00f;
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

	Temperature = 283.15f;
	HeatConduct = 0;
	Description = "RAYT scans in 8 directions for the element in its ctype and sparks the conductor on the opposite side";

	Properties = TYPE_SOLID | PROP_DRAWONCTYPE | PROP_NOCTYPEDRAW;
	Properties2 |= PROP_DRAWONCTYPE | PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_RAYT::update;
}

const int mask_invert_filter =  0x1;
const int mask_ignore_energy =  0x2;
const int mask_no_copy_color =  0x4;
const int mask_keep_searching = 0x8;

//NOTES:
// ctype is used to store the target element, if any. (NONE is treated as a wildcard)
// life is used for the amount of pixels to skip before starting the scan. Starts just in front of the RAYT if 0.
// tmp is the number of particles that will be scanned before scanning stops. Unbounded if 0.
// tmp2 is used for settings (binary flags). The flags are as follows:
// 0x01: Inverts the CTYPE filter so that the element in ctype is the only thing that doesn't trigger RAYT, instead of the opposite.
// 0x02: Ignore energy particles
// 0x04: Ignore FILT (do not use color copying mode)
// 0x08: Keep searching even after finding a particle


/* Returns true for particles that activate the special FILT color copying mode */
bool phot_data_type(int rt)
{
	if (rt == PT_FILT || rt == PT_PHOT || rt == PT_BRAY)
		return true;
	return false;
}

//#TPT-Directive ElementHeader Element_RAYT static int update(UPDATE_FUNC_ARGS)
int Element_RAYT::update(UPDATE_FUNC_ARGS)
{
	int max = parts[i].tmp + parts[i].life, rt;
	bool acc_typ;
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if (BOUNDS_CHECK && (rx || ry))
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = TYP(r);

				acc_typ = MULTIPPE_Update::isAcceptedConductor_i(sim, r);
				if (!acc_typ && ((parts[i].tmp2 & mask_no_copy_color) || !phot_data_type(rt)))
					continue;

				// Stolen from DRAY, does the ray searching
				int xStep = rx * -1, yStep = ry * -1;
				int xCurrent = x + (xStep * (parts[i].life + 1)), yCurrent = y + (yStep * (parts[i].life + 1));
				for (;(parts[i].tmp == 0) || !(xCurrent - x >= max) || (yCurrent-y >= max); xCurrent += xStep, yCurrent += yStep)
				{
					int rr = pmap[yCurrent][xCurrent];
					if (!(xCurrent>=0 && yCurrent>=0 && xCurrent<XRES && yCurrent<YRES))
					{
						break; // We're out of bounds! Oops!
					}
					if (!rr)
					{
						rr = sim->photons[yCurrent][xCurrent];
						if (!(rr && !(parts[i].tmp2 & mask_ignore_energy)))
						{
							continue;
						}
					}
					if (!rr)
						continue;

					// Usual DTEC-like mode
					if (!phot_data_type(rt))
					{
						// If ctype isn't set (no type restriction), or ctype matches what we found
						// Can use .tmp2 flag to invert this
						if (parts[i].ctype == 0 || (parts[i].ctype == TYP(rr)) ^ !!(parts[i].tmp2 & mask_invert_filter))
						{
							// if (!acc_typ) break;
							r = ID(r);
							parts[r].life = 4;
							parts[r].ctype = rt;
							sim->part_change_type(r, x+rx, y+ry, PT_SPRK); // Fixed RAYT!
							break;
						}
						// room for more conditions here.
					}
					// FILT color copying mode
					else
					{
						// If ctype isn't set (no type restriction), or ctype matches what we found
						// Can use .tmp2 flag to invert this
						if (parts[i].ctype == 0 || (parts[i].ctype == TYP(rr)) ^ !!(parts[i].tmp2 & mask_invert_filter))
						{
							if (phot_data_type(TYP(rr)))
							{
								int nx = x + rx, ny = y + ry;
								Element_MULTIPP::setFilter(sim, nx, ny, rx, ry, Element_FILT::getWavelengths(&parts[ID(rr)]));
								break;
							}
						}
					}
					if (!(parts[i].tmp2 & mask_keep_searching))
						break;
				}
			}
		}
	}
	return 0;
}

Element_RAYT::~Element_RAYT() {}
