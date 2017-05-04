#include "simulation/Elements.h"
#include "simulation/E189_update.h"
#include "Probability.h"

//#TPT-Directive ElementClass Element_E189 PT_E189 189
Element_E189::Element_E189()
{
	Identifier = "DEFAULT_PT_E189";
	Name = "E189";
	Colour = PIXPACK(0xFFB060);
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
#if defined(DEBUG) || defined(SNAPSHOT)
	Enabled = 1;
#else
	Enabled = 0;
#endif

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

	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 0;
	Description = "Experimental element. has multi-purpose.";

	Properties = TYPE_SOLID | PROP_NOSLOWDOWN | PROP_TRANSPARENT;
	Properties2 = PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &E189_Update::update;
	Graphics = &E189_Update::graphics;
	IconGenerator = &Element_E189::iconGen;
	// Notice: Exotic solid!
	// Properties without PROP_LIFE_DEC and PROP_LIFE_KILL_DEC, has reason.
}

//#TPT-Directive ElementHeader Element_E189 static bool useDefaultPart
bool Element_E189::useDefaultPart = false;

//#TPT-Directive ElementHeader Element_E189 static void HSV2RGB(int ctype, int *r, int *g, int *b)
void Element_E189::HSV2RGB (int ctype, int *r, int *g, int *b)
{
	int ptmp = ctype;
	float tmpr, tmpg, tmpb;
	float hh, ss, vv, cc;
	int phue = (ptmp >> 16) % 0x600;
	if (phue < 0)
		phue += 0x600;
	hh = (float)phue / 256.0f;
	ss = (float)((ptmp >> 8) & 0xFF) / 255.0f;
	vv = (float)(ptmp & 0xFF);
	cc = vv * ss;
	int p_add = (int)(vv - cc);
	switch (phue >> 8)
	{
	case 0:
		tmpr = cc;
		tmpg = cc * hh;
		tmpb = 0.0f;
		break;
	case 1:
		tmpr = cc * (2.0f - hh);
		tmpg = cc;
		tmpb = 0.0f;
		break;
	case 2:
		tmpr = 0.0f;
		tmpg = cc;
		tmpb = cc * (hh - 2.0f);
		break;
	case 3:
		tmpr = 0.0f;
		tmpg = cc * (4.0f - hh);
		tmpb = cc;
		break;
	case 4:
		tmpr = cc * (hh - 4.0f);
		tmpg = 0.0f;
		tmpb = cc;
		break;
	case 5:
		tmpr = cc;
		tmpg = 0.0f;
		tmpb = cc * (6.0f - hh);
		break;
	}
	*r = (int)tmpr + p_add;
	*g = (int)tmpg + p_add;
	*b = (int)tmpb + p_add;
}


//#TPT-Directive ElementHeader Element_E189 static VideoBuffer * iconGen(int, int, int)
VideoBuffer * Element_E189::iconGen(int toolID, int width, int height)
{
	VideoBuffer * newTexture = new VideoBuffer(width, height);
	
	for (int j = 0; j < height; j++)
	{
		int r = 100, g = 150, b = 50;
		int rd = 1, gd = -1, bd = -1;
		for (int i = 0; i < width; i++)
		{
			r += 15*rd;
			g += 15*gd;
			b += 15*bd;
			if (r > 200) rd = -1;
			if (g > 200) gd = -1;
			if (b > 200) bd = -1;
			if (r < 15) rd = 1;
			if (g < 15) gd = 1;
			if (b < 15) bd = 1;
			int rc = std::min(150, std::max(0, r));
			int gc = std::min(200, std::max(0, g));
			int bc = std::min(200, std::max(0, b));
			newTexture->SetPixel(i, j, rc, gc, bc, 255);
		}
	}
	
	return newTexture;
}

