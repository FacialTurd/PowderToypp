#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <cmath>
#include "Config.h"
#include "Misc.h"
#include "icondoc.h"
#include "common/tpt-minmax.h"

//Signum function
int isign(float i) //TODO: INline or macro
{
	if (i<0)
		return -1;
	if (i>0)
		return 1;
	return 0;
}

unsigned clamp_flt(float f, float min, float max) //TODO: Also inline/macro
{
	if (f<min)
		return 0;
	if (f>max)
		return 255;
	return (int)(255.0f*(f-min)/(max-min));
}

float restrict_flt(float f, float min, float max) //TODO Inline or macro or something
{
	if (f<min)
		return min;
	if (f>max)
		return max;
	return f;
}

char *mystrdup(const char *s)
{
	char *x;
	if (s != NULL)
	{
		x = (char*)malloc(strlen(s)+1);
		strcpy(x, s);
		return x;
	}
	return NULL;
}

char * strlist_add(struct strlist **list, const char *str)
{
	// 成功返回 true, 失败返回 false
	struct strlist *item = (struct strlist*)malloc(sizeof(struct strlist));
	char* tmp_str = mystrdup(str);
	if (tmp_str != NULL)
	{
		item->str = tmp_str;
		item->next = *list;
		*list = item;
		return tmp_str;
	}
	free(item);
	return NULL;
}

bool strlist_remove(struct strlist **list, char *str, bool release)
{
	// 成功返回 true, 失败返回 false
	struct strlist *item, **p_item = list;
	while (*p_item != NULL)
	{
		item = *p_item;
		if (item->str == str)
		{
			struct strlist *item_next = item->next;
			if (release)
				free(item->str);
			free(item);
			*p_item = item_next;
			return true;
		}
		p_item = &item->next;
	}
	return false;
}

int strlist_find(struct strlist **list, char *str)
{
	struct strlist *item;
	for (item=*list; item != NULL; item=item->next)
		if (!strcmp(item->str, str))
			return 1;
	return 0;
}

struct strlist * strlist_find_ptr(struct strlist **list, const char *str)
{
	struct strlist *item;
	for (item=*list; item != NULL; item=item->next)
		if (item->str == str)
			return item;
	return NULL;
}

void strlist_free(struct strlist **list, bool release)
{
	struct strlist *item;
	while (*list != NULL)
	{
		item = *list;
		*list = (*list)->next;
		if (release)
			free(item->str);
		free(item);
	}
}

bool str_realloc(char *& a, char *b)
{
	size_t l = strlen(b)+1;
	char *c = (char*)realloc(a, l);
	if (c == NULL)
		return false;
	a = (char*)memcpy(c, b, l);
	return true;
}

const static char hex[] = "0123456789ABCDEF";
void strcaturl(char *dst, char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		if ((*s>='0' && *s<='9') ||
		        (*s>='a' && *s<='z') ||
		        (*s>='A' && *s<='Z'))
			*(d++) = *s;
		else
		{
			*(d++) = '%';
			*(d++) = hex[*s>>4];
			*(d++) = hex[*s&15];
		}
	}
	*d = 0;
}

void strappend(char *dst, const char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		*(d++) = *s;
	}
	*d = 0;
}

void *file_load(char *fn, int *size)
{
	FILE *f = fopen(fn, "rb");
	void *s;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	s = malloc(*size);
	if (!s)
	{
		fclose(f);
		return NULL;
	}
	int readsize = fread(s, *size, 1, f);
	fclose(f);
	if (readsize != 1)
	{
		free(s);
		return NULL;
	}
	return s;
}

