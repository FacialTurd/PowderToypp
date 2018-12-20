#ifndef SIMULATION_H
#define SIMULATION_H
#include <cstring>
#include <cstddef>
#include <vector>

#include "Config.h"
#include "Elements.h"
#include "SimulationData.h"
#include "Sign.h"
#include "Particle.h"
#include "Stickman.h"
#include "WallType.h"
#include "GOLMenu.h"
#include "MenuSection.h"
#include "elements/Element.h"

#define CHANNELS ((int)(MAX_TEMP-73)/100+2)

#ifndef PT_PINVIS
// #define PT_PINVIS 192
#endif

class Snapshot;
class SimTool;
class Brush;
class SimulationSample;
struct matrix2d;
struct vector2d;

class Simulation;
class Renderer;
class Gravity;
class Air;
class GameSave;
struct Simulation_move;

class Simulation
{
private:
	void _DelaySimulate(int);

public:

	Gravity * grav;
	Air * air;
	
	union _DWORD_union
	{
		int32_t dw;
		int16_t w[2];
		int8_t  b[4];
	};
	
	std::vector<sign> signs;
	Element elements[PT_NUM];
	//Element * elements;
	std::vector<SimTool*> tools;
	unsigned int * platent;
	wall_type wtypes[UI_WALLCOUNT];
	gol_menu gmenu[NGOL];
	int goltype[NGOL];
	int grule[NGOL+1][10];
	menu_section msections[SC_TOTAL];
	int temporary_sim_variable[10];

	int currentTick;
	int replaceModeSelected;
	int replaceModeFlags;
	bool isFromMyMod;
	bool isPrevFromMyMod;

	char can_move[PT_NUM][PT_NUM];
	int debug_currentParticle;
	int parts_lastActiveIndex;
	int pfree;
	int NUM_PARTS;
	bool elementRecount;
	int elementCount[PT_NUM];
	int ISWIRE;
	int ISWIRE2;
	bool force_stacking_check;
	int emp_decor;
	int emp_trigger_count;
	int emp2_trigger_count;
	bool etrd_count_valid;
	int etrd_life0_count;
	int lightningRecreate;
	int extraDelay;
	int delayEnd;
	int ineutcount;
	//Stickman
	playerst player;
	playerst player2;
	// playerstx player_stored;
	// playerstx player2_stored;
	playerst fighters[MAX_FIGHTERS]; //Defined in Stickman.h
	unsigned char fighcount; //Contains the number of fighters
	bool gravWallChanged;
	//Portals and Wifi
	Particle portalp[CHANNELS][8][80];
	int portal_rx[8];
	int portal_ry[8];
	int wireless[CHANNELS][2];
	int wireless2[CHANNELS][16];
	// int wireless2[128][2];
	//Gol sim
	int CGOL;
	int GSPEED;
	unsigned char gol[YRES][XRES];
	unsigned short gol2[YRES][XRES][9];
	unsigned char lloopsrule[8][8][8][8][8];
	int extraLoopsCA;
	int extraLoopsType;
	// int INVS_hardness_tmp; // unused
	//Air sim
	float (*vx)[XRES/CELL];
	float (*vy)[XRES/CELL];
	float (*pv)[XRES/CELL];
	float (*hv)[XRES/CELL];
	//Gravity sim
	float *gravx;//gravx[(YRES/CELL) * (XRES/CELL)];
	float *gravy;//gravy[(YRES/CELL) * (XRES/CELL)];
	float *gravp;//gravp[(YRES/CELL) * (XRES/CELL)];
	float *gravmap;//gravmap[(YRES/CELL) * (XRES/CELL)];
	//Walls
	unsigned char bmap[YRES/CELL][XRES/CELL];
	unsigned char emap[YRES/CELL][XRES/CELL];
	unsigned char bmap_brk[YRES/CELL][XRES/CELL];
	float fvx[YRES/CELL][XRES/CELL];
	float fvy[YRES/CELL][XRES/CELL];
	bool breakable_wall_recount;
	int breakable_wall_count;
	float sim_max_pressure;
	static int bltable[][2];
	//Particles
	Particle parts[NPART];
	// int part_references [NPART];
	int pmap[YRES][XRES];
	int photons[YRES][XRES];
	int pmap_count[YRES][XRES];
	//Simulation Settings
	int edgeMode;
	int gravityMode;
	int legacy_enable;
	int aheat_enable;
	int water_equal_test;
	int sys_pause;
	int SimExtraFunc;
	int Extra_FIGH_pause_check;
	int Extra_FIGH_pause;
	int framerender;
	int pretty_powder;
	int sandcolour;
	int sandcolour_frame;
	bool no_generating_BHOL;
#ifdef TPT_NEED_DLL_PLUGIN
	int dllexceptionflag;
#endif
	
