#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_DRAY PT_DRAY 178
Element_DRAY::Element_DRAY()
{
	Identifier = "DEFAULT_PT_DRAY";
	Name = "DRAY";
	Colour = PIXPACK(0xFFAA22);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
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

	HeatConduct = 0;
	Description = "Duplicator ray. Replicates a line of particles in front of it.";

	Properties = TYPE_SOLID|PROP_LIFE_DEC;
	Properties2 = PROP_DRAWONCTYPE | PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_DRAY::update;
	Graphics = NULL;
}

//should probably be in Simulation.h
bool InBounds(int x, int y)
{
	return (x>=0 && y>=0 && x<XRES && y<YRES);
}

//#TPT-Directive ElementHeader Element_DRAY static int update(UPDATE_FUNC_ARGS)
int Element_DRAY::update(UPDATE_FUNC_ARGS)
{
	int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype), copyLength = parts[i].tmp, copySpaces = parts[i].tmp2;
	if (copySpaces < 0)
		copySpaces = parts[i].tmp2 = 0;
	if (copyLength < 0)
		copyLength = parts[i].tmp = 0;
	else if (copyLength > 0)
		copySpaces++; //strange hack
	if (!parts[i].life) // only fire when life is 0, but nothing sets the life right now
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					int rt = TYP(r);
					r = ID(r);

					if (rt == PT_SPRK && parts[r].life == 3) //spark found, start creating
					{
						bool overwrite = parts[r].ctype == PT_PSCN;
						int partsRemaining = copyLength, xCopyTo = -1, yCopyTo = -1; //positions where the line will start being copied at
						int localCopyLength = copyLength;
						int rr;

						if (parts[r].ctype == PT_INWR && rx && ry) // INWR doesn't spark from diagonals
							continue;

						//figure out where the copying will start/end
						bool foundParticle = false;
						bool isEnergy = false;
						for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; ; xCurrent+=xStep, yCurrent+=yStep)
						{
							// Out of bounds, stop looking and don't copy anything
							if (!sim->InBounds(xCurrent, yCurrent))
								break;
							// haven't found a particle yet, keep looking for one
							// the first particle it sees decides whether it will copy energy particles or not
							if (!foundParticle)
							{
								rr = pmap[yCurrent][xCurrent];
								if (!rr)
								{
									rr = sim->photons[yCurrent][xCurrent];
									if (rr)
										foundParticle = isEnergy = true;
								}
								else
									foundParticle = true;
							}
							// now that it knows what kind of particle it is copying, do some extra stuff here so we can determine when to stop
							if ((ctype && sim->elements[ctype].Properties&TYPE_ENERGY) || isEnergy)
								rr = sim->photons[yCurrent][xCurrent];
							else
								rr = pmap[yCurrent][xCurrent];

							// Checks for when to stop:
							//  1: if .tmp isn't set, and the element in this spot is the ctype, then stop
							//  2: if .tmp is set, stop when the length limit reaches 0
							if ((!localCopyLength && TYP(rr) == ctype && (ctype != PT_LIFE || parts[ID(rr)].ctype == ctypeExtra))
									|| !--partsRemaining)
							{
								localCopyLength -= partsRemaining;
								xCopyTo = xCurrent + xStep*copySpaces;
								yCopyTo = yCurrent + yStep*copySpaces;
								break;
							}
						}

						// now, actually copy the particles
						partsRemaining = localCopyLength + 1;
						int type, p;
						for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; InBounds(xCopyTo, yCopyTo) && --partsRemaining; xCurrent+=xStep, yCurrent+=yStep, xCopyTo+=xStep, yCopyTo+=yStep)
						{
							// get particle to copy
							if (isEnergy)
								rr = sim->photons[yCurrent][xCurrent];
							else
								rr = pmap[yCurrent][xCurrent];
							type = TYP(rr);

							// if sparked by PSCN, overwrite whatever is in the target location, instead of just ignoring it
							if (overwrite)
							{
								int r2;
								if (isEnergy)
								{
									if (r2 = sim->photons[yCopyTo][xCopyTo])
										sim->kill_part(ID(r2));
								}
								else
								{
									r2 = pmap[yCopyTo][xCopyTo];
									if (r2 && !(sim->elements[TYP(r2)].Properties2 & PROP_NODESTRUCT))
										sim->kill_part(ID(r2));
								}
							}
							
							if (!type)
								continue;
							if (isEnergy)
								sim->duplicate_part(xCopyTo, yCopyTo, parts[ID(rr)]);
							else
								sim->duplicate_part(xCopyTo, yCopyTo, parts[ID(rr)]);
						}
					}
				}
	}
	return 0;
}

Element_DRAY::~Element_DRAY() {}
