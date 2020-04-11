#ifndef STICKMAN_H_
#define STICKMAN_H_

#define MAX_FIGHTERS 100

#define _STKM_FLAG_EMT		0x01
#define _STKM_FLAG_EFIGH	0x02
#define _STKM_FLAG_EPROP	0x03
#define _STKM_FLAG_EPROPSH	0
#define _STKM_FLAG_OVR		0x04
#define _STKM_FLAG_VAC		0x08
#define _STKM_FLAG_SUSPEND	0x10

#define _STKM_CMD_FLAG_NOSPAWN	0x01

struct playerst
{
	char comm;           //command cell
	char pcomm;          //previous command
	int elem;            //element power
	int pelem;           //previous element power
	float legs_curr[8];  //legs' positions
	float legs_prev[8];  //legs' positions
	float accs[8];       //accelerations
	char spwn;           //if stick man was spawned
	int __flags;         //stick man's extra flags
	// int __flags2;
	unsigned int frames; //frames since last particle spawn - used when spawning LIGH
	bool rocketBoots;
	bool fan;
	// int action;
	int spawnID;         //id of the SPWN particle that spawns it
	int rocketBootSpawn;
	int parentStickman;
	int firstChild;
	int prevStickman;
	int nextStickman;
	int lastChild;
	int self_ID;
	int underp;          // id of the other particle
	int commf;           // temporary command flags
};

/*
struct playerst_saved
{
	int life, ctype;
	float x, y, vx, vy;
	struct playerst _saved;
}
*/

#endif