	int  DIRCHInteractCount;
	int  DIRCHInteractSize;
	int* DIRCHInteractTable;

	int Load(GameSave * save, bool includePressure = true);
	int Load(int x, int y, GameSave * save, bool includePressure = true);
	GameSave * Save(bool includePressure = true);
	GameSave * Save(int x1, int y1, int x2, int y2, bool includePressure = true);
	void SaveSimOptions(GameSave * gameSave);
	SimulationSample GetSample(int x, int y);

	Snapshot * CreateSnapshot();
	void Restore(const Snapshot & snap);

	int is_blocking(int t, int x, int y);
	int is_boundary(int pt, int x, int y);
	int find_next_boundary(int pt, int *x, int *y, int dm, int *em);
	void photoelectric_effect(int nx, int ny);
	unsigned direction_to_map(float dx, float dy, int t);
	int do_move(int i, int x, int y, float nxf, float nyf);
	int do_move(Simulation_move & mov);
	int try_move(const Simulation_move & mov);
	int eval_move(int pt, int nx, int ny, unsigned *rr);
	void init_can_move();
	bool IsWallBlocking(int x, int y, int type);
	bool IsValidElement(int type) {
		return (type >= 0 && type < PT_NUM && elements[type].Enabled);
	}
	void create_cherenkov_photon(int pp);
	void create_gain_photon(int pp);

	void restrict_can_move(/* bool oldstate, bool newstate */);
	void kill_part(int i);
	bool FloodFillPmapCheck(int x, int y, int type);
	int flood_prop(int x, int y, size_t propoffset, PropertyValue propvalue, StructProperty::PropertyType proptype);
	int flood_water(int x, int y, int i, int originaly, int check);
	int FloodINST(int x, int y, int fullc, int cm);
	void detach(int i);
	bool part_change_type(int i, int x, int y, int t);
	//int InCurrentBrush(int i, int j, int rx, int ry);
	//int get_brush_flags();
	int create_part(int p, int x, int y, int t, int v = -1);
	void delete_part(int x, int y);
	void get_sign_pos(int i, int *x0, int *y0, int *w, int *h);
	int is_wire(int x, int y);
	int is_wire_off(int x, int y);
	void set_emap(int x, int y);
	int parts_avg(int ci, int ni, int t);
	void create_arc(int sx, int sy, int dx, int dy, int midpoints, int variance, int type, int flags);
	void UpdateParticles(int start, int end);
	void SimulateGoL();
	void SimulateLLoops();
	void RecalcFreeParticles(bool do_life_dec);
	void CheckStacking();
	void BeforeSim();
	void AfterSim();
	void rotate_area(int area_x, int area_y, int area_w, int area_h, int invert);
	void clear_area(int area_x, int area_y, int area_w, int area_h);

	void SetEdgeMode(int newEdgeMode);

