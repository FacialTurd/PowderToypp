#include "simulation/Elements.h"

//#TPT-Directive ElementClass Element_TRON PT_TRON 143
Element_TRON::Element_TRON()
{
	Identifier = "DEFAULT_PT_TRON";
	Name = "TRON";
	Colour = PIXPACK(0xA9FF00);
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f  * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 100;

	DefaultProperties.temp = 0.0f;
	HeatConduct = 40;
	Description = "Smart particles, Travels in straight lines and avoids obstacles. Grows with time.";

	Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_LIFE_KILL;
	Properties2 = PROP_DEBUG_USE_TMP2;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_TRON::update;
	Graphics = &Element_TRON::graphics;

	Element_TRON::init_graphics();
}

/* TRON element is meant to resemble a tron bike (or worm) moving around and trying to avoid obstacles itself.
 * It has four direction each turn to choose from, 0 (left) 1 (up) 2 (right) 3 (down).
 * Each turn has a small random chance to randomly turn one way (so it doesn't do the exact same thing in a large room)
 * If the place it wants to move isn't a barrier, it will try and 'see' in front of itself to determine its safety.
 * For now the tron can only see its own body length in pixels ahead of itself (and around corners)
 *  - - - - - - - - - -
 *  - - - - + - - - - -
 *  - - - + + + - - - -
 *  - - +<--+-->+ - - -
 *  - +<----+---->+ - -
 *  - - - - H - - - - -
 * Where H is the head with tail length 4, it checks the + area to see if it can hit any of the edges, then it is called safe, or picks the biggest area if none safe.
 * .tmp bit values: 1st head, 2nd no tail growth, 3rd wait flag, 4th Nodie, 5th Dying, 6th & 7th is direction, 8th - 16th hue, 17th Norandom, 18th inside gate, 19th splitting flag
 * .tmp2 is tail length (gets longer every few hundred frames)
 * .life is the timer that kills the end of the tail (the head uses life for how often it grows longer)
 * .ctype Contains the colour, lost on save, regenerated using hue tmp (bits 7 - 16)
 *
 * For tron gate element, .tmp bit values: 1st & 2nd is direction, 3rd splitting flag, 4th crossing flag
 */

#define TRON_HEAD 1
#define TRON_NOGROW 2
#define TRON_WAIT 4 //it was just created, so WAIT a frame
#define TRON_NODIE 8
#define TRON_DEATH 16 //Crashed, now dying
#define TRON_NORANDOM 65536
#define TRON_INGATE		(1<<17)
#define TRON_SPLITTING	(1<<18)

int tron_rx[4] = {-1, 0, 1, 0};
int tron_ry[4] = { 0,-1, 0, 1};
unsigned int tron_colours[32];

//#TPT-Directive ElementHeader Element_TRON static void init_graphics()
void Element_TRON::init_graphics()
{
	int i;
	int r, g, b;
	for (i=0; i<32; i++)
	{
		HSV_to_RGB(i<<4,255,255,&r,&g,&b);
		tron_colours[i] = r<<16 | g<<8 | b;
	}
}

