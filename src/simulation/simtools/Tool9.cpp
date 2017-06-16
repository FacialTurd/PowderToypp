#include "ToolClasses.h"
#include "simulation/Simulation.h"
//#TPT-Directive ToolClass Tool_Tool9 TOOL_TOOL9 9
Tool_Tool9::Tool_Tool9()
{
	Identifier = "DEFAULT_TOOL_TOOL9";
	Name = "MSTG";
	Colour = PIXPACK(0xEE22EE);
	Description = "Mandelbrot set (image) generator.";
}

int Tool_Tool9::Perform(Simulation * sim, Particle * cpart, int x, int y, float strength)
{
	double multipler = 25.0 / 4096.0 * ((YRES - CELL * 2) / 376.0);
	double cx = -0.764407332533302 + (x - (XRES / 2)) * multipler;
	double cy =  0.0 + (y - (YRES / 2)) * multipler;
	double zx = cx, zy = cy;
	double zx2 = zx * zx, zy2 = zy * zy;
	// formula: z <-> z**2 + c
	// r' = (r+i) * (r-i) + c
	// i' = 2 * r*i + c
	for (int iter = 0; iter < 511; iter++)
	{
		if (zx2 + zy2 > 4.0) // if cabs (z) > 2.0 then render
		{
			int npart = sim->create_part(-1, x, y, ELEM_MULTIPP, 18);
			if (npart >= 0)
			{
				// ctype = red, tmp = green, tmp2 = blue
				iter *= 2;
				sim->parts[npart].ctype = iter > 255 ? (iter < 510 ? iter - 255 : 255) : 0;
				sim->parts[npart].tmp = iter < 255 ? iter : (iter > 765 ? (1020 - iter) : 255);
				sim->parts[npart].tmp2 = iter > 510 ? (iter < 765 ? (765 - iter) : 0) : 255;
			}
			return 1;
		}
		zy = 2 * zx * zy + cy;
		zx = zx2 - zy2 + cx;
		zx2 = zx * zx;
		zy2 = zy * zy;
	}
	return 0;
}

Tool_Tool9::~Tool_Tool9() {}