	//Drawing Deco
	void ApplyDecoration(int x, int y, int colR, int colG, int colB, int colA, int mode);
	void ApplyDecorationPoint(int x, int y, int colR, int colG, int colB, int colA, int mode, Brush * cBrush = NULL);
	void ApplyDecorationLine(int x1, int y1, int x2, int y2, int colR, int colG, int colB, int colA, int mode, Brush * cBrush = NULL);
	void ApplyDecorationBox(int x1, int y1, int x2, int y2, int colR, int colG, int colB, int colA, int mode);
	bool ColorCompare(Renderer *ren, int x, int y, int replaceR, int replaceG, int replaceB);
	void ApplyDecorationFill(Renderer *ren, int x, int y, int colR, int colG, int colB, int colA, int replaceR, int replaceG, int replaceB);

	//Drawing Tools like HEAT, AIR, and GRAV
	int Tool(int x, int y, int tool, int brushX, int brushY, float strength = 1.0f);
	int ToolBrush(int x, int y, int tool, Brush * cBrush, float strength = 1.0f);
	void ToolLine(int x1, int y1, int x2, int y2, int tool, Brush * cBrush, float strength = 1.0f);
	void ToolBox(int x1, int y1, int x2, int y2, int tool, float strength = 1.0f);
	
	//Drawing Walls
	int CreateWall_with_brk(int x, int y, int wall);
	int CreateWalls(int x, int y, int rx, int ry, int wall, Brush * cBrush = NULL);
	void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall, Brush * cBrush = NULL);
	void CreateWallBox(int x1, int y1, int x2, int y2, int wall);
	int FloodWalls(int x, int y, int wall, int bm);
	int _GetBreakableWallCount();

	//Drawing Particles
	int CreateParts(int positionX, int positionY, int c, Brush * cBrush, int flags = -1);
	int CreateParts(int x, int y, int rx, int ry, int c, int flags = -1);
	int CreatePartFlags(int x, int y, int c, int flags);
	void CreateLine(int x1, int y1, int x2, int y2, int c, Brush * cBrush, int flags = -1);
	void CreateLine(int x1, int y1, int x2, int y2, int c);
	void CreateBox(int x1, int y1, int x2, int y2, int c, int flags = -1);
	int FloodParts(int x, int y, int c, int cm, int flags = -1);
	void pmap_add(int i, int x, int y, int t);
	bool pmap_clearif(int & r, unsigned int i);
	int & pmap_get(int x, int y);
	void pmap_heatconduct(int r, float temp);
	void pmap_heatconduct(int x, int y, float temp);
	void pmap_remove(unsigned int i, int x, int y);
	int parts_allocate()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		if (i>parts_lastActiveIndex)
			parts_lastActiveIndex = i;
		return i;
	}
	void parts_deallocate(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
	}

	void GetGravityField(int x, int y, float particleGrav, float newtonGrav, float & pGravX, float & pGravY);
	int GetHeatConduct(int i, int t);
	int GetParticleType(std::string type);

	void orbitalparts_get(int block1, int block2, int resblock1[], int resblock2[]);
	void orbitalparts_set(int *block1, int *block2, int resblock1[], int resblock2[]);
	int get_wavelength_bin(int *wm);
	int get_normal(int pt, int x, int y, float dx, float dy, float *nx, float *ny);
	int get_normal_interp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);
	void clear_sim();
	void
#if defined(_WIN32) && !defined(_WIN64) && defined(TPT_NEED_DLL_PLUGIN)
	__fastcall
#endif
	_check_neut_base0(int*, char*, bool = true);
	Simulation();
	~Simulation();

	bool InBounds(int x, int y)
	{
		return (x>=0 && y>=0 && x<XRES && y<YRES);
	}

	// These don't really belong anywhere at the moment, so go here for loop edge mode
	static int remainder_p(int x, int y)
	{
		return (x % y) + (x>=0 ? 0 : y);
	}
	static float remainder_p(float x, float y)
	{
		return std::fmod(x, y) + (x>=0 ? 0 : y);
	}
};

#ifdef _MSC_VER
unsigned msvc_ctz(unsigned a);
unsigned msvc_clz(unsigned a);
#endif

#endif /* SIMULATION_H */
