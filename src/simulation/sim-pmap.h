#pragma once

void Simulation::pmap_add(int i, int x, int y, int t)
{
	// NB: all arguments are assumed to be within bounds
	if (elements[t].Properties & TYPE_ENERGY)
		photons[y][x] = PMAP(i, t);
	else if ((!pmap[y][x] || !(elements[t].Properties2 & PROP_INVISIBLE))) // fixed
		pmap[y][x] = PMAP(i, t);
}

bool Simulation::pmap_clearif(int & r, unsigned int i)
{
	bool ret = r && (ID((unsigned int)r) == i);
	if (ret)
		r = 0;
	return ret;
}

int & Simulation::pmap_get(int x, int y)
{
	int &r = pmap[y][x];
	if (TYP(r) == PT_PINVIS)
		return partsi(r).tmp4;
	return r;
}

void Simulation::pmap_heatconduct(int r, float temp)
{
	int ri = ID((unsigned int)r), rt = TYP(r);
	if (GetHeatConduct(ri, rt))
		parts[ri].temp = temp;
}

void Simulation::pmap_heatconduct(int x, int y, float temp)
{
	int r = pmap[y][x];
	if (r)
		pmap_heatconduct(r, temp);
	r = photons[y][x];
	if (r)
		pmap_heatconduct(r, temp);
}

void Simulation::pmap_remove(unsigned int i, int x, int y)
{
	// NB: all arguments are assumed to be within bounds
	if (pmap_clearif(pmap[y][x], i) || (TYP(pmap[y][x]) == PT_PINVIS && pmap_clearif(partsi(pmap[y][x]).tmp4, i)))
		return;
	if (ID((unsigned int)(photons[y][x])) == i)
		photons[y][x] = 0;
}
