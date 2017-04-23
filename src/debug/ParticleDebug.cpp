#include <sstream>
#include "ParticleDebug.h"
#include "gui/interface/Engine.h"
#include "gui/game/GameView.h"
#include "gui/game/GameController.h"

ParticleDebug::ParticleDebug(unsigned int id, Simulation * sim, GameModel * model):
	DebugInfo(id),
	sim(sim),
	model(model)
{

}

void ParticleDebug::Debug(int mode, int x, int y)
{
	int debug_currentParticle = sim->debug_currentParticle;
	int i;
	std::stringstream logmessage;

	if (sim->debug_currentParticle == 0 && sim->debug_needReloadParticleOrder)
	{
		model->ReloadParticleOrder();
		sim->debug_needReloadParticleOrder = false;
	}
	
	if (mode == 0)
	{
		if (!sim->NUM_PARTS)
			return;

		if (sim->debug_currentParticle == 0)
		{
			sim->framerender = 1;
			sim->BeforeSim();
			sim->framerender = 0;
		}
		int rangeStart = debug_currentParticle;
		do
		{
			i = rangeStart;
			while (i < NPART && !sim->parts[i].type)
				i++;

			sim->UpdateParticles(rangeStart, i);
			rangeStart = i+1;
		}
		while(i < NPART && !sim->debug_interestingChangeOccurred);

		if (i == NPART)
		{
			logmessage << "End of particles reached, updated sim";
			model->Log(logmessage.str(), false);
		}

		//else
		//	logmessage << "Updated particles #" << debug_currentParticle << " through #" << i;
	}
	else{
		if (sim->debug_currentParticle == 0)
		{
			sim->framerender = 1;
			sim->BeforeSim();
			sim->framerender = 0;
		}

		if (mode == 1)
		{
			if (x < 0 || x >= XRES || y < 0 || y >= YRES || !sim->pmap[y][x] || (i = (sim->pmap[y][x]>>8)) < debug_currentParticle)
			{
				i = NPART;
				logmessage << "Updated particles from #" << debug_currentParticle << " to end, updated sim";
			}
			else
				logmessage << "Updated particles #" << debug_currentParticle << " through #" << i;
			model->Log(logmessage.str(), false);
		}

		sim->UpdateParticles(debug_currentParticle, i);
	}

	if (i < NPART-1)
		sim->debug_currentParticle = i+1;
	else
	{
		sim->AfterSim();
		sim->debug_currentParticle = 0;
	}
}

bool ParticleDebug::KeyPress(int key, Uint16 character, bool shift, bool ctrl, bool alt, ui::Point currentMouse)
{
	if (key == 'f')
	{
		model->SetPaused(1);
		if (alt)
		{
			Debug(0, 0, 0);
		}
		else if (shift)
		{
			ui::Point mouse = currentMouse;
			if (mouse.X >= XRES)
				mouse.X = XRES-1;
			else if (mouse.X < 0)
				mouse.X = 0;
			if (mouse.Y >= YRES)
				mouse.Y = YRES-1;
			else if (mouse.Y < 0)
				mouse.Y = 0;

			mouse = model->AdjustZoomCoords(mouse);
			Debug(1, mouse.X, mouse.Y);
		}
		else
		{
			if (sim->debug_currentParticle > 0)
			{
				sim->CompleteDebugUpdateParticles();

				std::stringstream logmessage;
				logmessage << "Updated particles from #" << sim->debug_currentParticle << " to end, updated sim";
				model->Log(logmessage.str(), false);
			}
			else
			{
				model->FrameStep(1);
			}
		}
		return false;
	}
	return true;
}

ParticleDebug::~ParticleDebug()
{

}
