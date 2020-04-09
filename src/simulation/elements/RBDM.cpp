#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_RBDM PT_RBDM 41
Element_RBDM::Element_RBDM()
{
	Identifier = "DEFAULT_PT_RBDM";
	Name = "RBDM";
	Colour = PIXPACK(0xCCCCCC);
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
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

	Flammable = 1000;
	Explosive = 1;
	Meltable = 50;
	Hardness = 1;

	Weight = 100;

	HeatConduct = 240;
	Description = "Rubidium. Explosive, especially on contact with water. Low melting point.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 312.0f;
	HighTemperatureTransition = PT_LRBD;

	Update = NULL;
}

#if 0
Element_RBDM::Isotope_40K::Update (UPDATE_FUNC_ARGS)
{
	if ((slowly decays))
	{
		if (rand() % 10) // approx. 90%
		{
			int (ray id) = sim->create_part(-3, x, y, PT_ELEC); // beta decay
			if ((ray id) >= 0)
			{
				parts[(ray id)].source = PT_RBDM;
				parts[(ray id)].actual_source = ATOM_POTASSIUM; // Source: potassium-40
				parts[(ray id)].energy = 1.311 * UNIT_MEV;
				sim->part_change_type(i, x, y, PT_CALC); // producing Calcium-40
			}
			// anti-neutrinos are undetected
		}
		else
		{
			sim->part_change_type(i, x, y, PT_NBLE); // producing Argon-40
			parts[(ray id)].actual_type = ATOM_ARGON;
			// neutrinos are undetected
		}
	}
}

Element_RBDM::Isotope_87Rb::Update (UPDATE_FUNC_ARGS)
{
	if ((slowly decays))
	{
		int (ray id) = sim->create_part(-3, x, y, PT_ELEC); // beta decay
		if ((ray id) >= 0)
		{
			parts[(ray id)].source = PT_RBDM; 
			parts[(ray id)].actual_source = ATOM_RUBIDIUM; // Source: rubidium-87
			parts[(ray id)].energy = 0.283 * UNIT_MEV;
			sim->part_change_type(i, x, y, PT_STRN); // producing Strontium-87
		}
		// anti-neutrinos are undetected
	}
}

Element_RBDM::Isotope_135Cs::Update (UPDATE_FUNC_ARGS)
{
	if ((slowly decays))
	{
		int (ray id) = sim->create_part(-3, x, y, PT_ELEC); // beta decay
		// anti-neutrinos are undetected
		if ((ray id) >= 0)
		{
			parts[(ray id)].source = PT_RBDM; 
			parts[(ray id)].actual_source = ATOM_CAESIUM; // Source: rubidium-135
			parts[(ray id)].energy = 0.269 * UNIT_MEV;
			sim->part_change_type(i, x, y, PT_BARI); // producing Barium-135
		}
	}
}
#endif

Element_RBDM::~Element_RBDM() {}
