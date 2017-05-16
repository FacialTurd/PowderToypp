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
	char buff[256];
	char signText[256];
	sprintf(signText, "%s", text.substr(0, 255).c_str());

	if(signText[0] && signText[0] == '{')
	{
		if (!strcmp(signText,"{p}"))
		{
			float pressure = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				pressure = sim->pv[y/CELL][x/CELL];
			sprintf(buff, "Pressure: %3.2f", pressure);  //...pressure
		}
		else if (!strcmp(signText,"{aheat}"))
		{
			float aheat = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				aheat = sim->hv[y/CELL][x/CELL];
			sprintf(buff, "%3.2f", aheat);
		}
		else if (!strcmp(signText,"{t}"))
		{
			if (x>=0 && x<XRES && y>=0 && y<YRES && sim->pmap[y][x])
				sprintf(buff, "Temp: %4.2f", sim->parts[sim->pmap[y][x]>>8].temp-273.15);  //...temperature
			else
				sprintf(buff, "Temp: 0.00");  //...temperature
		}
		else if (signText[1] == 'v') // air velocity
		{
			if (signText[3] == '}' && !signText[4])
			{
				float velocity = 0.0f;
				if (signText[2] == 'x')
				{
					velocity = sim->vx[y/CELL][x/CELL];
					sprintf(buff, "%3.2f", velocity);
				}
				else if (signText[2] == 'y')
				{
					velocity = sim->vy[y/CELL][x/CELL];
					sprintf(buff, "%3.2f", velocity);
				}
				else
				{
					strcpy(buff, signText);
				}
			}
			else
				strcpy(buff, signText);
		}
		else if (signText[1] == '.') // element
		{
			int r = 0, num1, ptr1 = 2, ptr2 = 0, ptr3 = 0;
			char cchar;
			char dchar = 0;
			char structtype = 0;
			// structtype = 0: int
			// structtype = 1: float
			// structtype = 2: particle type
			char matched1 [256];
			const char* matched1ptr = matched1;
			
			for (;;) // infinite loop
			{
				cchar = signText[ptr1++];
				if (!cchar) // found end of string
				{
					strcpy(buff, signText);
					return std::string(buff);
				}
				if (cchar == '}') // found "}"
				{
					if (signText[ptr1]) // if isn't end of string
					{
						strcpy(buff, signText);
						return std::string(buff);
					}
					matched1[ptr2++] = '\0';
					break;
				}
				if (cchar == ':' && !ptr3) // found ":"
				{
					ptr3 = ptr2 + 1;
					cchar = '\0';
				}
				matched1[ptr2++] = cchar;
			}
			if (x>=0 && x<XRES && y>=0 && y<YRES)
			{
				r = sim->photons[y][x];
				if (!r)
					r = sim->pmap[y][x];
			}
			if (r)
			{
				if (!strcmp(matched1, "i"))
					num1 = r >> 8;
				else if (!strcmp(matched1, "type"))
				{
					structtype = 2;
					num1 = r & 0xFF;
				}
				else if (!strcmp(matched1, "life"))
					num1 = sim->parts[r>>8].life;
				else if (!strcmp(matched1, "ctype"))
				{
					structtype = 2;
					num1 = sim->parts[r>>8].ctype;
				}
				else if (!strcmp(matched1, "temp"))
				{
					structtype = 1;
					num1 = * (int*) & (sim->parts[r>>8].temp);
				}
				else if (!strcmp(matched1, "tmp"))
					num1 = sim->parts[r>>8].tmp;
				else if (!strcmp(matched1, "tmp2"))
					num1 = sim->parts[r>>8].tmp2;
				else if (!strcmp(matched1, "tmp3"))
					num1 = sim->parts[r>>8].tmp3;
				else if (!strcmp(matched1, "tmp4"))
					num1 = sim->parts[r>>8].tmp4;
				/*
				else if (!strcmp(matched1, "vrad"))
				{
					float veloc_x = sim->parts[r>>8].vx;
					float veloc_y = sim->parts[r>>8].vy;
					float velocity = hypotf(veloc_x, veloc_y);
					num1 = * (int*) & velocity;
				}
				*/
				else if (!strcmp(matched1, "vx"))
				{
					structtype = 1;
					num1 = * (int*) & (sim->parts[r>>8].vx);
				}
				else if (!strcmp(matched1, "vy"))
				{
					structtype = 1;
					num1 = * (int*) & (sim->parts[r>>8].vy);
				}
				else if (!strcmp(matched1, "pavg0"))
				{
					structtype = 1;
					num1 = * (int*) & (sim->parts[r>>8].pavg[0]);
				}
				else if (!strcmp(matched1, "pavg1"))
				{
					structtype = 1;
					num1 = * (int*) & (sim->parts[r>>8].pavg[1]);
				}
				else
				{
					strcpy(buff, signText);
					return std::string(buff);
				}

				if (ptr3)
				{
					matched1ptr += ptr3;
					if (!strcmp(matched1ptr, "f"))
						structtype = 1;
					else if (!strcmp(matched1ptr, "t") || !strcmp(matched1ptr, "xt"))
					{
						structtype = (*matched1ptr == 'x') ? 3 : 2;
					}
					else if (!strcmp(matched1ptr, "x"))
						structtype = 4;
					else if (!strcmp(matched1ptr, "X"))
						structtype = 5;
					else
						structtype = 0;
				}
				
				switch (structtype)
				{
					case 0: sprintf(buff, "%d", num1); break;
					case 1: sprintf(buff, "%3.2f", * (float*) & num1); break;
					case 2:
					case 3:
						if ( structtype == 3 )
						{
							int num1h = (num1 >> 8); // .ctype?
							if ( (num1 & 0xFF) == PT_LIFE && num1h >= 0 && num1h < NGOL )
							{
								sprintf(buff, "%s", sim->gmenu[num1h].name);
								overridden1 = true;
							}
						}
						if (!overridden1)
						{
							sprintf(buff, "%s", sim->elements[num1&0xFF].Name);
						}
					break;
					case 4: sprintf(buff, "0x%08x", num1); break;
					case 5: sprintf(buff, "0x%08X", num1); break;
				}
			}
			else
				strcpy(buff, "");
		}
		else
		{
			int pos = splitsign(signText);
			if (pos)
			{
				strcpy(buff, signText+pos+1);
				buff[strlen(signText)-pos-2]=0;
			}
			else
				strcpy(buff, signText);
		}
	}
	else
	{
		strcpy(buff, signText);
	}

	return std::string(buff);
}

void sign::pos(std::string signText, int & x0, int & y0, int & w, int & h)
{
	w = Graphics::textwidth(signText.c_str()) + 5;
	h = 15;
	x0 = (ju == Right) ? x - w :
		  (ju == Left) ? x : x - w/2;
	y0 = (y > 18) ? y - 18 : y + 4;
}

int sign::splitsign(const char* str, char * type)
{
	if (str[0]=='{' && (str[1]=='c' || str[1]=='t' || str[1]=='b' || str[1]=='s'))
	{
		const char* p = str+2;
		// signs with text arguments
		if (str[1] == 's')
		{
			if (str[2]==':')
			{
				p = str+4;
				while (*p && *p!='|')
					p++;
			}
			else
				return 0;
		}
		// signs with number arguments
		if (str[1] == 'c' || str[1] == 't')
		{
			if (str[2]==':' && str[3]>='0' && str[3]<='9')
			{
				p = str+4;
				while (*p>='0' && *p<='9')
					p++;
			}
			else
				return 0;
		}

		if (*p=='|')
		{
			int r = p-str;
			while (*p)
				p++;
			if (p[-1] == '}')
			{
				if (type)
					*type = str[1];
				return r;
			}
		}
	}
	return 0;
}
