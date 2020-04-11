#include "simulation/Elements.h"
#include <climits>
//#TPT-Directive ElementClass Element_STKM PT_STKM 55
Element_STKM::Element_STKM()
{
	Identifier = "DEFAULT_PT_STKM";
	Name = "STKM";
	Colour = PIXPACK(0xFFE0A0);
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.2f;
	Loss = 1.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	NewtonianGravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.00f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 50;

	DefaultProperties.temp = R_TEMP+14.6f+273.15f;
	HeatConduct = 0;
	Description = "Stickman. Don't kill him! Control with the arrow keys.";

	Properties = PROP_NOCTYPEDRAW;
	Properties2 |= PROP_ALLOWS_WALL;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 620.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &Element_STKM::update;
	Graphics = &Element_STKM::graphics;
}

//#TPT-Directive ElementHeader Element_STKM static int lifeinc[5]
int Element_STKM::lifeinc[5] = {0,0,0,0,100};

//#TPT-Directive ElementHeader Element_STKM static char phase
char Element_STKM::phase = 0;

//#TPT-Directive ElementHeader Element_STKM static int update(UPDATE_FUNC_ARGS)
int Element_STKM::update(UPDATE_FUNC_ARGS)

{
	if (sim->player.__flags & _STKM_FLAG_SUSPEND)
		return 1;
	run_stickman(&sim->player, UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

#define PMAPBITSp1 (PMAPBITS + 1)

//#TPT-Directive ElementHeader Element_STKM static int graphics(GRAPHICS_FUNC_ARGS)
int Element_STKM::graphics(GRAPHICS_FUNC_ARGS)
{	
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode = PSPEC_STICKMAN;
	return 1;
}

#define INBOND(x, y) ((x)>=0 && (y)>=0 && (x)<XRES && (y)<YRES)
#define _IS_EMIT_FN(a, b)		((a & _STKM_FLAG_EPROP) == (b))

#define _EMIT_SET_A(a, b)	(a = (a & ~_STKM_FLAG_EPROP) | (b))
#define _EMIT_CLR_A(a)		(a &= ~_STKM_FLAG_EPROP)
#define _EMIT_SET_T(a, b, c)	(_IS_EMIT_FN(a, b) && _EMIT_SET_A(a, c))
#define _EMIT_CLR_T(a, b)		(_IS_EMIT_FN(a, b) && _EMIT_CLR_A(a))
#define _EMIT_SET_F(a, b, c)	(_IS_EMIT_FN(a, b) || _EMIT_SET_A(a, c))
#define _EMIT_CLR_F(a, b)		(_IS_EMIT_FN(a, b) || _EMIT_CLR_A(a))

//#TPT-Directive ElementHeader Element_STKM static int run_stickman(playerst *playerp, UPDATE_FUNC_ARGS)
int Element_STKM::run_stickman(playerst *playerp, UPDATE_FUNC_ARGS) {
	int r, rx, ry, ctype;
	int t = parts[i].type;

	float pp, d, pressure;
	float dt = 0.9;// /(FPSB*FPSB);  //Delta time in square
	float gvx, gvy;
	float gx, gy, dl, dr, dx, dy;
	float rocketBootsHeadEffect = 0.35f;
	float rocketBootsFeetEffect = 0.15f;
	float rocketBootsHeadEffectV = 0.3f;// stronger acceleration vertically, to counteract gravity
	float rocketBootsFeetEffectV = 0.45f;
	Particle *newpart = NULL;

	if (playerp->__flags & _STKM_FLAG_OVR)
		playerp->__flags &= ~_STKM_FLAG_OVR;
	else if (!playerp->fan && parts[i].ctype && sim->IsValidElement(parts[i].ctype))
		STKM_set_element(sim, playerp, parts[i].ctype);
	playerp->frames++;
		
	//Temperature handling
	if (parts[i].temp<243 && !(sim->Extra_FIGH_pause & 16))
		parts[i].life -= 1;
	if ((parts[i].temp<309.6f) && (parts[i].temp>=243))
		parts[i].temp += 1;

	//Death
	pressure = sim->pv[y/CELL][x/CELL];
	if (pressure < 0.0f && (sim->Extra_FIGH_pause & 0x100))
	{
		pressure = -pressure;
	}
	if (parts[i].life<1 || (pressure>=4.5f && !(sim->Extra_FIGH_pause & 16) && !playerp->fan) ) //If his HP is less than 0 or there is very big wind...
	{
		if (!_IS_EMIT_FN(playerp->__flags, _STKM_FLAG_EFIGH))
		{
			for (r=-2; r<=1; r++)
			{
				sim->create_part(-1, x+r, y-2, playerp->elem);
				sim->create_part(-1, x+r+1, y+2, playerp->elem);
				sim->create_part(-1, x-2, y+r+1, playerp->elem);
				sim->create_part(-1, x+2, y+r, playerp->elem);
			}
		}
		else
			sim->create_part(-1, x, y, playerp->elem);
		sim->kill_part(i);  //Kill him
		return 1;
	}

	if (lifeinc[phase])
	{
		(lifeinc[phase] == 3) && (parts[i].life = 0);
		r = lifeinc[phase + 1];
		parts[i].life += r;
		(parts[i].life < 1) && (parts[i].life = (r > 0 ? INT_MAX : 0));
		sim->SimExtraFunc |= 0x1000;
	}

	//Follow gravity
	gvx = gvy = 0.0f;
	switch (sim->gravityMode)
	{
		default:
		case 0:
			gvy = 1;
			break;
		case 1:
			gvy = gvx = 0.0f;
			break;
		case 2:
			{
				float gravd;
				gravd = 0.01f - hypotf((parts[i].x - XCNTR), (parts[i].y - YCNTR));
				gvx = ((float)(parts[i].x - XCNTR) / gravd);
				gvy = ((float)(parts[i].y - YCNTR) / gravd);
			}
			break;
	}

	gvx += sim->gravx[((int)parts[i].y/CELL)*(XRES/CELL)+((int)parts[i].x/CELL)];
	gvy += sim->gravy[((int)parts[i].y/CELL)*(XRES/CELL)+((int)parts[i].x/CELL)];
	
	bool antigrav = sim->Extra_FIGH_pause & 0x200;
	bool prevent_spawn = false;
	int grav_multiplier = antigrav ? -1 : 1, comm = playerp->comm & 0x03;

	if (antigrav) // anti-gravity ?
	{
		gvx = -gvx;
		gvy = -gvy;
	}

	float rbx = gvx;
	float rby = gvy;
	bool rbLowGrav = false;
	float tmp = fabsf(rbx) > fabsf(rby)?fabsf(rbx):fabsf(rby);
	if (tmp < 0.001f)
	{
		rbLowGrav = true;
		rbx = -parts[i].vx;
		rby = -parts[i].vy;
		tmp = fabsf(rbx) > fabsf(rby)?fabsf(rbx):fabsf(rby);
	}
	if (tmp < 0.001f)
	{
		rbx = 0;
		rby = 1.0f;
		tmp = 1.0f;
	}
	else
		tmp = 1.0f/sqrtf(rbx*rbx+rby*rby);
	rbx *= tmp;// scale to a unit vector
	rby *= tmp;
	if (rbLowGrav)
	{
		rocketBootsHeadEffectV = rocketBootsHeadEffect;
		rocketBootsFeetEffectV = rocketBootsFeetEffect;
	}

	parts[i].vx -= gvx*dt;  //Head up!
	parts[i].vy -= gvy*dt;

	//Verlet integration
	pp = 2*playerp->legs_curr[0]-playerp->legs_prev[0]+playerp->accs[0]*dt*dt;
	playerp->legs_prev[0] = playerp->legs_curr[0];
	playerp->legs_curr[0] = pp;
	pp = 2*playerp->legs_curr[1]-playerp->legs_prev[1]+playerp->accs[1]*dt*dt;
	playerp->legs_prev[1] = playerp->legs_curr[1];
	playerp->legs_curr[1] = pp;

	pp = 2*playerp->legs_curr[2]-playerp->legs_prev[2]+(playerp->accs[2]+gvx)*dt*dt;
	playerp->legs_prev[2] = playerp->legs_curr[2];
	playerp->legs_curr[2] = pp;
	pp = 2*playerp->legs_curr[3]-playerp->legs_prev[3]+(playerp->accs[3]+gvy)*dt*dt;
	playerp->legs_prev[3] = playerp->legs_curr[3];
	playerp->legs_curr[3] = pp;

	pp = 2*playerp->legs_curr[4]-playerp->legs_prev[4]+playerp->accs[4]*dt*dt;
	playerp->legs_prev[4] = playerp->legs_curr[4];
	playerp->legs_curr[4] = pp;
	pp = 2*playerp->legs_curr[5]-playerp->legs_prev[5]+playerp->accs[5]*dt*dt;
	playerp->legs_prev[5] = playerp->legs_curr[5];
	playerp->legs_curr[5] = pp;

	pp = 2*playerp->legs_curr[6]-playerp->legs_prev[6]+(playerp->accs[6]+gvx)*dt*dt;
	playerp->legs_prev[6] = playerp->legs_curr[6];
	playerp->legs_curr[6] = pp;
	pp = 2*playerp->legs_curr[7]-playerp->legs_prev[7]+(playerp->accs[7]+gvy)*dt*dt;
	playerp->legs_prev[7] = playerp->legs_curr[7];
	playerp->legs_curr[7] = pp;

	//Setting acceleration to 0
	playerp->accs[0] = 0;
	playerp->accs[1] = 0;

	playerp->accs[2] = 0;
	playerp->accs[3] = 0;

	playerp->accs[4] = 0;
	playerp->accs[5] = 0;

	playerp->accs[6] = 0;
	playerp->accs[7] = 0;

	gx = (playerp->legs_curr[2] + playerp->legs_curr[6])/2 - gvy;
	gy = (playerp->legs_curr[3] + playerp->legs_curr[7])/2 + gvx;
	dl = pow(gx - playerp->legs_curr[2], 2) + pow(gy - playerp->legs_curr[3], 2);
	dr = pow(gx - playerp->legs_curr[6], 2) + pow(gy - playerp->legs_curr[7], 2);
	dx = dl - dr;
	dy = (comm & 1) - (comm >> 1);

	//Go left or right
	if (playerp->comm)
	{
		bool moved = false;
		dx *= dy;
		if (dx > 0)
		{
			if (is_blocking(sim, playerp, t, playerp->legs_curr[2], playerp->legs_curr[3]))
			{
				playerp->accs[2] = -dy * 3 *gvy - 3 * gvx;
				playerp->accs[3] = 3 * gvx - 3 * gvy;
				playerp->accs[0] = -dy * gvy;
				playerp->accs[1] = gvx;
				moved = true;
			}
		}
		else
		{
			if (is_blocking(sim, playerp, t, playerp->legs_curr[6], playerp->legs_curr[7]))
			{
				playerp->accs[6] = -dy * 3 *gvy - 3 * gvx;
				playerp->accs[7] = 3 * gvx - 3 * gvy;
				playerp->accs[0] = -dy * gvy;
				playerp->accs[1] = gvx;
				moved = true;
			}
		}
		if (!moved && playerp->rocketBoots)
		{
			parts[i].vx -= rocketBootsHeadEffect*rby;
			parts[i].vy += rocketBootsHeadEffect*rbx;
			playerp->accs[2] -= rocketBootsFeetEffect*rby;
			playerp->accs[6] -= rocketBootsFeetEffect*rby;
			playerp->accs[3] += rocketBootsFeetEffect*rbx;
			playerp->accs[7] += rocketBootsFeetEffect*rbx;
			if (!(sim->Extra_FIGH_pause & 0x400))
			{
				int comm = playerp->comm; dy *= 25;
				for (int leg=0; leg<2; leg++)
				{
					if (!(comm & (1 << leg)))
						continue;
					int footX = playerp->legs_curr[leg*4+2], footY = playerp->legs_curr[leg*4+3];
					int np = sim->create_part(-1, footX, footY, PT_PLSM);
					if (np>=0)
					{
						parts[np].vx = parts[i].vx+rby*dy;
						parts[np].vy = parts[i].vy-rbx*dy;
						parts[np].life += 30;
					}
				}
			}
		}
	}

	if (playerp->rocketBoots && comm == 0x03)
	{
		// Pressing left and right simultaneously with rocket boots on slows the stickman down
		// Particularly useful in zero gravity
		parts[i].vx *= 0.5f;
		parts[i].vy *= 0.5f;
		playerp->accs[2] = playerp->accs[6] = 0;
		playerp->accs[3] = playerp->accs[7] = 0;
	}

	//Jump
	if (((int)(playerp->comm)&0x04) == 0x04)
	{
		if (playerp->rocketBoots)
		{
			parts[i].vx -= rocketBootsHeadEffectV*rbx;
			parts[i].vy -= rocketBootsHeadEffectV*rby;
			playerp->accs[2] -= rocketBootsFeetEffectV*rbx;
			playerp->accs[6] -= rocketBootsFeetEffectV*rbx;
			playerp->accs[3] -= rocketBootsFeetEffectV*rby;
			playerp->accs[7] -= rocketBootsFeetEffectV*rby;
			if (!(sim->Extra_FIGH_pause & 0x400))
			{
				for (int leg=0; leg<2; leg++)
				{
					int footX = playerp->legs_curr[leg*4+2], footY = playerp->legs_curr[leg*4+3];
					int np = sim->create_part(-1, footX, footY+1, PT_PLSM);
					if (np>=0)
					{
						parts[np].vx = parts[i].vx+rbx*30;
						parts[np].vy = parts[i].vy+rby*30;
						parts[np].life += 10;
					}
				}
			}
		}
		else if (is_blocking(sim, playerp, t, playerp->legs_curr[2], playerp->legs_curr[3]) ||
				 is_blocking(sim, playerp, t, playerp->legs_curr[6], playerp->legs_curr[7]))
		{
			parts[i].vx -= 4*gvx;
			parts[i].vy -= 4*gvy;
			playerp->accs[2] -= gvx;
			playerp->accs[6] -= gvx;
			playerp->accs[3] -= gvy;
			playerp->accs[7] -= gvy;
		}
	}

	//Charge detector wall if foot inside
	rx = (int)(playerp->legs_curr[2]+0.5)/CELL;
	ry = (int)(playerp->legs_curr[3]+0.5)/CELL;
	if (INBOND(rx, ry) && sim->bmap[ry][rx]==WL_DETECT)
		sim->set_emap(rx, ry);

	rx = (int)(playerp->legs_curr[6]+0.5)/CELL;
	ry = (int)(playerp->legs_curr[7]+0.5)/CELL;
	if (INBOND(rx, ry) && sim->bmap[ry][rx]==WL_DETECT)
	    sim->set_emap(rx, ry);

	int rndstore, randpool = 0, under_wall, rt;
	//Searching for particles near head
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					r = sim->photons[y+ry][x+rx];

				if (!r && !sim->bmap[(y+ry)/CELL][(x+rx)/CELL])
					continue;
				
				rt = TYP(r); r = ID(r);
				
				if (!(sim->Extra_FIGH_pause & 32 || rt == PT_E195 && _IS_EMIT_FN(playerp->__flags, _STKM_FLAG_EMT)))
				{
					STKM_set_element(sim, playerp, rt);
				}
				if (!(sim->Extra_FIGH_pause & 16))
				{
					int l_upper = lifeinc[4];
					if (rt == PT_PLNT && parts[i].life < l_upper) //Plant gives him 5 HP
					{
						if (parts[i].life <= (l_upper - 5))
							parts[i].life += 5;
						else
							parts[i].life = l_upper;
						sim->kill_part(r);
					}

					if (rt == PT_NEUT)
					{
						if (parts[i].life<=100) parts[i].life -= (102-parts[i].life)/2;
						else parts[i].life *= 0.9f;
						sim->kill_part(r);
					}
				}

				if (rt == ELEM_MULTIPP)
				{
					if (!randpool)
					{
						randpool = 7;
						rndstore = rand(); // max refresh 4 times
					}
					if (!(rndstore & 3)) // condition: rand % 4 == 0 (actually 25% chance)
					{
						STKM_set_life_1(sim, r, i);
					}
					rndstore >>= 2; randpool--;
					if (parts[r].life == 27 && !(sim->Extra_FIGH_pause & 32))
					{
						ctype = parts[r].ctype;
						int xctype = ctype >> PMAPBITSp1;
						ctype &= ((1 << PMAPBITSp1) - 1); 

						STKM_set_element_Ex(sim, playerp, ctype);

						if (xctype == 1 || xctype == 2)
						{
							playerp->rocketBoots = xctype == 1 ? false : true;
						}
					}
					else if (parts[r].life == 16 && parts[r].ctype == 31 && !prevent_spawn)
					{
						prevent_spawn = true;
						if (((int)(playerp->comm)&0x08) == 0x08)
						{
							int spawnID = -1;
							if (t == PT_STKM)
								spawnID = sim->player.spawnID;
							else if (t == PT_STKM2)
								spawnID = sim->player2.spawnID;
							if (spawnID >= 0)
							{
								sim->part_change_type(spawnID, (int)(parts[spawnID].x+0.5f), (int)(parts[spawnID].y+0.5f), ELEM_MULTIPP);
								parts[spawnID].life = 16;
								parts[spawnID].ctype = 31;
							}
							if (t == PT_STKM)
								sim->part_change_type(r, x+rx, y+ry, PT_SPAWN);
							else if (t == PT_STKM2)
								sim->part_change_type(r, x+rx, y+ry, PT_SPAWN2);
						}
					}
				}

				under_wall = sim->bmap[(ry+y)/CELL][(rx+x)/CELL];
				switch (under_wall)
				{
				case WL_FAN:
					STKM_set_element_Ex(sim, playerp, (sim->Extra_FIGH_pause & 0x800) ? PMAP(1, 1) : PMAPID(1));
					break;
				case WL_EHOLE:
					playerp->rocketBoots = false;
					break;
				case WL_GRAV: /* && parts[i].type!=PT_FIGH */
					playerp->rocketBoots = true;
					break;
				}
				if (rt == PT_PRTI)
					Element_STKM::STKM_interact(sim, playerp, i, rx, ry, grav_multiplier);
				if (!parts[i].type)//STKM_interact may kill STKM
					return 1;
			}

	//Head position
	rx = x + grav_multiplier * (3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01));
	ry = y - grav_multiplier * (3*(playerp->pcomm == 0));

	if (prevent_spawn)
		playerp->commf |= _STKM_CMD_FLAG_NOSPAWN;
	
	//Spawn
	if (playerp->commf & _STKM_CMD_FLAG_NOSPAWN)
	{
		if (((int)(playerp->comm)&0x08) != 0x08)
			playerp->commf &= ~_STKM_CMD_FLAG_NOSPAWN;
	}
	else if (((int)(playerp->comm)&0x08) == 0x08)
	{
		ry -= grav_multiplier * (2*(rand()%2)+1);
		r = pmap[ry][rx];
		if ((sim->elements[TYP(r)].Properties&TYPE_SOLID) && TYP(r) != ELEM_MULTIPP)
		{
			sim->create_part(-1, rx, ry, PT_SPRK);
			playerp->frames = 0;
		}
		else
		{
			const int emt_ell[] = {0, PT_E195, PT_FIGH, 0};
			int flags = playerp->__flags;
			int np = -1;
			int emt_el = emt_ell[(flags & _STKM_FLAG_EPROP) >> _STKM_FLAG_EPROPSH];
			int nelem = (emt_el > 0) ? emt_el : playerp->elem;
			if (playerp->fan)
			{
				float air_dif = (flags & _STKM_FLAG_VAC) ? -0.03f : 0.03f;
				for (int j = -4; j < 5; j++)
					for (int k = -4; k < 5; k++)
					{
						int airx = rx + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01)+j;
						int airy = ry+k;
						sim->pv[airy/CELL][airx/CELL] += air_dif;
						if (airy + CELL < YRES)
							sim->pv[airy/CELL+1][airx/CELL] += air_dif;
						if (airx + CELL < XRES)
						{
							sim->pv[airy/CELL][airx/CELL+1] += air_dif;
							if (airy + CELL < YRES)
								sim->pv[airy/CELL+1][airx/CELL+1] += air_dif;
						}
					}
			}
			else if ((nelem == PT_LIGH || nelem == PT_FIGH) && playerp->frames<30)//limit lightning and fighter creation rate
				np = -1;
			else
				np = sim->create_part(-1, rx, ry, nelem);
			if ( (np < NPART) && np>=0)
			{
				if (nelem == PT_PHOT)
				{
					int random = abs(rand()%3-1)*3;
					if (random==0)
					{
						sim->kill_part(np);
					}
					else
					{
						parts[np].vy = 0;
						if (((int)playerp->pcomm)&(0x01|0x02))
							parts[np].vx = (((((int)playerp->pcomm)&0x02) == 0x02) - (((int)(playerp->pcomm)&0x01) == 0x01))*random;
						else
							parts[np].vx = random;
					}
				}
				else if (nelem == PT_E195)
				{
					newpart = &parts[np];
				}
				else if (nelem == PT_LIGH)
				{
					float angle;
					int power = 100;
					if (gvx!=0 || gvy!=0)
						angle = atan2(gvx, gvy)*180.0f/M_PI;
					else
						angle = rand()%360;
					if (((int)playerp->pcomm)&0x01)
						angle += 180;
					if (angle>360)
						angle-=360;
					if (angle<0)
						angle+=360;
					parts[np].tmp = angle;
					parts[np].life=rand()%(2+power/15)+power/7;
					parts[np].temp=parts[np].life*power/2.5;
					parts[np].tmp2=1;
				}
				else if (!playerp->fan)
				{
					parts[np].vx -= -gvy*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[np].vy -= gvx*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[i].vx -= (sim->elements[(int)nelem].Weight*parts[np].vx)/1000;
					if (nelem == PT_FIGH)
						createSTKMChild2(sim, playerp, i, parts[np].tmp);
				}
				playerp->frames = 0;
			}

		}
	}

	//Simulation of joints
	dx = playerp->legs_curr[0] - playerp->legs_curr[2];
	dy = playerp->legs_curr[1] - playerp->legs_curr[3];
	d = 25/(pow(dx, 2) + pow(dy, 2)+25) - 0.5;  //Fast distance
	playerp->legs_curr[2] -= dx * d;
	playerp->legs_curr[3] -= dy * d;
	playerp->legs_curr[0] += (playerp->legs_curr[0]-playerp->legs_curr[2])*d;
	playerp->legs_curr[1] += (playerp->legs_curr[1]-playerp->legs_curr[3])*d;

	dx = playerp->legs_curr[4] - playerp->legs_curr[6];
	dy = playerp->legs_curr[5] - playerp->legs_curr[7];
	d = 25/(pow(dx, 2) + pow(dy, 2)+25) - 0.5;
	playerp->legs_curr[6] -= dx * d;
	playerp->legs_curr[7] -= dy * d;
	playerp->legs_curr[4] += (playerp->legs_curr[4]-playerp->legs_curr[6])*d;
	playerp->legs_curr[5] += (playerp->legs_curr[5]-playerp->legs_curr[7])*d;

	dx = playerp->legs_curr[0] - parts[i].x;
	dy = playerp->legs_curr[1] - parts[i].y;
	d = 36/(pow(dx, 2) + pow(dy, 2)+36) - 0.5;
	parts[i].vx -= dx * d;
	parts[i].vy -= dy * d;
	playerp->legs_curr[0] += dx * d;
	playerp->legs_curr[1] += dy * d;

	dx = playerp->legs_curr[4] - parts[i].x;
	dy = playerp->legs_curr[5] - parts[i].y;
	d = 36/(pow(dx, 2) + pow(dy, 2)+36) - 0.5;
	parts[i].vx -= dx * d;
	parts[i].vy -= dy * d;
	playerp->legs_curr[4] += dx * d;
	playerp->legs_curr[5] += dy * d;

	if (is_blocking(sim, playerp, t, playerp->legs_curr[2], playerp->legs_curr[3]))
	{
		playerp->legs_curr[2] = playerp->legs_prev[2];
		playerp->legs_curr[3] = playerp->legs_prev[3];
	}

	if (is_blocking(sim, playerp, t, playerp->legs_curr[6], playerp->legs_curr[7]))
	{
		playerp->legs_curr[6] = playerp->legs_prev[6];
		playerp->legs_curr[7] = playerp->legs_prev[7];
	}

	//This makes stick man "pop" from obstacles
	if (is_blocking(sim, playerp, t, playerp->legs_curr[2], playerp->legs_curr[3]))
	{
		float t;
		t = playerp->legs_curr[2]; playerp->legs_curr[2] = playerp->legs_prev[2]; playerp->legs_prev[2] = t;
		t = playerp->legs_curr[3]; playerp->legs_curr[3] = playerp->legs_prev[3]; playerp->legs_prev[3] = t;
	}

	if (is_blocking(sim, playerp, t, playerp->legs_curr[6], playerp->legs_curr[7]))
	{
		float t;
		t = playerp->legs_curr[6]; playerp->legs_curr[6] = playerp->legs_prev[6]; playerp->legs_prev[6] = t;
		t = playerp->legs_curr[7]; playerp->legs_curr[7] = playerp->legs_prev[7]; playerp->legs_prev[7] = t;
	}

	//Keeping legs distance
	if ((pow((playerp->legs_curr[2] - playerp->legs_curr[6]), 2) + pow((playerp->legs_curr[3]-playerp->legs_curr[7]), 2))<16)
	{
		float tvx, tvy;
		tvx = -gvy;
		tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[2] -= 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[3] -= 0.2*tvy/hypot(tvx, tvy);

			playerp->accs[6] += 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[7] += 0.2*tvy/hypot(tvx, tvy);
		}
	}

	if ((pow((playerp->legs_curr[0] - playerp->legs_curr[4]), 2) + pow((playerp->legs_curr[1]-playerp->legs_curr[5]), 2))<16)
	{
		float tvx, tvy;
		tvx = -gvy;
		tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[0] -= 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[1] -= 0.2*tvy/hypot(tvx, tvy);

			playerp->accs[4] += 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[5] += 0.2*tvy/hypot(tvx, tvy);
		}
	}

	//If legs touch something
	Element_STKM::STKM_interact(sim, playerp, i, (int)(playerp->legs_curr[2]+0.5), (int)(playerp->legs_curr[3]+0.5), grav_multiplier);
	Element_STKM::STKM_interact(sim, playerp, i, (int)(playerp->legs_curr[6]+0.5), (int)(playerp->legs_curr[7]+0.5), grav_multiplier);
	Element_STKM::STKM_interact(sim, playerp, i, (int)(playerp->legs_curr[2]+0.5), (int)playerp->legs_curr[3], grav_multiplier);
	Element_STKM::STKM_interact(sim, playerp, i, (int)(playerp->legs_curr[6]+0.5), (int)playerp->legs_curr[7], grav_multiplier);
	if (!parts[i].type)
		return 1;

	if (!(sim->Extra_FIGH_pause & 8))
		_EMIT_CLR_T(playerp->__flags, _STKM_FLAG_EFIGH);

	parts[i].ctype = playerp->elem;
	
	if (newpart)
	{
		int rndstore = rand();
		if (!rbLowGrav)
		{
			float rad = 3.0f;
			float angle = (rndstore % 101 - 50) * 0.002f + atan2f(-gvx, gvy);
			int comm = (int)playerp->pcomm;
			if ((comm & 3) == 1 || (comm & 3) != 2 && (rndstore % 2))
				rad = -rad;
			newpart->vx = rad*cosf(angle);
			newpart->vy = rad*sinf(angle);
		}
		else
		{
			newpart->vx *= 1.5f;
			newpart->vy *= 1.5f;
		}
		newpart->ctype = playerp->elem;
		// if (newpart->ctype > PMAPMASK)
		//	newpart->ctype = 0;
	}
	return 0;
}