//#TPT-Directive ElementHeader Element_E189 static void interactDir(Simulation* sim, int i, int x, int y, Particle* part_phot, Particle* part_E189)
void Element_E189::interactDir(Simulation* sim, int i, int x, int y, Particle* part_phot, Particle* part_E189) // photons direction/type changer
{
	int rtmp = part_E189->tmp, rtmp2 = part_E189->tmp2, rct = part_E189->ctype, mask = 0x3FFFFFFF;
	int ctype, r1, r2, r3, temp;
	float rvx, rvy, rvx2, rvy2, rdif, multipler = 1.0f;
	long long int lsb;
	if (rtmp)
	{
		rvx = (float)rtmp2 / 1000.0f;
		rvy = (float)part_E189->tmp3 / 1000.0f;
		switch (rtmp)
		{
		case 1:
			part_phot->vx = rvx;
			part_phot->vy = rvy;
			break;
		case 2:
			part_phot->vx += rvx;
			part_phot->vy += rvy;
			break;
		case 3:
			rvx2 = part_phot->vx;
			rvy2 = part_phot->vy;
			part_phot->vx = rvx2 * rvx - rvy2 * rvy;
			part_phot->vy = rvx2 * rvy + rvy2 * rvx;
			break;
		case 4:
			rvx2 = rvx * 0.0174532925f;
			rdif = hypotf(part_phot->vx, part_phot->vy);
			if (rtmp & 0x100)
			{
				rvy2 = atan2f(part_phot->vy, part_phot->vx);
				rvx2 = rvx2 - rvy2;
			}
			part_phot->vx = rdif * cosf(rvx2);
			part_phot->vy = rdif * sinf(rvx2);
			break;
		}
	}
	else
	{
		switch (rtmp2)
		{
			case 0: // no photons operation
				break;
			case 1: // 50% turn left
				if (rand() & 1)
				{
					rdif = part_phot->vx;
					part_phot->vx = part_phot->vy;
					part_phot->vy = -rdif;
				}
				break;
			case 2: // 50% turn right
				if (rand() & 1)
				{
					rdif = part_phot->vx;
					part_phot->vx = -part_phot->vy;
					part_phot->vy = rdif;
				}
				break;
			case 3: // 50% turn left, 50% turn right
				rvx = part_phot->vx;
				rvy = (rand() & 1) ? 1.0 : -1.0;
				part_phot->vx =  rvy * part_phot->vy;
				part_phot->vy = -rvy * rdif;
				break;
			case 4: // turn left + go straight + turn right = 100%
				r1 = rand() % 3;
				if (r1)
				{
					rvx = part_phot->vx;
					rvy = (r1 & 1) ? 1.0 : -1.0;
					part_phot->vx =  rvy * part_phot->vy;
					part_phot->vy = -rvy * rdif;
				}
				break;
			case 5: // random "energy" particle
				part_phot->ctype = 0x101;
				sim->part_change_type(i, x, y, PT_E186);
				break;
			case 6: // photons absorber
				sim->kill_part(i);
				break;
			case 7: // PHOT->NEUT
				part_phot->ctype = 0x102;
				sim->part_change_type(i, x, y, PT_E186);
				break;
			case 8: // PHOT->ELEC
				part_phot->ctype = 0x103;
				sim->part_change_type(i, x, y, PT_E186);
				break;
			case 9: // PHOT->PROT
				part_phot->ctype = 0x104;
				sim->part_change_type(i, x, y, PT_E186);
				break;
			case 10: // PHOT->GRVT
				part_phot->ctype = 0x105;
				sim->part_change_type(i, x, y, PT_E186);
				break;
			case 11: // PHOT (tmp: 0 -> 1)
				part_phot->tmp |= 0x1;
				break;
			case 12: // PHOT (tmp: 1 -> 0)
				part_phot->tmp &= ~0x1;
				break;
			case 13: // set PHOT life
				part_phot->life = part_E189->ctype;
				break;
			case 14: // PHOT life extender (positive)
				if (part_phot->life > 0)
				{
					part_phot->life += part_E189->ctype;
					if (part_phot->life < 0)
						part_phot->life = 0;
				}
				break;
			case 15: // PHOT life extender (negative)
				if (part_phot->life > 0)
				{
					part_phot->life -= part_E189->ctype;
					if (part_phot->life <= 0)
						sim->kill_part(i);
				}
				break;
			case 16: // velocity to wavelength converter
				rvx = part_phot->vx;
				rvy = part_phot->vy;
				rdif = rvx * rvx + rvy * rvy;
				ctype = part_phot->ctype;
				// int's length is 32-bit
				// float's length is 32-bit
				r1 = * (int*) &rdif; // r1 = "bit level hack"
				r2 = ((r1 - 0x40cba592) >> 23); // 0x40cba592: for (9 / sqrt(2))
				if (r2 > 0)
				{
					// blue shift and decelerate
#ifdef __GNUC__
					r1 = (31 - __builtin_clz (ctype)) / 3;
					r3 = (r1 < r2 ? r1 : r2);
					ctype >>= 3 * r3;
#else
					r3 = 0;
					while (r2 && (ctype & 0xFFFFFFF8))
					{
						ctype >>= 3;
						r2--; r3++;
					}
#endif
					multipler = powf(0.5f, r3 * 0.5f);
					part_phot->vx = rvx * multipler;
					part_phot->vy = rvy * multipler;
				}
				else if (r2 < 0)
				{
					// red shift and accelerate
#ifdef __GNUC__
					r1 = (29 - __builtin_ctz (ctype)) / 3;
					r3 = (r1 < -r2 ? r1 : -r2);
					ctype <<= 3 * r3;
#else
					r3 = 0;
					while (r2 && (ctype & 0x07FFFFFF))
					{
						ctype <<= 3;
						r2++; r3++;
					}
#endif
					multipler = powf(2.0f, r3 * 0.5f);
					ctype &= mask;
					part_phot->vx = rvx * multipler;
					part_phot->vy = rvy * multipler;
				}
				part_phot->ctype = ctype;
				break;
			case 17: // PHOT life multipler
				if (part_phot->life > 0)
				{
					part_phot->life *= part_E189->ctype;
					if (part_phot->life < 0)
						part_phot->life = 0;
				}
				break;
		}
	}
}