//#TPT-Directive ElementHeader Element_TRON static int update(UPDATE_FUNC_ARGS)
int Element_TRON::update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp&TRON_WAIT)
	{
		parts[i].tmp &= ~TRON_WAIT;
		return 0;
	}
	if (parts[i].tmp&TRON_HEAD)
	{
		int firstdircheck = 0, seconddircheck = 0, lastdircheck = 0,
			firstTRONInput_, seconddir, lastdir;

		bool iscrossing = false;

		int direction = (parts[i].tmp>>5 & 0x3);
		int originaldir = direction;

		//random turn
		int random = rand()%340;

		// approximately 97 in 32768 turn left,
		// approximately 97 in 32768 turn right.
		if ((random==1 || random==3) && !(parts[i].tmp & TRON_NORANDOM))
		{
			//randomly turn left(3) or right(1)
			direction = (direction + random)%4;
		}

		// checking inside gate
		firstTRONInput_ = Element_TRON::checkTRONInput_(sim, i, x, y, originaldir);

		if ((parts[i].tmp & TRON_INGATE) || firstTRONInput_ > 0)
		{
			direction = originaldir;
			if (firstTRONInput_ == 2)
				iscrossing = true;
			goto TRONInput_checked;
		}
		else
		{
			int direction_l = (originaldir + ((rand()%2)*2)+1) % 4;
			int direction_l_inp = Element_TRON::checkTRONInput_(sim, i, x, y, direction_l);
			int direction_r = direction_l ^ 2; // XOR proof
			int direction_r_inp = Element_TRON::checkTRONInput_(sim, i, x, y, direction_r);

			if (direction_l_inp > 0)
			{
				direction = direction_l;
				goto TRONInput_checked;
			}
			else if (direction_r_inp > 0)
			{
				direction = direction_r;
				goto TRONInput_checked;
			}
			
			if (direction_l_inp < 0)
				direction = direction_r;
			else if (direction_r_inp < 0)
				direction = direction_l;
		}

		//check in front
		//do sight check

		firstdircheck = Element_TRON::trymovetron(sim,x,y,direction,i,parts[i].tmp2);
		if (firstdircheck < parts[i].tmp2)
		{
			if (parts[i].tmp & TRON_NORANDOM)
			{
				seconddir = (direction + 1)%4;
				lastdir = (direction + 3)%4;
			}
			else if (originaldir != direction) //if we just tried a random turn, don't pick random again
			{
				seconddir = originaldir;
				lastdir = (direction + 2)%4;
			}
			else
			{
				seconddir = (direction + ((rand()%2)*2)+1)% 4;
				lastdir = (seconddir + 2)%4;
			}
			seconddircheck = trymovetron(sim,x,y,seconddir,i,parts[i].tmp2);
			lastdircheck = trymovetron(sim,x,y,lastdir,i,parts[i].tmp2);
		}
		//find the best move
		if (seconddircheck > firstdircheck)
			direction = seconddir;
		if (lastdircheck > seconddircheck && lastdircheck > firstdircheck)
			direction = lastdir;

		TRONInput_checked:
		{
			bool crashed = false;
			int nx, ny, np, nd = (parts[i].tmp & TRON_SPLITTING) ? 6 : 0, nm = iscrossing ? 2 : 1;

			if (!iscrossing)
			{
				do
				{
					nd >>= 1;
					int dir2 = (direction + nd) % 4;
					//now try making new head, even if it fails
					nx = x + tron_rx[dir2], ny = y + tron_ry[dir2];
					np = Element_TRON::new_tronhead(sim, nx, ny, i, dir2);
					if (np == -1)
						crashed = true;
				}
				while (nd);
			}
			else
			{
				nx = x + 2 * tron_rx[direction], ny = y + 2 * tron_ry[direction];
				if (nx >= 0 && ny >= 0 && nx < XRES && ny < YRES)
				{
					int r = pmap[ny][nx];
					if (!r || CHECK_EXTEL(r, 2))
						Element_TRON::new_tronhead(sim, nx, ny, i, direction);
				}
			}

			if (parts[i].tmp & TRON_INGATE)
			{
				sim->part_change_type(i, x, y, ELEM_MULTIPP);
				parts[i].life = 2;
				parts[i].tmp = originaldir | ((parts[i].tmp >> (18 - 2)) & 4);
				
				if (crashed)
					sim->create_part(-1, nx, ny, PT_SPRK);
			}
			else
			{
				if (crashed)
				{
					//ohgod crash
					parts[i].tmp |= TRON_DEATH;
					//trigger tail death for TRON_NODIE, or is that mode even needed? just set a high tail length(but it still won't start dying when it crashes)
				}

				//set own life and clear .tmp (it dies if it can't move anyway)
				parts[i].life = parts[i].tmp2;
				parts[i].tmp &= parts[i].tmp&0xF818;
			}
		}
	}
	else // fade tail deco, or prevent tail from dying
	{
		if (parts[i].tmp&TRON_NODIE)
			parts[i].life++;
		//parts[i].dcolour =  clamp_flt((float)parts[i].life/(float)parts[i].tmp2,0,1.0f) << 24 |  parts[i].dcolour&0x00FFFFFF;
	}
	return 0;
}



