#ifndef ELEMENTS_H_
#define ELEMENTS_H_

//#include "Config.h"
//#include "Simulation.h"

#define R_TEMP 22
#define MAX_TEMP 9999
#define MIN_TEMP 0
#define O_MAX_TEMP 3500
#define O_MIN_TEMP -273

#define TYPE_PART			0x00001  //1 Powders
#define TYPE_LIQUID			0x00002  //2 Liquids
#define TYPE_SOLID			0x00004  //4 Solids
#define TYPE_GAS			0x00008  //8 Gases (Includes plasma)
#define TYPE_ENERGY			0x00010  //16 Energy (Thunder, Light, Neutrons etc.)
#define STATE_FLAGS			0x0001F
#define PROP_CONDUCTS		0x00020    //   32 Conducts electricity
#define PROP_BLACK			0x00040    //   64 Absorbs Photons (not currently implemented or used, a photwl attribute might be better)
#define PROP_NEUTPENETRATE	0x00080    //  128 Penetrated by neutrons
#define PROP_NEUTABSORB		0x00100    //  256 Absorbs neutrons, reflect is default
#define PROP_NEUTPASS		0x00200    //  512 Neutrons pass through, such as with glass
#define PROP_DEADLY			0x00400    // 1024 Is deadly for stickman
#define PROP_HOT_GLOW		0x00800    // 2048 Hot Metal Glow
#define PROP_LIFE			0x01000    // 4096 Is a GoL type
#define PROP_RADIOACTIVE	0x02000    // 8192 Radioactive
#define PROP_LIFE_DEC		0x04000    // 2^14 Life decreases by one every frame if > zero
#define PROP_LIFE_KILL		0x08000    // 2^15 Kill when life value is <= zero
#define PROP_LIFE_KILL_DEC	0x10000    // 2^16 Kill when life value is decremented to <= zero
#define PROP_SPARKSETTLE	0x20000    // 2^17 Allow Sparks/Embers to settle
#define PROP_NOAMBHEAT		0x40000    // 2^18 Don't transfer or receive heat from ambient heat.
#define PROP_DRAWONCTYPE	0x80000    // 2^19 Set its ctype to another element if the element is drawn upon it (like what CLNE does)
#define PROP_NOCTYPEDRAW	0x100000   // 2^20 When this element is drawn upon with, do not set ctype (like BCLN for CLNE)
// 2^21 reserved
#define PROP_NOSLOWDOWN		0x02000000   // 2^25 Photons (PHOT) without slowed down
#define PROP_TRANSPARENT	0x04000000   // 2^26 Photons (PHOT) pass through
#define PROP_INSULATED		0x08000000   // 2^27 Insulated wires
#define PROP_CONDUCTS_SPEC	0x10000000   // 2^28 Special conducts (like INST)
#define PROP_NO_NBHL_GEN	0x20000000   // 2^29 

// second properties flag:
#define PROP_DEBUG_USE_TMP2		0x00000001	//   1 Use .tmp2 show it in the debug HUD
#define PROP_ELEC_HEATING		0x00000002  //   2 Electricity heating effect
#define PROP_NOWAVELENGTHS		0x00000004  //   4 Energy particle without wavelengths
#define PROP_CTYPE_INTG			0x00000008	//   8 Show .ctype as integer
#define PROP_CTYPE_WAVEL		0x00000010	//  16 Show .ctype as wavelengths
#define PROP_CTYPE_SPEC			0x00000020	//  32 Special case for .ctype
#define PROP_ENERGY_PART		0x00000040	//  64 VIBR absorb-able particle
#define PROP_DEBUG_HIDE_TMP		0x00000080	// 128 .tmp is hidden from the debug HUD
#define PROP_NEUTRONS_LIKE		0x00000100	// 256 neutrons-like element
#define PROP_UNBREAKABLECLONE	0x00004000  // 2^14
#define PROP_NODESTRUCT			0x00008000  // 2^15 Indestructible property
#define PROP_CLONE				0x00010000  // 2^16 Makes elements clone things that touch it
#define PROP_ALLOWS_WALL		0x00020000	// 2^17
// #define PROP_DRAWONCTYPE		0x00080000
// #define PROP_NOSLOWDOWN		0x02000000
#define PROP_INVISIBLE			0x04000000  // 2^26 Invisible to particles like INVS and FILT
#define PROP_UNLIMSTACKING		0x08000000  // 2^27 Currently no limit on stacking

#define FLAG_STAGNANT	0x1
#define FLAG_SKIPMOVE   0x2  // skip movement for one frame, only implemented for PHOT
#define FLAG_WATEREQUAL 0x4  // if a liquid was already checked during equalization
#define FLAG_MOVABLE    0x8  // compatibility with old saves (moving SPNG), only applies to SPNG
#define FLAG_PHOTDECO   0x8  // compatibility with old saves (decorated photons), only applies to PHOT. Having the same value as FLAG_MOVABLE is fine because they apply to different elements, and this saves space for future flags,
#define FLAG_SKIPCREATE 0x10 // skip creating PROT, only implemented for "E186"


#define PMAPBITS 8

#define PMAPMASK ((1<<PMAPBITS)-1)
#define part_ID(r) ((r)>>PMAPBITS)
#define TYP(r) ((r)&PMAPMASK)
#define PMAP(id, typ) (PMAPID(id) | TYP(typ))
#define PMAPID(id) ((id)<<PMAPBITS)

#define partsi(id) parts[part_ID(r)]
#define CHECK_EL_SPRK(r, t) \
	(TYP(r) == t || TYP(r) == PT_SPRK && partsi(r).ctype == t)
#define CHECK_EL_SPRKT(r, t) \
	(TYP((r) == PT_SPRK && partsi(r).ctype == (t))
#define CHECK_EL_SPRKL3(r) \
	(TYP(r) == PT_SPRK && partsi(r).life == 3)
#define CHECK_EL_SPRKL3T(r, t) \
	(CHECK_EL_SPRKL3(r) && partsi(r).ctype == (t))
#define CHECK_EXTEL(r, t) \
	(TYP(r) == ELEM_MULTIPP && partsi(r).life == (t))
#define CHECK_EL_INSL(t) ((t) == PT_INSL || (t) == PT_INDI)
#define PAVG_INSL(r) sim->parts_avg(i, ID(r), PT_INSL)

#define ELEMPROP(t) sim->elements[t].Properties
#define ELEMPROP2(t) sim->elements[t].Properties2
#define ELEMPROPT(t) ELEMPROP(TYP(t))
#define ELEMPROPP(r, p) ELEMPROP(partsi(r).p)
#define ELEMPROPW(id, op) (parts[id].flags op (FLAG_SKIPMOVE))


#define UPDATE_FUNC_ARGS Simulation* sim, int i, int x, int y, int surround_space, int nt, Particle *parts, int pmap[YRES][XRES]
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt, parts, pmap

#define GRAPHICS_FUNC_ARGS Renderer * ren, Particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS ren, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb

#define BOUNDS_CHECK true

#define OLD_PT_WIND 147

//#define PT_NUM  161
#define PT_NUM	256

struct playerst;

#include "ElementClasses.h"


#endif /* ELEMENTS_H_ */