//#TPT-Directive ElementHeader Element_STKM static void STKM_interact(Simulation *sim, playerst *playerp, int i, int x, int y, int gmult)
void Element_STKM::STKM_interact(Simulation *sim, playerst *playerp, int i, int x, int y, int gmult)
{
	int r, rt;
	if (x<0 || y<0 || x>=XRES || y>=YRES || !sim->parts[i].type)
		return;
	r = sim->pmap[y][x];
	if (r)
	{
		if (TYP(r) == PT_PINVIS)
			r = sim->parts[ID(r)].tmp4;

		rt = TYP(r); r = ID(r);

		if (!(sim->Extra_FIGH_pause & 16))
		{
			if (rt == PT_SPRK && playerp->elem!=PT_LIGH) //If on charge
			{
				sim->parts[i].life -= (int)(rand()*20/RAND_MAX)+32;
			}

			if (sim->GetHeatConduct(r, rt) &&
				((playerp->elem!=PT_LIGH && sim->parts[r].temp>=323) || sim->parts[r].temp<=243) &&
				(!playerp->rocketBoots || rt != PT_PLSM))
			{
				sim->parts[i].life -= 2;
				playerp->accs[3] -= gmult;
			}
				
			if (sim->elements[rt].Properties&PROP_DEADLY)
				switch (rt)
				{
					case PT_ACID:
						sim->parts[i].life -= 5;
						break;
					default:
						sim->parts[i].life -= 1;
						break;
				}

			if (sim->elements[rt].Properties&PROP_RADIOACTIVE)
				sim->parts[i].life -= 1;
		}
		
		if (rt == ELEM_MULTIPP)
		{
			STKM_set_life_1(sim, r, i);
			if (sim->parts[r].life == 23)
			{
				sim->parts[i].vy -= 3;
				playerp->accs[3] -= gmult * 3;
				playerp->accs[7] -= gmult * 3;
			}
		}

		if (rt == PT_PRTI && sim->parts[i].type)
		{
			int nnx, count=1;//gives rx=0, ry=1 in update_PRTO
			sim->parts[r].tmp = (int)((sim->parts[r].temp-73.15f)/100+1);
			if (sim->parts[r].tmp>=CHANNELS) sim->parts[r].tmp = CHANNELS-1;
			else if (sim->parts[r].tmp<0) sim->parts[r].tmp = 0;
			for (nnx=0; nnx<80; nnx++)
				if (!sim->portalp[sim->parts[r].tmp][count][nnx].type)
				{
					sim->portalp[sim->parts[r].tmp][count][nnx] = sim->parts[i];
					sim->kill_part(i);
					//stop new STKM/fighters being created to replace the ones in the portal:
					playerp->spwn = 1;
					if (sim->portalp[sim->parts[r].tmp][count][nnx].type==PT_FIGH)
						sim->fighcount++;
					break;
				}
		}
		if ((rt == PT_BHOL || rt == PT_NBHL) && sim->parts[i].type)
		{
			if (!sim->legacy_enable)
			{
				sim->parts[r].temp = restrict_flt(sim->parts[r].temp+sim->parts[i].temp/2, MIN_TEMP, MAX_TEMP);
			}
			sim->kill_part(i);
		}
		if ((rt == PT_VOID || (rt == PT_PVOD && sim->parts[r].life==10)) &&
			(!sim->parts[r].ctype || (sim->parts[r].ctype == sim->parts[i].type) != (sim->parts[r].tmp&1)) &&
			sim->parts[i].type)
		{
			sim->kill_part(i);
		}
	}
}