//#TPT-Directive ElementHeader Element_E189 static void duplicatePhotons(Simulation* sim, int i, int x, int y, Particle* part_phot, Particle* part_E189)
void Element_E189::duplicatePhotons(Simulation* sim, int i, int x, int y, Particle* part_phot, Particle* part_E189)
{
	int rtmp = part_E189->tmp, ri;
	if (!rtmp)
		return;
	float rvx = (float)(((rtmp ^ 0x08) & 0x0F) - 0x08);
	float rvy = (float)((((rtmp >> 4) ^ 0x08) & 0x0F) - 0x08);
	float rdif = (float)((((rtmp >> 8) ^ 0x80) & 0xFF) - 0x80);
	
	ri = sim->create_part(-3, x, y, PT_PHOT);
	if (ri < 0)
		return;
	if (ri > i)
		sim->parts[ri].flags |= FLAG_SKIPMOVE;
	sim->parts[ri].vx = rvx * rdif / 16.0f;
	sim->parts[ri].vy = rvy * rdif / 16.0f;
	sim->parts[ri].temp = part_phot->temp;
	sim->parts[ri].tmp  = part_phot->tmp;
	sim->parts[ri].life = part_E189->tmp2;
	if (part_E189->ctype)
		sim->parts[ri].ctype = part_E189->ctype;
	else
		sim->parts[ri].ctype = part_phot->ctype;
}

