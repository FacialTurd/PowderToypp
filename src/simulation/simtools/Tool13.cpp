#include "ToolClasses.h"
#include "simulation/Simulation.h"
//#TPT-Directive ToolClass Tool_Tool13 TOOL_TOOL13 14
Tool_Tool13::Tool_Tool13()
{
	Identifier = "DEFAULT_TOOL_TOOL13";
	Name = "SETE";
	Colour = PIXPACK(0xEE22EE);
	Description = "Set emap";
}

int Tool_Tool13::Perform(Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	int t = sim->bmap[y][x];
	sim->set_emap(x/CELL, y/CELL);
	return t != sim->bmap[y][x];
}

Tool_Tool13::~Tool_Tool13() {}
