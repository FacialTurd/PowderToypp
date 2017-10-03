#include "ToolClasses.h"
#include "simulation/Simulation.h"
//#TPT-Directive ToolClass Tool_Tool11 TOOL_TOOL11 11
Tool_Tool11::Tool_Tool11()
{
	Identifier = "DEFAULT_TOOL_TOOL11";
	Name = "PSC2";
	Colour = PIXPACK(0xEE22EE);
	Description = "Second PSCN, can activate powered materials.";
}

int Tool_Tool11::Perform(Simulation * sim, Particle * cpart, int x, int y, float strength)
{
	int id = sim->create_part(-1, x, y, PT_PSCN);
	if (id >= 0)
	{
		sim->parts[id].tmp = 1;
		return 1;
	}
	return 0;
}

Tool_Tool11::~Tool_Tool11() {}