//#TPT-Directive ElementHeader Element_STKM static bool is_blocking(Simulation *sim, playerst *playerp, int t, float x, float y)
bool Element_STKM::is_blocking(Simulation *sim, playerst *playerp, int t, float x, float y)
{
	if (INBOND(x, y))
	{
		return !sim->eval_move(t, x, y, NULL);
	}
	return false;
}

//#TPT-Directive ElementHeader Element_STKM static void STKM_clear(Simulation *sim, playerst *playerp)
void Element_STKM::STKM_clear(Simulation * sim, playerst *playerp)
{
	playerp->spwn = 0;
	playerp->spawnID = -1;
	playerp->rocketBoots = false;
	playerp->fan = false;
}

//#TPT-Directive ElementHeader Element_STKM static void STKM_init_legs(Simulation *sim, playerst *playerp, int i)
void Element_STKM::STKM_init_legs(Simulation *sim, playerst *playerp, int i)
{
	int j, x, y;

	x = (int)(sim->parts[i].x+0.5f);
	y = (int)(sim->parts[i].y+0.5f);
	
	int gmult = (sim->Extra_FIGH_pause & 0x200) ? -1 : 1;
	int _leg_v[] = {-1,6,-3,12,1,6,3,12};

	for (j = 0; j < 8; j += 2)
	{
		playerp->legs_curr[j] = x + gmult*_leg_v[j];
		playerp->legs_curr[j+1] = y + gmult*_leg_v[j+1];
	}
	for (j = 0; j < 8; j++)
	{
		playerp->legs_prev[j] = playerp->legs_curr[j];
		playerp->accs[j] = 0;
	}
	playerp->comm = 0;
	playerp->pcomm = 0;
	playerp->frames = 0;
	playerp->fan = false;
	playerp->__flags = 0;
	playerp->parentStickman = -1;
	playerp->firstChild = -1;
	playerp->prevStickman = -1;
	playerp->nextStickman = -1;
	playerp->lastChild = -1;
	playerp->self_ID = i;
	playerp->underp = 0;
}