matrix2d m2d_multiply_m2d(matrix2d m1, matrix2d m2)
{
	matrix2d result = {
		m1.a*m2.a+m1.b*m2.c, m1.a*m2.b+m1.b*m2.d,
		m1.c*m2.a+m1.d*m2.c, m1.c*m2.b+m1.d*m2.d
	};
	return result;
}
vector2d m2d_multiply_v2d(matrix2d m, vector2d v)
{
	vector2d result = {
		m.a*v.x+m.b*v.y,
		m.c*v.x+m.d*v.y
	};
	return result;
}
matrix2d m2d_multiply_float(matrix2d m, float s)
{
	matrix2d result = {
		m.a*s, m.b*s,
		m.c*s, m.d*s,
	};
	return result;
}

vector2d v2d_multiply_float(vector2d v, float s)
{
	vector2d result = {
		v.x*s,
		v.y*s
	};
	return result;
}

vector2d v2d_add(vector2d v1, vector2d v2)
{
	vector2d result = {
		v1.x+v2.x,
		v1.y+v2.y
	};
	return result;
}
vector2d v2d_sub(vector2d v1, vector2d v2)
{
	vector2d result = {
		v1.x-v2.x,
		v1.y-v2.y
	};
	return result;
}

matrix2d m2d_new(float me0, float me1, float me2, float me3)
{
	matrix2d result = {me0,me1,me2,me3};
	return result;
}
vector2d v2d_new(float x, float y)
{
	vector2d result = {x, y};
	return result;
}

void HSV_to_RGB(int h,int s,int v,int *r,int *g,int *b)//convert 0-255(0-360 for H) HSV values to 0-255 RGB
{
	float hh, ss, vv, c, x;
	int m;
	hh = h/60.0f;//normalize values
	ss = s/255.0f;
	vv = v/255.0f;
	c = vv * ss;
	x = c * ( 1 - fabs(fmod(hh,2.0f) -1) );
	if(hh<1){
		*r = (int)(c*255.0);
		*g = (int)(x*255.0);
		*b = 0;
	}
	else if(hh<2){
		*r = (int)(x*255.0);
		*g = (int)(c*255.0);
		*b = 0;
	}
	else if(hh<3){
		*r = 0;
		*g = (int)(c*255.0);
		*b = (int)(x*255.0);
	}
	else if(hh<4){
		*r = 0;
		*g = (int)(x*255.0);
		*b = (int)(c*255.0);
	}
	else if(hh<5){
		*r = (int)(x*255.0);
		*g = 0;
		*b = (int)(c*255.0);
	}
	else if(hh<6){
		*r = (int)(c*255.0);
		*g = 0;
		*b = (int)(x*255.0);
	}
	m = (int)((vv-c)*255.0);
	*r += m;
	*g += m;
	*b += m;
}

void RGB_to_HSV(int r,int g,int b,int *h,int *s,int *v)//convert 0-255 RGB values to 0-255(0-360 for H) HSV
{
	float rr, gg, bb, a,x,c,d;
	rr = r/255.0f;//normalize values
	gg = g/255.0f;
	bb = b/255.0f;
	a = std::min(rr,gg);
	a = std::min(a,bb);
	x = std::max(rr,gg);
	x = std::max(x,bb);
	if (a==x)//greyscale
	{
		*h = 0;
		*s = 0;
		*v = (int)(a*255.0);
	}
	else
	{
 		c = (rr==a) ? gg-bb : ((bb==a) ? rr-gg : bb-rr);
 		d = (rr==a) ? 3 : ((bb==a) ? 1 : 5);
 		*h = (int)(60.0*(d - c/(x - a)));
 		*s = (int)(255.0*((x - a)/x));
 		*v = (int)(255.0*x);
	}
}

void membwand(void * destv, void * srcv, size_t destsize, size_t srcsize)
{
	size_t i;
	unsigned char * dest = (unsigned char*)destv;
	unsigned char * src = (unsigned char*)srcv;
	for(i = 0; i < destsize; i++){
		dest[i] = dest[i] & src[i%srcsize];
	}
}

vector2d v2d_zero = {0,0};
matrix2d m2d_identity = {1,0,0,1};
