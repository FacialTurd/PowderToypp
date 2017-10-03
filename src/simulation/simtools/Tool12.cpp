#include "ToolClasses.h"
#include "simulation/Simulation.h"
//#TPT-Directive ToolClass Tool_Tool12 TOOL_TOOL12 12
Tool_Tool12::Tool_Tool12()
{
	Identifier = "DEFAULT_TOOL_TOOL12";
	Name = "NSC2";
	Colour = PIXPACK(0xEE22EE);
	Description = "Second NSCN, can deactivate powered materials.";
}

int Tool_Tool12::Perform(Simulation * sim, Particle * cpart, int x, int y, float strength)
{
	int id = sim->create_part(-1, x, y, PT_NSCN);
	if (id >= 0)
	{
		sim->parts[id].tmp = 1;
		return 1;
	}
	return 0;
}

Tool_Tool12::~Tool_Tool12() {}
