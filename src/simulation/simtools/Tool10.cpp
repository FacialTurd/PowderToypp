#include "ToolClasses.h"
#include "simulation/Simulation.h"
//#TPT-Directive ToolClass Tool_Tool10 TOOL_TOOL10 11
Tool_Tool10::Tool_Tool10()
{
	Identifier = "DEFAULT_TOOL_TOOL10";
	Name = "CNDT";
	Colour = PIXPACK(0xEE22EE);
	Description = "Conductor, virtually indestructible.";
}

int Tool_Tool10::Perform(Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	int id = sim->create_part(-1, x, y, PT_TESC, 0);
	return (id >= 0 ? 1 : 0);
}

Tool_Tool10::~Tool_Tool10() {}
