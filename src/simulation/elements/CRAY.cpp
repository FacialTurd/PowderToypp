#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_CRAY PT_CRAY 167
Element_CRAY::Element_CRAY()
{
	Identifier = "DEFAULT_PT_CRAY";
	Name = "CRAY";
	Colour = PIXPACK(0xBBFF00);
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

	Temperature = R_TEMP+0.0f +273.15f;
	HeatConduct = 0;
	Description = "Particle Ray Emitter. Creates a beam of particles set by its ctype, with a range set by tmp.";

	Properties = TYPE_SOLID;
	Properties2 = PROP_DRAWONCTYPE | PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_CRAY::update;
}

//#TPT-Directive ElementHeader Element_CRAY static int update(UPDATE_FUNC_ARGS)
int Element_CRAY::update(UPDATE_FUNC_ARGS)
{
	int nxx, nyy, docontinue, nxi, nyi;
	// set ctype to things that touch it if it doesn't have one already
	if (parts[i].ctype<=0 || !sim->elements[TYP(parts[i].ctype)].Enabled)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK)
				{
					int r = sim->photons[y+ry][x+rx];
					if (!r)
						r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					int rt = TYP(r);
					if (rt!=PT_CRAY && rt!=PT_PSCN && rt!=PT_INST && rt!=PT_METL && rt!=PT_SPRK && rt!=ELEM_MULTIPP && rt!=PT_INDC && rt<PT_NUM)
					{
						parts[i].ctype = TYP(r);
						parts[i].temp = partsi(r).temp;
					}
				}
	}
	else
	{
		int new_part_life = parts[i].life;
		int new_part_ctype = parts[i].ctype;
		size_t offset1 = (TYP(new_part_ctype) == ELEM_MULTIPP) ? offsetof(Particle, ctype) : offsetof(Particle, life);
		for (int rx =-1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (CHECK_EL_SPRKL3(r)) { //spark found, start creating
						unsigned int colored = 0;
						bool destroy = partsi(r).ctype==PT_PSCN;
						bool nostop = partsi(r).ctype==PT_INST;
						bool createSpark = (partsi(r).ctype==PT_INWR);
						int partsRemaining = 255;
						if (parts[i].tmp) //how far it shoots
							partsRemaining = parts[i].tmp;
						int spacesRemaining = parts[i].tmp2;
						for (docontinue = 1, nxi = rx*-1, nyi = ry*-1, nxx = spacesRemaining*nxi, nyy = spacesRemaining*nyi; docontinue; nyy+=nyi, nxx+=nxi)
						{
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
								break;
							}
							r = pmap[y+nyi+nyy][x+nxi+nxx];
							if (!sim->IsWallBlocking(x+nxi+nxx, y+nyi+nyy, TYP(new_part_ctype)) && (!sim->pmap[y+nyi+nyy][x+nxi+nxx] || createSpark)) { // create, also set color if it has passed through FILT
								int nr = sim->create_part(-1, x+nxi+nxx, y+nyi+nyy, TYP(new_part_ctype), ID(new_part_ctype));
								if (nr!=-1) {
									if (colored)
										parts[nr].dcolour = colored;
									parts[nr].temp = parts[i].temp;
									if (new_part_life >= 0)
										*((int*)(((char*)&parts[nr]) + offset1)) = new_part_life;
									if(!--partsRemaining)
										docontinue = 0;
								}
							} else if (TYP(r)==PT_FILT) { // get color if passed through FILT
								if (partsi(r).dcolour == 0xFF000000)
									colored = 0xFF000000;
								else if (partsi(r).tmp==0)
								{
									colored = wavelengthToDecoColour(Element_FILT::getWavelengths(&partsi(r)));
								}
								else if (colored==0xFF000000)
									colored = 0;
								partsi(r).life = 4;
							} else if (TYP(r) == PT_CRAY || nostop) {
								docontinue = 1;
							} else if(destroy && r && !(sim->elements[TYP(r)].Properties2 & PROP_NODESTRUCT)) {
								sim->kill_part(ID(r));
								if(!--partsRemaining)
									docontinue = 0;
							}
							else
								docontinue = 0;
							if(!partsRemaining)
								docontinue = 0;
						}
					}
				}
	}
	return 0;
}
//#TPT-Directive ElementHeader Element_CRAY static unsigned int wavelengthToDecoColour(int wavelength)
unsigned int Element_CRAY::wavelengthToDecoColour(int wavelength)
{
	int colr = 0, colg = 0, colb = 0, x;
	for (x=0; x<12; x++) {
		colr += (wavelength >> (x+18)) & 1;
		colb += (wavelength >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		colg += (wavelength >> (x+9))  & 1;
	x = 624/(colr+colg+colb+1);
	colr *= x;
	colg *= x;
	colb *= x;

	if(colr > 255) colr = 255;
	else if(colr < 0) colr = 0;
	if(colg > 255) colg = 255;
	else if(colg < 0) colg = 0;
	if(colb > 255) colb = 255;
	else if(colb < 0) colb = 0;

	return (255<<24) | (colr<<16) | (colg<<8) | colb;
}


Element_CRAY::~Element_CRAY() {}