//#TPT-Directive ElementHeader Element_E189 static int EMPTrigger(Simulation *sim, int triggerCount)
int Element_E189::EMPTrigger(Simulation *sim, int triggerCount)
{
	int t, ct, rx, ry, r1;
	Particle *parts = sim->parts;
	
	float prob_breakPInsulator = Probability::binomial_gte1(triggerCount, 1.0f/200);
	float prob_breakTRONPortal = Probability::binomial_gte1(triggerCount, 1.0f/160);
	float prob_randLaser = Probability::binomial_gte1(triggerCount, 1.0f/40);
	float prob_breakLaser = Probability::binomial_gte1(triggerCount, 1.0f/120);
	float prob_breakDChanger = Probability::binomial_gte1(triggerCount, 1.0f/160);
	float prob_breakHeater = Probability::binomial_gte1(triggerCount, 1.0f/100);
	float prob_breakElectronics = Probability::binomial_gte1(triggerCount, 1.0f/300);

	for (int r = 0; r <=sim->parts_lastActiveIndex; r++)
	{
		t = parts[r].type;
		if (sim->elements[t].Properties2 & PROP_NODESTRUCT)
			continue; // particle's type is PT_DMND and PT_INDI are indestructible.
		rx = parts[r].x;
		ry = parts[r].y;

		switch ( t )
		{
		case PT_DMND:
		case PT_INDI:
			break;
		case PT_METL:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BMTL);
			break;
		case PT_COAL:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BCOL);
			break;
		case PT_BMTL:
		case PT_PIPE:
		case PT_PPIP:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BRMT);
			break;
		case PT_GLAS:
		case PT_LCRY:
		case PT_FILT:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BGLA);
			break;
		case PT_QRTZ:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_PQRT);
			break;
		case PT_TTAN:
		case PT_GOLD:
			if (Probability::randFloat() < prob_breakElectronics)
			{
				sim->create_part(r, rx, ry, PT_E189, 8);
				parts[r].tmp = 21000;
			}
			break;
		case PT_VOID:
		case PT_PVOD:
		case PT_CONV:
			if (Probability::randFloat() < prob_breakElectronics)
			{
				sim->create_part(r, rx, ry, PT_WARP);
				parts[r].tmp2 = 6000;
			}
			break;
		case PT_CRMC:
			if (Probability::randFloat() < prob_breakElectronics)
			{
				if (rand() & 1)
					sim->part_change_type(r, rx, ry, PT_CLST);
				else
					sim->part_change_type(r, rx, ry, PT_PQRT);
			}
			break;
		case PT_BRCK:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_STNE);
			break;
		case PT_DLAY:
			if (Probability::randFloat() < prob_breakElectronics)
				parts[r].temp = (rand()%512) + 274.15f; // Randomize delay
			break;
		case PT_EMP:
			if (triggerCount > rand() % 1000)
				sim->part_change_type(r, rx, ry, PT_BREC);
			break;
		case PT_PSCN: case PT_NSCN:
		case PT_PTCT: case PT_NTCT:
		case PT_SWCH: case PT_DTEC:
		case PT_PSNS: case PT_TSNS: case PT_LSNS:
		case PT_FRME: case PT_PSTN:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BREC);
			break;
		case PT_WIFI:
		case PT_PRTI: case PT_PRTO:
			if (Probability::randFloat() < prob_breakElectronics)
			{
				// Randomize channel
				parts[r].temp = rand()%MAX_TEMP;
			}
			else if (prob_breakTRONPortal)
			{
				sim->part_change_type(r, rx, ry, PT_WARP);
			}
			break;
		case PT_SPRK:
			if (!(sim->elements[parts[r].ctype].Properties2 & PROP_NODESTRUCT) &&
			    (Probability::randFloat() < prob_breakElectronics))
				sim->part_change_type(r, rx, ry, PT_BREC);
			break;
		case PT_CLNE:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_BCLN);
			break;
		case PT_PCLN:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_PBCN);
			break;
		case PT_BCLN:
			if (triggerCount > rand() % 1000)
				parts[r].life = rand()%40+80;
			break;
		case PT_PBCN:
			if (triggerCount > rand() % 1000)
				parts[r].tmp2 = rand()%40+80;
			break;
		case PT_SPNG:
		case PT_BTRY:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->create_part(r, rx, ry, PT_PLSM);
			break;
		case PT_VIBR:
			if (Probability::randFloat() < prob_breakElectronics * 0.2)
				sim->part_change_type(r, rx, ry, PT_BVBR);
			break;
		case PT_BVBR:
			if (Probability::randFloat() < prob_breakElectronics * 0.1)
			{
				sim->part_change_type(r, rx, ry, PT_VIBR);
				parts[r].life = 1000;
			}
			break;
		case PT_URAN:
			if (Probability::randFloat() < prob_breakElectronics)
				sim->part_change_type(r, rx, ry, PT_PLUT);
			break;
		case PT_E189:
			switch (parts[r].life)
			{
			case 0:
			case 1:
				if (Probability::randFloat() < prob_breakPInsulator)
				{
					parts[r].life = 8;
					parts[r].tmp = 21000;
				}
				break;
			case 12:
			case 16:
				if (Probability::randFloat() < prob_breakElectronics)
				{
					sim->part_change_type(r, rx, ry, PT_BREC);
				}
				break;
			case 2:
			case 3:
				if (Probability::randFloat() < prob_breakTRONPortal)
				{
					sim->create_part(r, rx, ry, PT_PLSM);
				}
				break;
			case 4:
			case 7:
			case 11:
				if (Probability::randFloat() < prob_randLaser)
				{
					parts[r].ctype += (rand() << 15) + rand();
					parts[r].tmp = (parts[r].tmp + rand()) & 0x0000FFFF;
				}
				if (Probability::randFloat() < prob_breakLaser)
				{
					sim->create_part(r, rx, ry, PT_BRMT);
				}
				break;
			case 5:
				if (Probability::randFloat() < prob_breakDChanger)
				{
					sim->create_part(r, rx, ry, PT_BGLA);
				}
				break;
			case 6:
				if (Probability::randFloat() < prob_breakHeater)
				{
					sim->create_part(r, rx, ry, PT_PLSM);
				}
			case 33:
				if (Probability::randFloat() < prob_breakElectronics)
				{
					sim->part_change_type(r, rx, ry, PT_WIFI);
					parts[r].ctype = 0;
				}
			}
			break;
		}
	}
}