//#TPT-Directive ElementHeader Element_STKM static void STKM_set_element_Ex(Simulation *sim, playerst *playerp, int element)
void Element_STKM::STKM_set_element_Ex(Simulation *sim, playerst *playerp, int element)
{
	bool mask_ovr = element & PMAPID(2);
	bool mask_ext = element & PMAPID(1);
	element &= PMAPMASK;

	if (mask_ext)
	{
		if (element < 5)
		{
			if (element < 2)
				playerp->fan = true;

			int exl0[5] = {~_STKM_FLAG_VAC, ~0, ~_STKM_FLAG_EPROP, ~_STKM_FLAG_EPROP, ~0};
			int exl1[5] = {0, _STKM_FLAG_VAC, _STKM_FLAG_EMT, 0, _STKM_FLAG_SUSPEND};

			playerp->__flags &= exl0[element];
			playerp->__flags |= exl1[element];
		}
		else if (element == 5)
			playerp->fan = false;
	}
	else if ((element > 0) && (element < PT_NUM))
		STKM_set_element(sim, playerp, element);

	if (mask_ovr)
		playerp->__flags |= _STKM_FLAG_OVR;
}

//#TPT-Directive ElementHeader Element_STKM static void STKM_set_element(Simulation *sim, playerst *playerp, int element)
void Element_STKM::STKM_set_element(Simulation *sim, playerst *playerp, int element)
{
	if (sim->elements[element].Falldown != 0
	    || sim->elements[element].Properties&TYPE_GAS
	    || sim->elements[element].Properties&TYPE_LIQUID
	    || sim->elements[element].Properties&TYPE_ENERGY
	    || element == PT_LOLZ || element == PT_LOVE)
	{
		if (!playerp->rocketBoots || element != PT_PLSM)
		{
			playerp->elem = element;
			playerp->fan = false;
		}
	}
	if (element == PT_TESC || element == PT_LIGH)
	{
		playerp->elem = PT_LIGH;
		playerp->fan = false;
	}
	if (element == PT_FIGH && (sim->Extra_FIGH_pause & 8))
	{
		_EMIT_SET_A(playerp->__flags, _STKM_FLAG_EFIGH);
		playerp->fan = false;
	}
}

