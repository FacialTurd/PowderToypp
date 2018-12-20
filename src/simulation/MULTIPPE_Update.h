#ifndef __MULTIPPE_Update_H__
#define __MULTIPPE_Update_H__

#include "simulation/Elements.h"

#define NUM_COLOR_SPC 41

#ifdef LUACONSOLE
#include "lua/LuaScriptInterface.h"
#include "lua/LuaScriptHelper.h"

class LuaScriptInterface;
#endif
class Simulation;
class Renderer;
struct Particle;
struct Simulation_move;

namespace MULTIPPE_Update
{
	extern pixel ColorsSpc [NUM_COLOR_SPC];
	extern Renderer * ren_;
	int update(UPDATE_FUNC_ARGS);
	int graphics(GRAPHICS_FUNC_ARGS);
	void do_breakpoint();
	void InsertText(Simulation *sim, int i, int x, int y, int ix, int iy);
	// static int AddCharacter(Simulation *sim, int x, int y, int c, int rgb);
	bool isAcceptedConductor (Simulation* sim, int r);
	bool isAcceptedConductor_i (Simulation* sim, int r);
	static void conductTo (Simulation* sim, int r, int x, int y, Particle *parts) // Inline or macro?
	{
		if (!partsi(r).life)
		{
			partsi(r).ctype = TYP(r);
			sim->part_change_type(part_ID(r), x, y, PT_SPRK);
			partsi(r).life = 4;
		}
	}
	static void conductToSWCH (Simulation* sim, int r, int x, int y, Particle *parts) // Inline or macro?
	{
		if (partsi(r).life == 10)
		{
			partsi(r).ctype = TYP(r);
			sim->part_change_type(part_ID(r), x, y, PT_SPRK);
			partsi(r).life = 4;
		}
	}
	static bool BreakWallTest (Simulation* sim, int x, int y, bool a) // Inline or macro?
	{
		int xb = x/CELL, yb = y/CELL;
		if (xb >= 0 && xb < XRES/CELL && yb >= 0 && yb < YRES/CELL &&
			sim->wtypes[ sim->bmap[yb][xb] ].PressureTransition >= 0)
		{
			if (a) sim->bmap_brk[yb][xb] = true;
			return true;
		}
		return false;
	}
// #ifdef LUACONSOLE
//	static void luacall (LuaScriptInterface* ci, int t, int i, int x, int y);
// #endif
	
	// struct ???
	// static bool SetDecoration(bool decorationState); // file used: src/gui/game/GameModel.cpp
	// static bool GetDecoration();
};
#endif
