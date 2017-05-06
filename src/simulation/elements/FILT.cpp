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
	Properties2 = PROP_INVISIBLE | PROP_CTYPE_WAVEL;

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

int my_clz (int value)
{
#ifdef __GNUC__
	int x = __builtin_clz (value);
#else
	int x = 0;
	unsigned int xx = 0x80000000;
	while (!(value & xx))
	{
		xx >>= 1; x ++;
	}
#endif
	return x;
}

int my_ctz (int value)
{
#ifdef __GNUC__
	int x = __builtin_ctz (value);
#else
	int x = 0;
	int xx = 1;
	while (!(value & xx))
	{
		xx <<= 1; x ++;
	}
#endif
	return x;
}

int my_popc (int value)
{
#ifdef __GNUC__
	int x = __builtin_popcount (value);
#else
	int x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = (x * 0x01010101) >> 24;
#endif
	return x;
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
		case (FILT_NORMAL_OPERATIONS + 0): // random wavelength
		{
			int r1 = rand();
			r1 += (rand() << 15);
			if ((r1 ^ origWl) & mask == 0)
				return origWl;
			else
				return (origWl ^ r1) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 1): // reversing wavelength from "Hacker's Delight"
		{
			int r1, r2;
			r1  = origWl;
			r2  = (r1 << 15) | (r1 >> 15); // wavelength rotate 15
			r1  = (r2 ^ (r2>>10)) & 0x000F801F; // swap 10
			r2 ^= (r1 | (r1<<10));
			r1  = (r2 ^ (r2>> 3)) & 0x06318C63; // swap 3
			r2 ^= (r1 | (r1<< 3));
			r1  = (r2 ^ (r2>> 1)) & 0x1294A529; // swap 1
			return (r1 | (r1<< 1)) ^ r2;
		}
		case (FILT_NORMAL_OPERATIONS + 2): // population count
		{
			return 1 << (my_popc(origWl & mask) - 1);
		}
		case (FILT_NORMAL_OPERATIONS + 3): // cyclic red shift
		{
			int x = my_ctz (filtWl);
			return ((origWl << x) | (origWl >> (30 - x))) & mask;
		}
		case (FILT_NORMAL_OPERATIONS + 4): // cyclic blue shift
		{
			int x = my_ctz (filtWl);
			return ((origWl >> x) | (origWl << (30 - x))) & mask;
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