//#TPT-Directive ElementHeader Element_E189 static void FloodButton(Simulation *sim, int i, int x, int y)
void Element_E189::FloodButton(Simulation *sim, int i, int x, int y)
{
	int coord_stack_limit = XRES*YRES;
	unsigned short (*coord_stack)[2];
	int coord_stack_size = 0;
	int x1, x2, r;
	
	Particle * parts = sim->parts;
	int (*pmap)[XRES] = sim->pmap;
	
	coord_stack = new unsigned short[coord_stack_limit][2];
	coord_stack[coord_stack_size][0] = x;
	coord_stack[coord_stack_size][1] = y;
	coord_stack_size++;
	
	if ((parts[i].type != PT_E189) || (parts[i].life != 26) || parts[i].tmp)
	{
		delete[] coord_stack;
		return;
	}
	
	do
	{
		coord_stack_size--;
		x = coord_stack[coord_stack_size][0];
		y = coord_stack[coord_stack_size][1];
		x1 = x2 = x;
		
		// go left as far as possible
		while (x1 >= 0)
		{
			r = pmap[y][x1-1];
			if ((r&0xFF) != PT_E189 || parts[r>>8].life != 26)
			{
				break;
			}
			x1--;
		}
		// go right as far as possible
		while (x2 < XRES)
		{
			r = pmap[y][x2+1];
			if ((r&0xFF) != PT_E189 || parts[r>>8].life != 26)
			{
				break;
			}
			x2++;
		}
		
		// fill span
		for (x=x1; x<=x2; x++)
		{
			r = pmap[y][x]>>8;
			parts[r].tmp = 8;
			// parts[r].flags |= 0x80000000;
		}
		
		// add adjacent pixels to stack
		if (y >= 1)
			for (x=x1-1; x<=x2+1; x++)
			{
				r = pmap[y-1][x];
				if ((r&0xFF) == PT_E189 && parts[r>>8].life == 26 && !parts[r>>8].tmp)
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y-1;
					coord_stack_size++;
					if (coord_stack_size>=coord_stack_limit)
					{
						delete[] coord_stack;
						return;
					}
				}
			}
		if (y < YRES-1)
			for (x=x1-1; x<=x2+1; x++)
			{
				r = pmap[y+1][x];
				if ((r&0xFF) == PT_E189 && parts[r>>8].life == 26 && !parts[r>>8].tmp)
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y+1;
					coord_stack_size++;
					if (coord_stack_size>=coord_stack_limit)
					{
						delete[] coord_stack;
						return;
					}
				}
			}
		
	} while (coord_stack_size>0);
	delete[] coord_stack;
}

Element_E189::~Element_E189() {}
