#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_FILT PT_FILT 125
Element_FILT::Element_FILT()
{
	Identifier = "DEFAULT_PT_FILT";
	Name = "FILT";
	Colour = PIXPACK(0x000056);
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
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 251;
	Description = "Filter for photons, changes the color.";

	Properties = TYPE_SOLID | PROP_NOAMBHEAT | PROP_LIFE_DEC | PROP_TRANSPARENT;
	Properties2 = PROP_INVISIBLE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = NULL;
	Graphics = &Element_FILT::graphics;
}

//#TPT-Directive ElementHeader Element_FILT static int graphics(GRAPHICS_FUNC_ARGS)
int Element_FILT::graphics(GRAPHICS_FUNC_ARGS)
{
	int x, wl = Element_FILT::getWavelengths(cpart);
	*colg = 0;
	*colb = 0;
	*colr = 0;
	for (x=0; x<12; x++) {
		*colr += (wl >> (x+18)) & 1;
		*colb += (wl >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (wl >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	if (cpart->life>0 && cpart->life<=4)
		*cola = 127+cpart->life*30;
	else
		*cola = 127;
	*colr *= x;
	*colg *= x;
	*colb *= x;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND;
	return 0;
}

#define FILT_NORMAL_OPERATIONS 12

//#TPT-Directive ElementHeader Element_FILT static int interactWavelengths(Particle* cpart, int origWl)
// Returns the wavelengths in a particle after FILT interacts with it (e.g. a photon)
// cpart is the FILT particle, origWl the original wavelengths in the interacting particle
int Element_FILT::interactWavelengths(Particle* cpart, int origWl)
{
	const int mask = 0x3FFFFFFF;
	int filtWl = getWavelengths(cpart);
	switch (cpart->tmp)
	{
		case 0:
			return filtWl; //Assign Colour
		case 1:
			return origWl & filtWl; //Filter Colour
		case 2:
			return origWl | filtWl; //Add Colour
		case 3:
			return origWl & (~filtWl); //Subtract colour of filt from colour of photon
		case 4:
		{
			int shift = int((cpart->temp-273.0f)*0.025f);
			if (shift<=0) shift = 1;
			return (origWl << shift) & mask; // red shift
		}
		case 5:
		{
			int shift = int((cpart->temp-273.0f)*0.025f);
			if (shift<=0) shift = 1;
			return (origWl >> shift) & mask; // blue shift
		}
		case 6:
			return origWl; // No change
		case 7:
			return origWl ^ filtWl; // XOR colours
		case 8:
			return (~origWl) & mask; // Invert colours
		case 9:
		{
			int t1 = (origWl & 0x0000FF)+(rand()%5)-2;
			int t2 = ((origWl & 0x00FF00)>>8)+(rand()%5)-2;
			int t3 = ((origWl & 0xFF0000)>>16)+(rand()%5)-2;
			return (origWl & 0xFF000000) | (t3<<16) | (t2<<8) | t1;
		}
		case 10:
		{
			long long int lsb = filtWl & (-filtWl);
			return (origWl * lsb) & 0x3FFFFFFF; //red shift
		}
		case 11:
		{
			long long int lsb = filtWl & (-filtWl);
			return (origWl / lsb) & 0x3FFFFFFF; // blue shift
		}
		//--- custom part ---//
		case (FILT_NORMAL_OPERATIONS + 0):
		{
			return origWl | (~filtWl & mask);
		}
		case (FILT_NORMAL_OPERATIONS + 1): // Arithmetic addition
		{
			return ((origWl + filtWl) | 0x20000000) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 2): // Arithmetic subtraction
		{
			return ((origWl - filtWl) | 0x20000000) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 3): // Arithmetic multiply
		{
			return ((origWl * filtWl) | 0x20000000) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 4): // rotate red shift
		{
			long long int lsb = filtWl & (-filtWl);
			return ((origWl * lsb) | (origWl / (0x40000000 / lsb))) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 5): // rotate blue shift
		{
			long long int lsb = filtWl & (-filtWl);
			return ((origWl / lsb) | (origWl * (0x40000000 / lsb))) & 0x3FFFFFFF;
		}
		case (FILT_NORMAL_OPERATIONS + 6): // set flag 0
		{
			long long int lsb = filtWl & (-filtWl);
			return origWl & ~lsb;
		}
		case (FILT_NORMAL_OPERATIONS + 7): // set flag 1
		{
			long long int lsb = filtWl & (-filtWl);
			return origWl | lsb;
		}
		case (FILT_NORMAL_OPERATIONS + 8): // toggle flag
		{
			long long int lsb = filtWl & (-filtWl);
			return origWl ^ lsb;
		}
		case (FILT_NORMAL_OPERATIONS + 9): // random toggle
		{
			if (rand() & 1)
			{
				long long int lsb = filtWl & (-filtWl);
				return origWl ^ lsb;
			}
		}
		case (FILT_NORMAL_OPERATIONS + 10): // reversing wavelength from "Hacker's Delight"
		{
			int r1, r2;
			r1  = origWl;
			r2  = (r1 << 15) | (r1 >> 15); // wavelength rotate 15
			r1  = (r2 ^ (r2>>10)) & 0x000F801F; // swap 10
			r2 ^= (r1 | (r1<<10));
			r1  = (r2 ^ (r2>> 3)) & 0x06318C63; // swap 3
			r2 ^= (r1 | (r1<< 3));
			r1  = (r2 ^ (r2>> 1)) & 0x1294A529; // swap 1
			return = (r1 | (r1<< 1)) ^ r2;
		}
		case (FILT_NORMAL_OPERATIONS + 11): // random wavelength
		{
			int r1 = rand();
			r1 += (rand() << 15);
			if ((r1 ^ origWl) & mask == 0)
				return origWl;
			else
				return (origWl ^ r1) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 12): // get "extraLoopsCA" info, without pause state
		{
			int r1 = 0;
			if (!sim->extraLoopsCA)
				r1 = 0x1;
			else
				r1 = 0x2 << sim->extraLoopsType;
			if (sim->elementCount[PT_LOVE] > 0)
				r1 |= 0x10;
			if (sim->elementCount[PT_LOLZ] > 0)
				r1 |= 0x20;
			if (sim->elementCount[PT_WIRE] > 0)
				r1 |= 0x40;
			if (sim->elementCount[PT_LIFE] > 0)
				r1 |= 0x80;
			if (sim->player.spwn)
				r1 |= 0x100;
			if (sim->player2.spwn)
				r1 |= 0x200;
			if (sim->elementCount[PT_WIFI] > 0)
				r1 |= 0x400;
			if (sim->elementCount[PT_DMND] > 0)
				r1 |= 0x800;
			if (sim->elementCount[PT_INSL] > 0)
				r1 |= 0x1000;
			if (sim->elementCount[PT_INDI] > 0)
				r1 |= 0x2000;
			if (sim->elementCount[PT_PINS] > 0)
				r1 |= 0x4000;
			if (sim->elementCount[PT_PINVIS] > 0)
				r1 |= 0x8000;
			if (sim->elementCount[PT_INDC] > 0)
				r1 |= 0x10000;
			return = r1;
		}
		default:
			return filtWl;
	}
}

//#TPT-Directive ElementHeader Element_FILT static int getWavelengths(Particle* cpart)
int Element_FILT::getWavelengths(Particle* cpart)
{
	if (cpart->ctype&0x3FFFFFFF)
	{
		return cpart->ctype;
	}
	else
	{
		int temp_bin = (int)((cpart->temp-273.0f)*0.025f);
		if (temp_bin < 0) temp_bin = 0;
		if (temp_bin > 25) temp_bin = 25;
		return (0x1F << temp_bin);
	}
}

Element_FILT::~Element_FILT() {}