//#TPT-Directive ElementHeader Element_TRON static int graphics(GRAPHICS_FUNC_ARGS)
int Element_TRON::graphics(GRAPHICS_FUNC_ARGS)
{
	unsigned int col = tron_colours[(cpart->tmp&0xF800)>>11];
	if(cpart->tmp & TRON_HEAD)
		*pixel_mode |= PMODE_GLOW;
	*colr = (col & 0xFF0000)>>16;
	*colg = (col & 0x00FF00)>>8;
	*colb = (col & 0x0000FF);
	if(cpart->tmp & TRON_DEATH)
	{
		*pixel_mode |= FIRE_ADD | PMODE_FLARE;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*firea = 255;
	}
	if(cpart->life < cpart->tmp2 && !(cpart->tmp & TRON_HEAD))
	{
		*pixel_mode |= PMODE_BLEND;
		*pixel_mode &= ~PMODE_FLAT;
		*cola = (int)((((float)cpart->life)/((float)cpart->tmp2))*255.0f);
	}
	return 0;
}

#define FUNC_TRON_GATE		2
#define FUNC_TRON_FILTER	30
#define FUNC_TRON_DELAY		31

//#TPT-Directive ElementHeader Element_TRON static int convertToAbsDirection(int flags)
int Element_TRON::convertToAbsDirection(int flags)
{
	int base = (flags >> 5);
	if (flags & (1 << 19))
		base = 0;
	return (base + (flags >> 17)) & 3;
}

//#TPT-Directive ElementHeader Element_TRON static int new_tronhead(Simulation * sim, int x, int y, int i, int direction)
int Element_TRON::new_tronhead(Simulation * sim, int x, int y, int i, int direction)
{
	if (x < 0 || y < 0 || x >= XRES || y >= YRES)
		return -1;
		
	int r = sim->pmap[y][x], ni = -1, extraFlags = 0, funcCode, z;
	int tmp = sim->parts[i].tmp;
	int type = sim->parts[i].type;

	if (TYP(r) == ELEM_MULTIPP)
	{
		int ri = ID(r), tmp;
		funcCode = sim->parts[ri].life;
		switch (funcCode)
		{
		case FUNC_TRON_GATE:
			z = sim->parts[ri].tmp;
			extraFlags |= TRON_INGATE | ((z & 4) << (18 - 2));
			direction = z & 3;
			break;
		case FUNC_TRON_DELAY:
			if (sim->parts[ri].tmp3)
				return -1;
			sim->parts[ri].tmp3 = sim->parts[ri].ctype;
		case FUNC_TRON_FILTER:
			sim->parts[ri].tmp &= (funcCode == FUNC_TRON_FILTER) ? 0x7E0000 : 0xE0000;
			sim->parts[ri].tmp |= (tmp & 0x1FF9F) | (direction << 5) | ((ri > i) ? TRON_WAIT : 0);
			sim->parts[ri].tmp2 = sim->parts[i].tmp2;
			return -1;
		}
	}
	else if (TYP(r) == PT_TRON && (sim->partsi(r).tmp & TRON_INGATE))
	{
		z = sim->partsi(r).tmp;
		extraFlags |= TRON_INGATE | (z & TRON_SPLITTING);
		direction = (z >> 5) & 3;
	}
	
	if (extraFlags)
		ni = ID(r);

	int np = sim->create_part(ni, x, y, PT_TRON);
	if (np==-1)
		return -1;

	if (type != PT_TRON || sim->parts[i].life >= 100) // increase tail length
	{
		if (type != PT_TRON || !(tmp & TRON_NOGROW))
			sim->parts[i].tmp2++;
		sim->parts[i].life = 5;
	}

	//give new head our properties
	sim->parts[np].tmp = 1 | direction<<5 | (tmp & (TRON_NOGROW|TRON_NODIE|TRON_NORANDOM)) | (tmp & 0xF800) | extraFlags;
	if (np > i)
		sim->parts[np].tmp |= TRON_WAIT;

	sim->parts[np].ctype = sim->parts[i].ctype;
	sim->parts[np].tmp2 = sim->parts[i].tmp2;
	sim->parts[np].life = sim->parts[i].life + 2;
	return 1;
}

