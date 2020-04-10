#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_SLCN PT_SLCN 187
Element_SLCN::Element_SLCN()
{
	Identifier = "DEFAULT_PT_SLCN";
	Name = "SLCN";
	Colour = PIXPACK(0xBCCDDF);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.27f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 90;

	HeatConduct = 100;
	Description = "Powdered Silicon. A key element in multiple materials.";

	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3538.15f;
	HighTemperatureTransition = PT_LAVA;
	//PhotonReflectWavelengths = TODO;

	Update = &Element_SLCN::update;
	Graphics = &Element_SLCN::graphics;
}

// This list provides the color wheel used for SLCN's sparkle effect.
// Colors sampled from a public domain picture of pure silicon.
// https://upload.wikimedia.org/wikipedia/commons/e/e9/SiliconCroda.jpg
static int SLCN_SPARKLE[16] = {
	PIXPACK(0x5A6679),
	PIXPACK(0x6878A1),
	PIXPACK(0xABBFDD),
	PIXPACK(0x838490),
	
	PIXPACK(0xBCCDDF),
	PIXPACK(0x82A0D2),
	PIXPACK(0x5B6680),
	PIXPACK(0x232C3B),

	PIXPACK(0x485067),
	PIXPACK(0x8B9AB6),
	PIXPACK(0xADB1C1),
	PIXPACK(0xC3C6D1),
	
	PIXPACK(0x8594AD),
	PIXPACK(0x262F47),
	PIXPACK(0xA9AEBC),
	PIXPACK(0xC2E1F7),
};

static int SLCN_SPARKLE_FX[16] = {
	0,
	0,
	PMODE_FLARE | PMODE_GLOW,
	0,

	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	0,
	0,
	0,
	PMODE_SPARK,
};

static const float SPARKLE_RATE = 0.01f;
static const float VELOCITY_MULTIPLIER = 7.0f;
static const float PHASE_THRESHOLD = 60.0f;

//#TPT-Directive ElementHeader Element_SLCN static int update(UPDATE_FUNC_ARGS)
int Element_SLCN::update(UPDATE_FUNC_ARGS)
{
	Particle &self = parts[i];

	float velocity = std::sqrt(std::pow(self.vx, 2) + std::pow(self.vy, 2));

	//FROM: GOLD.cpp:74 but prettied up a bit.
	if (self.life == 0 && self.temp < 373.15f)
	{
		for (int j = 0; j < 4; j++)
		{ 
			int check_coords_x[] = { -4, 4, 0, 0 };
			int check_coords_y[] = { 0, 0, -4, 4 };

			int neighbour_x = check_coords_x[j];
			int neighbour_y = check_coords_y[j];

			int neighbour_id = pmap[y + neighbour_y][x + neighbour_x];

			if (neighbour_id == 0) continue;

			int neighbour_type = TYP(neighbour_id);
			Particle &neighbour = parts[ID(neighbour_id)];

			if (neighbour_type == PT_SPRK && neighbour.life != 0 && neighbour.life < 4)
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				self.life = 4;
				self.ctype = PT_SLCN;
			}
		}
	}

	int gt_phase_count = 0;

	for (int neighbour_x=-2; neighbour_x<3; neighbour_x++)
		for (int neighbour_y=-2; neighbour_y<3; neighbour_y++) 
		{
			int neighbour_id = pmap[y + neighbour_y][x + neighbour_x];

			if (neighbour_id == 0) continue;

			int neighbour_type = TYP(neighbour_id);
			Particle &neighbour = parts[ID(neighbour_id)];
			if (neighbour_type == PT_SLCN)
			{
				if (self.tmp == neighbour.tmp)
				{
					gt_phase_count += 1;
				}
			}
		}

	// Phase change logic.
	int current_color_phase = self.tmp & 31; // limited to 0 through 31. using bitlogic for simplicity.
	int current_spark_phase = self.tmp & 15; // limited to 0 through 15.

	float &clr_phse_transition_cnt = self.pavg[0];
	float &sprk_phse_transition_cnt = self.pavg[1];

	// prevent these values from going through the roof.
	clr_phse_transition_cnt = clamp_flt(clr_phse_transition_cnt, 0.0f, PHASE_THRESHOLD * 2);
	sprk_phse_transition_cnt = clamp_flt(sprk_phse_transition_cnt, 0.0f, PHASE_THRESHOLD * 2);

	clr_phse_transition_cnt += (velocity * VELOCITY_MULTIPLIER) + SPARKLE_RATE + (rand() / (float)RAND_MAX * SPARKLE_RATE);
	self.pavg[1] += (velocity * VELOCITY_MULTIPLIER) + SPARKLE_RATE + (rand() / (float)RAND_MAX * SPARKLE_RATE);

	// evil trick to combat syncronization.
	if (gt_phase_count > 3) {
		clr_phse_transition_cnt -= 0.2;
		if (!(rand() % 30)) {
			self.tmp = rand() % 32;
			self.tmp2 = rand() % 16;
			clr_phse_transition_cnt = (float)(rand() % (int)(PHASE_THRESHOLD + 1));
			sprk_phse_transition_cnt = (float)(rand() % (int)(PHASE_THRESHOLD + 1));
		}
	}

	if (clr_phse_transition_cnt > PHASE_THRESHOLD) {
		clr_phse_transition_cnt -= PHASE_THRESHOLD;
		self.tmp = (current_color_phase + 1) & 31;
	}

	// check if currently in a FX phase, and if so, greatly accelerate phase change.
	if (SLCN_SPARKLE_FX[self.tmp2] != 0) {
		sprk_phse_transition_cnt += rand() / (float)RAND_MAX * PHASE_THRESHOLD; // good chance of skipping the phase.
	}

	if (self.pavg[1] > PHASE_THRESHOLD) {
		sprk_phse_transition_cnt -= PHASE_THRESHOLD;
		self.tmp2 = (current_spark_phase + 1) & 15;
	}

	return 0;
}

//#TPT-Directive ElementHeader Element_SLCN static int graphics(GRAPHICS_FUNC_ARGS)
int Element_SLCN::graphics(GRAPHICS_FUNC_ARGS)
{
	int phase = cpart->tmp & 31;

	int selected_color = 0xFF00FF; // Debug color. will be shown in case of error.
	if ((phase & 1) == 0) {
		selected_color = SLCN_SPARKLE[phase >> 1];
	} else {
		int color_a = SLCN_SPARKLE[phase >> 1];
		int color_b = SLCN_SPARKLE[((phase + 1) >> 1) & 15];
		selected_color = 
			PIXRGB(
				(PIXR(color_a) + PIXR(color_b)) / 2,
				(PIXG(color_a) + PIXG(color_b)) / 2,
				(PIXB(color_a) + PIXB(color_b)) / 2
			);
	}
	*colr = PIXR(selected_color);
	*colg = PIXG(selected_color);
	*colb = PIXB(selected_color);

	*pixel_mode |= SLCN_SPARKLE_FX[cpart->tmp2 & 15];

	return 0;
}

//#TPT-Directive ElementHeader Element_SLCN static void init(Simulation *sim, int i)
void Element_SLCN::init (Simulation *sim, int i)
{
	sim->parts[i].tmp = (rand() % 32);
	sim->parts[i].tmp2 = (rand() % 16);
	sim->parts[i].pavg[0] = (float)(rand() % (int)(PHASE_THRESHOLD + 1));
	sim->parts[i].pavg[1] = (float)(rand() % (int)(PHASE_THRESHOLD + 1));
}

Element_SLCN::~Element_SLCN() {}
