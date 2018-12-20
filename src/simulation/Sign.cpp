#include <iomanip>
#include <sstream>
#include "Sign.h"
#include "graphics/Graphics.h"
#include "simulation/Simulation.h"

sign::sign(std::string text_, int x_, int y_, Justification justification_):
	x(x_),
	y(y_),
	ju(justification_),
	text(text_)
{
}

std::string sign::getText(Simulation *sim)
{
	std::stringstream signTextNew;
	if (text[0] && text[0] == '{')
	{
		if (text == "{p}")
		{
			float pressure = 0.0f;
			if (x >= 0 && x < XRES && y >= 0 && y < YRES)
				pressure = sim->pv[y/CELL][x/CELL];
			signTextNew << std::fixed << std::showpoint << std::setprecision(2) << "Pressure: " << pressure;
		}
		else if (text == "{aheat}")
		{
			float aheat = 0.0f;
			if (x >= 0 && x < XRES && y >= 0 && y < YRES)
				aheat = sim->hv[y/CELL][x/CELL];
			signTextNew << std::fixed << std::showpoint << std::setprecision(2) << aheat-273.15f;
		}
		else if (text == "{t}")
		{
			if (x >= 0 && x < XRES && y >= 0 && y < YRES && sim->pmap[y][x])
				signTextNew << std::fixed << std::showpoint << std::setprecision(2) << "Temp: " << sim->parts[ID(sim->pmap[y][x])].temp-273.15f;
			else
				signTextNew << "Temp: 0.00";
		}
		else
		{
			int pos, r = 0;
			bool extraArgFound = true, propFound = false;

			if (text[1] == '.' && text[text.length() - 1] == '}')
			{
				pos = text.find(':', 2);
				if (pos == std::string::npos)
					pos = text.length() - 1, extraArgFound = false;
				std::string subtext = text.substr(2, pos-2);

				if (x >= 0 && x < XRES && y >= 0 && y < YRES)
					r = sim->pmap[y][x];
				
				StructProperty prop;
				std::vector<StructProperty> props = Particle::GetProperties();
				
				for(std::vector<StructProperty>::iterator iter = props.begin(), end = props.end(); iter != end; ++iter)
				{
					if((*iter).Name == subtext)
					{
						prop = *iter;
						propFound = true;
						break;
					}
				}

				if (subtext == "i")
				{
					if (r)
						signTextNew << ID(r);
					propFound = true;
				}
				else if (propFound && TYP(r))
				{
					signTextNew << *((int*)(((unsigned char*)&sim->parts[ID(r)])+(prop.Offset)));
				}
			}
			if (!propFound)
			{
				pos = splitsign(text);
				if (pos)
					signTextNew << text.substr(pos+1, text.length()-pos-2);
				else
					signTextNew << text;
			}
		}
	}
	else
	{
		signTextNew << text;
	}
// lastres:
	return signTextNew.str();
}

void sign::pos(std::string signText, int & x0, int & y0, int & w, int & h)
{
	w = Graphics::textwidth(signText.c_str()) + 5;
	h = 15;
	x0 = (ju == Right) ? x - w :
		  (ju == Left) ? x : x - w/2;
	y0 = (y > 18) ? y - 18 : y + 4;
}

int sign::splitsign(std::string str, char * type)
{
	if (str[0] == '{' && (str[1] == 'c' || str[1] == 't' || str[1] == 'b' || str[1] == 's'))
	{
		size_t strIndex = 2;
		// Signs with text arguments
		if (str[1] == 's')
		{
			if (str[2] == ':')
			{
				strIndex = 3;
				while (strIndex < str.length() && str[strIndex] != '|')
					strIndex++;
			}
			else
				return 0;
		}
		// Signs with number arguments
		if (str[1] == 'c' || str[1] == 't')
		{
			if (str[2] == ':' && str[3] >= '0' && str[3] <= '9')
			{
				strIndex = 4;
				while (str[strIndex] >= '0' && str[strIndex] <= '9')
					strIndex++;
			}
			else
				return 0;
		}

		if (str[strIndex] == '|')
		{
			if (str[str.length() - 1] == '}')
			{
				if (type)
					*type = str[1];
				return strIndex;
			}
		}
	}
	return 0;
}