//#TPT-Directive ElementHeader Element_TRON static int trymovetron(Simulation * sim, int x, int y, int dir, int i, int len)
int Element_TRON::trymovetron(Simulation * sim, int x, int y, int dir, int i, int len)
{
	int k,j,r,rx,ry,tx,ty,count;
	count = 0;
	rx = x;
	ry = y;
	for (k = 1; k <= len; k ++)
	{
		rx += tron_rx[dir];
		ry += tron_ry[dir];
		r = sim->pmap[ry][rx];
		if (canmovetron(sim, r, k-1) && !sim->bmap[(ry)/CELL][(rx)/CELL] && ry > CELL && rx > CELL && ry < YRES-CELL && rx < XRES-CELL)
		{
			count++;
			for (tx = rx - tron_ry[dir] , ty = ry - tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx-=tron_ry[dir],ty-=tron_rx[dir],j++)
			{
				r = sim->pmap[ty][tx];
				if (canmovetron(sim, r, j+k-1) && !sim->bmap[(ty)/CELL][(tx)/CELL] && ty > CELL && tx > CELL && ty < YRES-CELL && tx < XRES-CELL)
				{
					if (j == (len-k))//there is a safe path, so we can break out
						return len+1;
					count++;
				}
				else //we hit a block so no need to check farther here
					break;
			}
			for (tx = rx + tron_ry[dir] , ty = ry + tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx+=tron_ry[dir],ty+=tron_rx[dir],j++)
			{
				r = sim->pmap[ty][tx];
				if (canmovetron(sim, r, j+k-1) && !sim->bmap[(ty)/CELL][(tx)/CELL] && ty > CELL && tx > CELL && ty < YRES-CELL && tx < XRES-CELL)
				{
					if (j == (len-k))
						return len+1;
					count++;
				}
				else
					break;
			}
		}
		else //a block infront, no need to continue
			break;
	}
	return count;
}

//#TPT-Directive ElementHeader Element_TRON static bool canmovetron(Simulation * sim, int r, int len)
bool Element_TRON::canmovetron(Simulation * sim, int r, int len)
{
	int prop, l;
	if (TYP(r) == PT_PINVIS && sim->partsi(r).life >= 10)
		r = sim->partsi(r).tmp4;
	l = sim->partsi(r).life;
	if (!r || ( TYP(r) == PT_SWCH && l >= 10) || (TYP(r) == PT_INVIS && sim->partsi(r).tmp2 == 1))
		return true;
	prop = sim->elements[TYP(r)].Properties;
	if ((((prop & PROP_LIFE_KILL_DEC) && l > 0) || ((prop & PROP_LIFE_KILL) && (prop & PROP_LIFE_DEC))) && l < len)
		return true;
	return false;
}

//#TPT-Directive ElementHeader Element_TRON static int checkTRONInput_(Simulation * sim, int i, int x, int y, int dir)
int Element_TRON::checkTRONInput_(Simulation * sim, int i, int x, int y, int dir)
{
	int rx = x + tron_rx[dir];
	int ry = y + tron_ry[dir];
	int r = sim->pmap[ry][rx], d, l, tmp;

	if (TYP(r) == ELEM_MULTIPP)
	{
		l = sim->partsi(r).life;
		if (l == FUNC_TRON_GATE)
		{
			tmp = sim->partsi(r).tmp;
			return (tmp & 0x8) == 8 ? 2 : (dir ^ 2) == (tmp & 3) ? -1 : 1;
		}
		else if (l == FUNC_TRON_DELAY || l == FUNC_TRON_FILTER)
		{
			return (dir ^ 2) == convertToAbsDirection((sim->partsi(r).tmp & ~0x60) | (dir << 5)) ? -1 : 1;
		}
	}
	else if (TYP(r) == PT_TRON && (sim->partsi(r).tmp & TRON_INGATE))
		return ((dir ^ 2) == (sim->partsi(r).tmp >> 5) & 3) ? -1 : 1;
	return 0;
}

Element_TRON::~Element_TRON() {}