//#TPT-Directive ElementHeader Element_STKM static void STKM_set_life_1(Simulation *sim, int s, int i)
void Element_STKM::STKM_set_life_1(Simulation *sim, int s, int i)
{
	if (sim->parts[s].life == 16 && sim->parts[s].ctype == 5)
	{
		int sur_part_tmp = sim->parts[s].tmp >> 6;
		if (sur_part_tmp > 0)
		{
			sur_part_tmp--; // is temporary variable, not global
			int t = sim->parts[i].type;
			int inc_life = (sur_part_tmp & 1) ? 1 : -1;
			bool inc_life_cond = false;
			switch (sur_part_tmp >> 1)
			{
				case 0:
					inc_life_cond = true;
				break;
				case 1:
					inc_life_cond = (t == PT_STKM);
				break;
				case 2:
					inc_life_cond = (t == PT_STKM2);
				break;
				case 3:
					inc_life_cond = (t == PT_STKM || t == PT_STKM2);
				break;
				case 4:
					inc_life_cond = (t == PT_FIGH);
				break;
			}
			if (inc_life_cond)
			{
				int limit = lifeinc[4];

				if (inc_life < 0 && -sim->parts[i].life > inc_life)
					inc_life = -sim->parts[i].life;
				else if (inc_life > 0 && limit - sim->parts[i].life < inc_life)
					inc_life = limit - sim->parts[i].life;

				sim->parts[i].life += inc_life;
			}
		}
	}
}

