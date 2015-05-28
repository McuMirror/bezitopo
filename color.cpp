/******************************************************/
/*                                                    */
/* color.cpp - drawing colors                         */
/*                                                    */
/******************************************************/

#include "color.h"

unsigned char tab40[]=
{
    0,  7, 13, 20, 26, 33, 39, 46, 52, 59, 65, 72, 78, 85, 92, 98,105,111,118,124,
  131,137,144,150,157,163,170,177,183,190,196,203,209,216,222,229,235,242,248,255
};

char tab256[]=
{
   0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
   2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5,
   5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
   7, 7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9,10,
  10,10,10,10,10,11,11,11,11,11,11,11,12,12,12,12,
  12,12,13,13,13,13,13,13,13,14,14,14,14,14,14,15,
  15,15,15,15,15,15,16,16,16,16,16,16,17,17,17,17,
  17,17,17,18,18,18,18,18,18,19,19,19,19,19,19,19,
  20,20,20,20,20,20,20,21,21,21,21,21,21,22,22,22,
  22,22,22,22,23,23,23,23,23,23,24,24,24,24,24,24,
  24,25,25,25,25,25,25,26,26,26,26,26,26,26,27,27,
  27,27,27,27,28,28,28,28,28,28,28,29,29,29,29,29,
  29,30,30,30,30,30,30,30,31,31,31,31,31,31,32,32,
  32,32,32,32,32,33,33,33,33,33,33,33,34,34,34,34,
  34,34,35,35,35,35,35,35,35,36,36,36,36,36,36,37,
  37,37,37,37,37,37,38,38,38,38,38,38,39,39,39,39
};

int colorint(unsigned short colorshort)
{
  int r,g,b,ret;
  if (colorshort<64000)
  {
    b=colorshort%40;
    r=colorshort/1600;
    g=(colorshort-r*1600)/40;
    r=tab40[r];
    g=tab40[g];
    b=tab40[b];
    ret=(r<<16)|(g<<8)|b;
  }
  else
    ret=colorshort+0x7fff0000;
  return ret;
}

unsigned short colorshort(int colorint)
{
  int r,g,b;
  unsigned short ret;
  if (colorint<16777216)
  {
    r=(colorint&0xff0000)>>16;
    g=(colorint&0xff00)>>8;
    b=colorint&0xff;
    r=tab256[r];
    g=tab256[g];
    b=tab256[b];
    ret=(r*1600)+(g*40)+b;
  }
  else
    ret=colorint&0xffff;
  return ret;
}