//#TPT-Directive ElementHeader Element_STKM static void removeSTKMChilds(Simulation *sim, playerst* playerp)
void Element_STKM::removeSTKMChilds(Simulation *sim, playerst* playerp)
{
	int child_f = playerp->firstChild;
	while (child_f >= 0)
	{
		sim->fighters[child_f].parentStickman = -1;
		child_f = sim->fighters[child_f].nextStickman;
	}
}

//#TPT-Directive ElementHeader Element_STKM static playerst* _get_playerst(Simulation *sim, int stickmanID)
playerst* Element_STKM::_get_playerst(Simulation *sim, int stickmanID)
{
	if (stickmanID >= 0 && stickmanID < MAX_FIGHTERS)
		return &sim->fighters[stickmanID];
	else if (stickmanID == MAX_FIGHTERS)
		return &sim->player;
	else if (stickmanID == (MAX_FIGHTERS + 1))
		return &sim->player2;
	return NULL;
}

//#TPT-Directive ElementHeader Element_STKM static void createSTKMChild(Simulation *sim, playerst* playerp, int parentf, int nc)
void Element_STKM::createSTKMChild(Simulation *sim, playerst* playerp, int parentf, int nc)
{
	int old_FIGH_id, new_FIGH_id;
	old_FIGH_id = playerp->lastChild;
	new_FIGH_id = nc;
	playerp->lastChild = new_FIGH_id;
	if (playerp->firstChild < 0)
		playerp->firstChild = new_FIGH_id;
	else {
		sim->fighters[new_FIGH_id].prevStickman = old_FIGH_id;
		sim->fighters[old_FIGH_id].nextStickman = new_FIGH_id;
	}
	sim->fighters[new_FIGH_id].parentStickman = parentf;
}

//#TPT-Directive ElementHeader Element_STKM static void createSTKMChild2(Simulation *sim, playerst* playerp, int i, int nc)
void Element_STKM::createSTKMChild2(Simulation *sim, playerst* playerp, int i, int nc)
{
	int parentf = -1;
	if (sim->parts[i].type == PT_FIGH)
		parentf = sim->parts[i].tmp;
	else if (sim->parts[i].type == PT_STKM)
		parentf = MAX_FIGHTERS;
	else if (sim->parts[i].type == PT_STKM2)
		parentf = MAX_FIGHTERS + 1;
	createSTKMChild(sim, playerp, parentf, nc);
}

Element_STKM::~Element_STKM() {}
