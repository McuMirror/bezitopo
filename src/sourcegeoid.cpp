/******************************************************/
/*                                                    */
/* sourcegeoid.cpp - geoidal undulation source data   */
/*                                                    */
/******************************************************/
/* Copyright 2015-2019 Pierre Abbat.
 * This file is part of Bezitopo.
 *
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License and Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and Lesser General Public License along with Bezitopo. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "config.h"
#include "sourcegeoid.h"
#include "smooth5.h"
#include "binio.h"
#include "bicubic.h"
#include "manysum.h"
#include "ldecimal.h"
#include "except.h"

using namespace std;
vector<geoid> geo;
map<int,matrix> quadinv;
vector<smallcircle> excerptcircles;
cylinterval excerptinterval;
bool outBigEndian;

void setEndian(int n)
{
  if (n==ENDIAN_BIG)
    outBigEndian=true;
  if (n==ENDIAN_NATIVE)
#ifdef BIGENDIAN
    outBigEndian=true;
#else
    outBigEndian=false;
#endif
  if (n==ENDIAN_LITTLE)
    outBigEndian=false;
}

void writebinshort(ostream &file,short i)
{
  if (outBigEndian)
    writebeshort(file,i);
  else
    writeleshort(file,i);
}

void writebinint(ostream &file,int i)
{
  if (outBigEndian)
    writebeint(file,i);
  else
    writeleint(file,i);
}

void writebinfloat(ostream &file,float f)
{
  if (outBigEndian)
    writebefloat(file,f);
  else
    writelefloat(file,f);
}

void writebindouble(ostream &file,double f)
{
  if (outBigEndian)
    writebedouble(file,f);
  else
    writeledouble(file,f);
}

string readword(istream &file)
{
  int ch;
  string ret;
  int state=0;
  do
  {
    ch=file.get();
    switch (state)
    {
      case 0:
	if (!isspace(ch))
	  state=1;
	if (ch<0)
	  state=2;
	break;
      case 1:
	if (isspace(ch) || ch<0)
	  state=2;
    }
    if (state==1)
      ret+=ch;
  } while (state<2);
  return ret;
}

double readdouble(istream &file)
// This can throw.
{
  string str;
  size_t pos;
  double ret;
  str=readword(file);
  ret=stod(str,&pos);
  if (pos<str.length())
    throw 0;
  return ret;
}

double geolattice::elev(int lat,int lon)
{
  int easting,northing,eint,nint;
  double epart,npart,ne,nw,se,sw,ret;
  xy neslp,nwslp,seslp,swslp;
  easting=(lon-wbd)&0x7fffffff;
  northing=lat-sbd;
  epart=-(double)easting*width/(wbd-ebd);
  npart=(double)northing*height/(nbd-sbd);
  eint=floor(epart);
  nint=floor(npart);
  epart-=eint;
  npart-=nint;
  if (eint==width && epart==0)
  {
    eint--;
    epart=1;
  }
  if (nint==height && npart==0)
  {
    nint--;
    npart=1;
  }
  npart=1-npart;
  epart=1-epart;
  npart=1-npart;
  epart=1-epart;
  if (eint>=0 && eint<width && nint>=0 && nint<height)
  {
    sw=undula[(width+1)*nint+eint];
    se=undula[(width+1)*nint+eint+1];
    nw=undula[(width+1)*(nint+1)+eint];
    ne=undula[(width+1)*(nint+1)+eint+1];
    swslp=xy(eslope[(width+1)*nint+eint],nslope[(width+1)*nint+eint])/2;
    seslp=xy(eslope[(width+1)*nint+eint+1],nslope[(width+1)*nint+eint+1])/2;
    nwslp=xy(eslope[(width+1)*(nint+1)+eint],nslope[(width+1)*(nint+1)+eint])/2;
    neslp=xy(eslope[(width+1)*(nint+1)+eint+1],nslope[(width+1)*(nint+1)+eint+1])/2;
  }
  else
    sw=se=nw=ne=-2147483648;
  if (sw==-2147483648)
    sw=1e30;
  if (se==-2147483648)
    se=1e30;
  if (nw==-2147483648)
    nw=1e30;
  if (ne==-2147483648)
    ne=1e30;
  //ret=((sw*(1-epart)+se*epart)*(1-npart)+(nw*(1-epart)+ne*epart)*npart)/65536;
  ret=bicubic(sw,swslp,se,seslp,nw,nwslp,ne,neslp,epart,npart)/65536;
  if (ret>8850 || ret<-11000)
    ret=NAN;
  return ret;
}

void geolattice::setundula()
{
  int i,j;
  latlong ll;
  for (i=0;i<=height;i++)
  {
    ll.lat=bintorad(sbd)-(double)i/height*bintorad(sbd-nbd);
    for (j=0;j<=width;j++)
    {
      ll.lon=bintorad(wbd)-(double)j/width*bintorad(wbd-ebd);
      undula[(width+1)*i+j]=rint(avgelev(Sphere.geoc(ll,0))*65536);
    }
  }
}

double geolattice::elev(xyz dir)
{
  return elev(dir.lati(),dir.loni());
}

void geolattice::dump()
{
  int i,j;
  cout<<"undula:"<<endl;
  for (i=0;i<height+1;i++)
  {
    for (j=0;j<width+1;j++)
      cout<<setw(11)<<undula[i*(width+1)+j];
    cout<<endl;
  }
  cout<<"eslope:"<<endl;
  for (i=0;i<height+1;i++)
  {
    for (j=0;j<width+1;j++)
      cout<<setw(11)<<eslope[i*(width+1)+j];
    cout<<endl;
  }
  cout<<"nslope:"<<endl;
  for (i=0;i<height+1;i++)
  {
    for (j=0;j<width+1;j++)
      cout<<setw(11)<<nslope[i*(width+1)+j];
    cout<<endl;
  }
}

void geolattice::setslopes()
/* Given points a,b,c spaced 1 apart in order:
 * Slope at b is sl(a,b)+sl(b,c)-sl(a,c). This is just sl(a,c)=(c-a)/2.
 * (2b-2a+2c-2b+a-c)/2=(c-a)/2
 * The division by 2 is done in elev.
 * Slope at c (the edge) is sl(b,c)+sl(c,a)-sl(a,b). This is (c-b)+(c-a)/2-(b-a)
 * =(2c-2b+c-a-2b+2a)/2=(3c-4b+a)/2
 */
{
  int i,j;
  for (i=0;i<height+1;i++)
    for (j=1;j<width;j++)
      eslope[i*(width+1)+j]=undula[i*(width+1)+j+1]-undula[i*(width+1)+j-1];
  if (width>1)
    if (ebd-wbd==DEG360)
      for (i=0;i<height+1;i++)
      {
        eslope[i*(width+1)]=eslope[(i+1)*(width+1)-1]=undula[i*(width+1)+1]-undula[(i+1)*(width+1)-2];
      }
    else
      for (i=0;i<height+1;i++)
      {
        eslope[i*(width+1)]=4*undula[i*(width+1)+1]-undula[i*(width+1)+2]-3*undula[i*(width+1)];
        eslope[(i+1)*(width+1)-1]=3*undula[(i+1)*(width+1)-1]-4*undula[(i+1)*(width+1)-2]+undula[(i+1)*(width+1)-3];
      }
  for (i=1;i<height;i++)
    for (j=0;j<width+1;j++)
      nslope[i*(width+1)+j]=undula[(i+1)*(width+1)+j]-undula[(i-1)*(width+1)+j];
  if (height>1)
    for (j=0;j<width+1;j++)
    {
      nslope[j]=4*undula[(width+1)+j]-undula[2*(width+1)+j]-3*undula[j];
      nslope[height*(width+1)+j]=3*undula[height*(width+1)+j]-4*undula[(height-1)*(width+1)+j]+undula[(height-2)*(width+1)+j];
    }
  if (height<=16 && width<=16)
    dump();
}

void readusngsbinheaderbe(usngsheader &hdr,fstream &file)
{
  hdr.south=readbedouble(file);
  hdr.west=readbedouble(file);
  hdr.latspace=readbedouble(file);
  hdr.longspace=readbedouble(file);
  hdr.nlat=readbeint(file);
  hdr.nlong=readbeint(file);
  hdr.dtype=readbeint(file);
}

void readusngsbinheaderle(usngsheader &hdr,fstream &file)
{
  hdr.south=readledouble(file);
  hdr.west=readledouble(file);
  hdr.latspace=readledouble(file);
  hdr.longspace=readledouble(file);
  hdr.nlat=readleint(file);
  hdr.nlong=readleint(file);
  hdr.dtype=readleint(file);
}

void writeusngsbinheader(usngsheader &hdr,ostream &file)
{
  writebindouble(file,hdr.south);
  writebindouble(file,hdr.west);
  writebindouble(file,hdr.latspace);
  writebindouble(file,hdr.longspace);
  writebinint(file,hdr.nlat);
  writebinint(file,hdr.nlong);
  writebinint(file,hdr.dtype);
}

bool sanitycheck(usngsheader &hdr)
{
  bool ssane,wsane,latsane,longsane,nlatsane,nlongsane,typesane;
  ssane=hdr.south>-360.0001 && hdr.south<360.0001 && (hdr.south==0 || fabs(hdr.south)>0.000001);
  wsane=hdr.west>-360.0001 && hdr.west<360.0001 && (hdr.west==0 || fabs(hdr.west)>0.000001);
  latsane=hdr.latspace>0.000001 && hdr.latspace<190;
  longsane=hdr.longspace>0.000001 && hdr.longspace<190;
  nlatsane=hdr.nlat>0 && (hdr.nlat-1)*hdr.latspace<180.000001;
  nlongsane=hdr.nlong>0 && (hdr.nlong-1)*hdr.longspace<360.000001;
  typesane=hdr.dtype<256;
  return ssane && wsane && latsane && longsane && nlatsane && nlongsane && typesane;
}

bool sanitycheck(carlsongsfheader &hdr)
{
  bool ssane,wsane,nsane,esane,latsane,nlatsane,nlongsane;
  ssane=hdr.south>-360.0001 && hdr.south<360.0001 && (hdr.south==0 || fabs(hdr.south)>0.000001);
  wsane=hdr.west>-360.0001 && hdr.west<360.0001 && (hdr.west==0 || fabs(hdr.west)>0.000001);
  nsane=hdr.north>-360.0001 && hdr.north<360.0001 && (hdr.north==0 || fabs(hdr.north)>0.000001);
  esane=hdr.east>-360.0001 && hdr.east<360.0001 && (hdr.east==0 || fabs(hdr.east)>0.000001);
  latsane=hdr.south<hdr.north;
  nlatsane=hdr.nlat>0 && hdr.nlat<=2000000;
  nlongsane=hdr.nlong>0 && hdr.nlong<=4000000;
  return ssane && wsane && nsane && esane && latsane && nlatsane && nlongsane;
}

bool sanitycheck(usngatxtheader &hdr)
{
  bool ssane,wsane,nsane,esane,latsane,longsane;
  ssane=hdr.south>-360.0001 && hdr.south<360.0001 && (hdr.south==0 || fabs(hdr.south)>0.000001);
  wsane=hdr.west>-360.0001 && hdr.west<360.0001 && (hdr.west==0 || fabs(hdr.west)>0.000001);
  nsane=hdr.north>-360.0001 && hdr.north<360.0001 && (hdr.north==0 || fabs(hdr.north)>0.000001);
  esane=hdr.east>-360.0001 && hdr.east<360.0001 && (hdr.east==0 || fabs(hdr.east)>0.000001);
  latsane=hdr.south<hdr.north && hdr.latspace>0.000001 && hdr.latspace<190;
  longsane=hdr.longspace>0.000001 && hdr.longspace<190;
  return ssane && wsane && nsane && esane && latsane && longsane;
}

void geolattice::resize(size_t dataSize)
// dataSize is the largest number of data that can be in the file, given its size.
{
  if (((size_t)width+1)*((size_t)height+1)!=(width+1)*(height+1))
    throw BeziExcept(badHeader);
  if (dataSize<((size_t)width+1)*((size_t)height+1))
    throw BeziExcept(badHeader);
  undula.resize((width+1)*(height+1));
  eslope.resize((width+1)*(height+1));
  nslope.resize((width+1)*(height+1));
}

void geolattice::setheader(usngsheader &hdr,size_t dataSize)
{
  sbd=degtobin(hdr.south);
  wbd=degtobin(hdr.west);
  nbd=degtobin(hdr.south+(hdr.nlat-1)*hdr.latspace);
  ebd=degtobin(hdr.west+(hdr.nlong-1)*hdr.longspace);
  width=hdr.nlong-1;
  height=hdr.nlat-1;
  resize(dataSize);
}

void geolattice::cvtheader(usngsheader &hdr)
{
  hdr.south=bintodeg(sbd);
  hdr.west=bintodeg(wbd);
  hdr.latspace=bintodeg(nbd-sbd)/height;
  hdr.longspace=-bintodeg(wbd-ebd)/width;
  hdr.nlong=width+1;
  hdr.nlat=height+1;
  hdr.dtype=1; // 1 means float. Don't know what anything else would mean.
}

cylinterval geolattice::boundrect()
{
  cylinterval ret;
  ret.sbd=sbd;
  ret.wbd=wbd;
  ret.nbd=nbd;
  ret.ebd=ebd;
  return ret;
}

void geolattice::setbound(cylinterval bound)
{
  sbd=bound.sbd;
  wbd=bound.wbd;
  nbd=bound.nbd;
  ebd=bound.ebd;
}

void geolattice::setheader(carlsongsfheader &hdr,size_t dataSize)
{
  sbd=degtobin(hdr.south);
  wbd=degtobin(hdr.west);
  nbd=degtobin(hdr.north);
  ebd=degtobin(hdr.east);
  if (wbd-ebd>=0)
    ebd+=DEG360;
  width=hdr.nlong;
  height=hdr.nlat;
  resize(dataSize);
}

void geolattice::cvtheader(carlsongsfheader &hdr)
{
  hdr.south=bintodeg(sbd);
  hdr.west=bintodeg(wbd);
  hdr.north=bintodeg(nbd);
  hdr.east=bintodeg(ebd);
  if (hdr.west<0)
    hdr.west+=360;
  if (hdr.east<0)
    hdr.east+=360;
  hdr.nlong=width;
  hdr.nlat=height;
}

void geolattice::setheader(usngatxtheader &hdr,size_t dataSize)
{
  double around=0;
  sbd=degtobin(hdr.south);
  wbd=degtobin(hdr.west);
  nbd=degtobin(hdr.north);
  ebd=degtobin(hdr.east);
  if (wbd-ebd>=0)
  {
    ebd+=DEG360;
    around=360;
  }
  width=rint((around+hdr.east-hdr.west)/hdr.longspace);
  height=rint((hdr.north-hdr.south)/hdr.latspace);
  resize(dataSize);
}

void geolattice::cvtheader(usngatxtheader &hdr)
{
  hdr.south=bintodeg(sbd);
  hdr.west=bintodeg(wbd);
  hdr.north=bintodeg(nbd);
  hdr.east=bintodeg(ebd);
  if (hdr.east<=hdr.west)
    hdr.east+=720;
  hdr.longspace=(hdr.east-hdr.west)/width;
  hdr.latspace=(hdr.north-hdr.south)/height;
}

void geolattice::settest()
{
  int i,j;
  sbd=wbd=degtobin(-2);
  nbd=ebd=degtobin(2);
  width=height=4;
  resize();
  for (i=0;i<5;i++)
    for (j=0;j<5;j++)
      undula[i+5*j]=61000*(i-2)+4096*sqr(i-2)+37700*(j-2)-2048*sqr(j-2);
  setslopes();
}

void geolattice::setfineness(int latfineness,int lonfineness)
/* fineness is units per 180°. Doing this on a geolattice that already has
 * data in it will shear the data.
 */
{
  width=-rint((double)(wbd-ebd)*(double)lonfineness/DEG180);
  height=-rint((double)(sbd-nbd)*(double)latfineness/DEG180);
  resize();
}

int geolattice::getLatFineness()
{
  double dfine=-(double)height*DEG180/(sbd-nbd);
  if (dfine>0.6 && dfine<=DEG180)
    return nearestSmooth(rint(dfine));
  else
    return 0;
}

int geolattice::getLonFineness()
{
  double dfine=-(double)width*DEG180/(wbd-ebd);
  if (dfine>0.6 && dfine<=DEG180)
    return nearestSmooth(rint(dfine));
  else
    return 0;
}

void readusngatxtheader(usngatxtheader &hdr,istream &file)
{
  double dnlong,dnlat;
  try
  {
    hdr.south=readdouble(file);
    hdr.north=readdouble(file);
    hdr.west=readdouble(file);
    hdr.east=readdouble(file);
    hdr.latspace=readdouble(file);
    hdr.longspace=readdouble(file);
  }
  catch (...)
  {
    throw BeziExcept(badHeader);
  }
}

void writeusngatxtheader(usngatxtheader &hdr,ostream &file)
{
  double prec=bintodeg(1)/2;
  file<<fixed<<ldecimal(hdr.south,prec)<<' '<<ldecimal(hdr.north,prec)<<' ';
  file<<fixed<<ldecimal(hdr.west,prec)<<' '<<ldecimal(hdr.east,prec)<<endl;
  file<<ldecimal(hdr.latspace,prec)<<' '<<ldecimal(hdr.longspace,prec);
}

int readusngatxt(geolattice &geo,string filename)
/* This geoid file has order-360 harmonics, but is sampled every 0.25°,
 * so it may not interpolate accurately. It would be better to compute
 * the geoid from the coefficients; this requires making sense of a
 * Fortran program.
 * http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm96/egm96.html
 * http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
 */
{
  int i,j,ret=0;
  fstream file;
  usngatxtheader hdr;
  file.open(filename,fstream::in|fstream::binary);
  if (file.is_open())
  {
    try
    {
      readusngatxtheader(hdr,file);
    }
    catch (BeziExcept &e)
    {
      ret=-e.getNumber();
    }
    if (ret==0 && sanitycheck(hdr))
    {
      cout<<"Header sane"<<endl;
      cout<<"South "<<hdr.south<<" West "<<hdr.west<<endl;
      cout<<"North "<<hdr.north<<" East "<<hdr.east<<endl;
      cout<<"Latitude spacing "<<hdr.latspace<<" Longitude spacing "<<hdr.longspace<<endl;
      try
      {
	geo.setheader(hdr,fileSize(file)/2);
        for (i=0;i<geo.height+1;i++)
          for (j=0;j<geo.width+1;j++)
            geo.undula[(geo.height-i)*(geo.width+1)+j]=rint(65536*(readdouble(file)));
      }
      catch (...)
      {
        ret=1;
      }
      if (file.fail() || ret==1)
	ret=1;
      else
      {
	ret=2;
	geo.setslopes();
      }
    }
    else
      ret=1;
    file.close();
  }
  else
    ret=0;
  return ret;
}

void writeusngatxt(geolattice &geo,string filename)
{
  int i,j;
  fstream file;
  usngatxtheader hdr;
  geo.cvtheader(hdr);
  file.open(filename,fstream::out);
  writeusngatxtheader(hdr,file);
  for (i=geo.height;i>=0;i--)
    for (j=0;j<geo.width+1;j++)
    {
      if (j%16==0)
        file<<'\n';
      else
        file<<' ';
      file<<ldecimal(geo.undula[i*(geo.width+1)+j]/65536.,1/131072.);
    }
  file<<endl;
  file.close();
}

int readusngatxt(geoid &geo,string filename)
{
  delete geo.glat;
  delete geo.ghdr;
  delete geo.cmap;
  geo.ghdr=nullptr;
  geo.cmap=nullptr;
  geo.glat=new geolattice;
  return readusngatxt(*geo.glat,filename);
}

void writeusngatxt(geoid &geo,string filename)
{
  if (geo.glat)
    writeusngatxt(*geo.glat,filename);
  else
    throw BeziExcept(unsetGeoid);
}

int readusngabin(geolattice &geo,string filename)
/* Like the usngatxt format, this covers the whole earth, but it has no header.
 * The file consists of lines in this format:
 * n e e e e e e e e e e ... e e e e e e e e e e e n
 * where n is the total number of bytes of e's (i.e. 4 times the number of e's).
 * The first line is the North Pole, the last is the South Pole, and the first
 * number in each line has to be repeated at the end when reading it in.
 */
{
  int i,j,ret=0,endian,linelen0,linelen1;
  double firstund,und;
  fstream file;
  file.open(filename,fstream::in|fstream::binary);
  if (file.is_open())
  {
    for (endian=0;endian<2 && ret<2;endian++)
    {
      file.seekg(0);
      ret=0;
      geo.undula.clear();
      for (i=0;!file.eof() && ret==0;i++)
      {
        linelen0=endian?readbeint(file):readleint(file);
        if ((i>0 && linelen0!=linelen1) || linelen0&3)
          ret=1;
        for (j=0;!file.eof() && ret==0 && j<linelen0/4;j++)
        {
          und=(endian?readbefloat(file):readlefloat(file))*65536;
          if (j==0)
            firstund=und;
	  geo.undula.push_back(und);
	  if (file.peek()==EOF)
	    ret=1;
        }
        geo.undula.push_back(firstund);
        linelen1=endian?readbeint(file):readleint(file);
        if (linelen0!=linelen1)
          ret=1;
        file.peek(); // set file.eof if at end of file
      }
      if (file.fail())
	ret=1;
      else if (ret==0)
      {
	ret=2;
        geo.nbd=DEG90;
        geo.sbd=-DEG90;
        geo.wbd=0;
        geo.ebd=DEG360;
        geo.height=i-1;
        geo.width=linelen0/4;
        if (geo.height>0 && geo.width>0)
        {
          for (i=0;2*i<geo.height;i++)
            for (j=0;j<=geo.width;j++)
              swap(geo.undula[i*(geo.width+1)+j],geo.undula[(geo.height-i)*(geo.width+1)+j]);
          geo.resize();
          geo.setslopes();
        }
        else
          ret=1;
      }
    }
    file.close();
  }
  else
    ret=0;
  return ret;
}

int readusngabin(geoid &geo,string filename)
{
  delete geo.glat;
  delete geo.ghdr;
  delete geo.cmap;
  geo.ghdr=nullptr;
  geo.cmap=nullptr;
  geo.glat=new geolattice;
  return readusngabin(*geo.glat,filename);
}

void readcarlsongsfheader(carlsongsfheader &hdr,istream &file)
{
  double dnlong,dnlat;
  try
  { // Note the counterintuitive order of dnlong and dnlat.
    hdr.south=readdouble(file); // lat
    hdr.west=readdouble(file);  // lon
    hdr.north=readdouble(file); // lat
    hdr.east=readdouble(file);  // lon
    dnlong=readdouble(file);    // lon
    dnlat=readdouble(file);     // lat
    /* The numbers of rows and columns must be integers,
     * but are written as 118.0 in gsf files.
     */
    hdr.nlong=dnlong;
    hdr.nlat=dnlat;
  }
  catch (...)
  {
    throw BeziExcept(badHeader);
  }
  if (hdr.nlong!=dnlong || hdr.nlat!=dnlat)
    throw BeziExcept(badHeader);
}

void writecarlsongsfheader(carlsongsfheader &hdr,ostream &file)
{
  double prec=bintodeg(1)/2;
  file<<fixed<<ldecimal(hdr.south,prec)<<'\n'<<ldecimal(hdr.west,prec)<<'\n';
  file<<fixed<<ldecimal(hdr.north,prec)<<'\n'<<ldecimal(hdr.east,prec)<<'\n';
  file<<hdr.nlong<<'\n'<<hdr.nlat<<endl;
}

int readcarlsongsf(geolattice &geo,string filename)
/* This is a text file used by Carlson software.
 * http://web.carlsonsw.com/files/knowledgebase/kbase_attach/716/Geoid Separation File Format.pdf
 */
{
  int i,j,ret=0;
  fstream file;
  carlsongsfheader hdr;
  file.open(filename,fstream::in|fstream::binary);
  if (file.is_open())
  {
    try
    {
      readcarlsongsfheader(hdr,file);
    }
    catch (BeziExcept &e)
    {
      ret=-e.getNumber();
    }
    if (ret==0 && sanitycheck(hdr))
    {
      cout<<"Header sane"<<endl;
      cout<<"South "<<hdr.south<<" West "<<hdr.west<<endl;
      cout<<"North "<<hdr.north<<" East "<<hdr.east<<endl;
      cout<<"Rows "<<hdr.nlat<<" Columns "<<hdr.nlong<<endl;
      try
      {
	geo.setheader(hdr,fileSize(file)/2);
        for (i=0;i<geo.height+1;i++)
          for (j=0;j<geo.width+1;j++)
            geo.undula[i*(geo.width+1)+j]=rint(65536*(readdouble(file)));
      }
      catch (...)
      {
        ret=1;
      }
      if (file.fail() || ret==1)
	ret=1;
      else
      {
	ret=2;
	geo.setslopes();
      }
    }
    else
      ret=1;
    file.close();
  }
  else
    ret=0;
  return ret;
}

void writecarlsongsf(geolattice &geo,string filename)
/* Writes a GSF file. The use of ldecimal makes the output smaller.
 * The NGS files have numbers to the nearest 100 µm. These are rounded
 * to the nearest 1/65536 m when read in, then output to the GSF with
 * an error of at most 1/131072. The first number in the Alaska file
 * that is written out with five digits after the decimal point is:
 * NGS: 4.3433 (0x408afc50) (offset 0xfa8)
 * *64K: 284642.5
 * Rounded: 284642
 * /64k: 4.343292236328125
 * GSF: 4.34329
 * NGS output: 4.34329 (0x408afc40)
 */
{
  int i,j;
  fstream file;
  carlsongsfheader hdr;
  geo.cvtheader(hdr);
  file.open(filename,fstream::out);
  writecarlsongsfheader(hdr,file);
  for (i=0;i<geo.height+1;i++)
    for (j=0;j<geo.width+1;j++)
      file<<ldecimal(geo.undula[i*(geo.width+1)+j]/65536.,1/131072.)<<'\n';
  file.close();
}

int readcarlsongsf(geoid &geo,string filename)
{
  delete geo.glat;
  delete geo.ghdr;
  delete geo.cmap;
  geo.ghdr=nullptr;
  geo.cmap=nullptr;
  geo.glat=new geolattice;
  return readcarlsongsf(*geo.glat,filename);
}

void writecarlsongsf(geoid &geo,string filename)
{
  if (geo.glat)
    writecarlsongsf(*geo.glat,filename);
  else
    throw BeziExcept(unsetGeoid);
}

int readusngsbin(geolattice &geo,string filename)
{
  int i,j,ret;
  fstream file;
  usngsheader hdr;
  bool bigendian;
  file.open(filename,fstream::in|fstream::binary);
  if (file.is_open())
  {
    readusngsbinheaderle(hdr,file);
    if (sanitycheck(hdr))
      bigendian=false;
    else
    {
      file.seekg(0);
      readusngsbinheaderbe(hdr,file);
      bigendian=true;
    }
    if (sanitycheck(hdr))
    {
      cout<<"Header sane"<<endl;
      cout<<"South "<<hdr.south<<" West "<<hdr.west<<endl;
      cout<<"Latitude spacing "<<hdr.latspace<<" Longitude spacing "<<hdr.longspace<<endl;
      cout<<"Rows "<<hdr.nlat<<" Columns "<<hdr.nlong<<endl;
      try
      {
	geo.setheader(hdr,fileSize(file)/4);
      }
      catch (...)
      {
	ret=1;
	geo.height=geo.width=-1;
      }
      for (i=0;i<geo.height+1;i++)
	for (j=0;j<geo.width+1;j++)
	  geo.undula[i*(geo.width+1)+j]=rint(65536*(bigendian?readbefloat(file):readlefloat(file)));
      if (file.fail() || geo.height<0)
	ret=1;
      else
      {
	ret=2;
	geo.setslopes();
      }
    }
    else
      ret=1;
    file.close();
  }
  else
    ret=0;
  return ret;
}

void writeusngsbin(geolattice &geo,string filename)
{
  int i,j;
  fstream file;
  usngsheader hdr;
  geo.cvtheader(hdr);
  file.open(filename,fstream::out|fstream::binary);
  writeusngsbinheader(hdr,file);
  for (i=0;i<geo.height+1;i++)
    for (j=0;j<geo.width+1;j++)
      writebinfloat(file,geo.undula[i*(geo.width+1)+j]/65536.);
  file.close();
}

int readusngsbin(geoid &geo,string filename)
{
  delete geo.glat;
  delete geo.ghdr;
  delete geo.cmap;
  geo.ghdr=nullptr;
  geo.cmap=nullptr;
  geo.glat=new geolattice;
  return readusngsbin(*geo.glat,filename);
}

void writeusngsbin(geoid &geo,string filename)
{
  if (geo.glat)
    writeusngsbin(*geo.glat,filename);
  else
    throw BeziExcept(unsetGeoid);
}

int readboldatni(geoid &geo,string filename)
{
  delete geo.glat;
  delete geo.ghdr;
  delete geo.cmap;
  geo.glat=nullptr;
  geo.ghdr=new geoheader;
  geo.cmap=new cubemap;
  ifstream file;
  int ret;
  file.open(filename,fstream::in|fstream::binary);
  if (file.is_open())
  {
    ret=2;
    try
    {
      geo.ghdr->readBinary(file);
      geo.cmap->readBinary(file);
      geo.cmap->scale=ldexp(1,geo.ghdr->logScale);
    }
    catch (...)
    {
      ret=1;
    }
  }
  else
    ret=0;
  return ret;
}

void writeboldatni(geoid &geo,string filename)
{
  fstream file;
  file.open(filename,fstream::out|fstream::binary);
  if (geo.ghdr && geo.cmap)
  {
    geo.ghdr->hash=geo.cmap->hash();
    if (!geo.ghdr->excerpted)
      geo.ghdr->origHash=geo.ghdr->hash;
    geo.ghdr->writeBinary(file);
    geo.cmap->writeBinary(file);
  }
  else
    throw BeziExcept(unsetGeoid);
}

double avgelev(xyz dir)
{
  int i,n;
  double u,sum;
  for (sum=i=n=0;i<geo.size();i++)
  {
    u=geo[i].elev(dir);
    if (std::isfinite(u))
    {
      sum+=u;
      n++;
    }
  }
  return sum/n;
}

bool allBoldatni()
{
  int i;
  bool ret=true;
  for (i=0;i<geo.size();i++)
  {
    if (geo[i].cmap==nullptr)
      ret=false;
  }
  return ret;
}

geoquadMatch bolMatch(geoquad &quad)
{
  int i;
  geoquadMatch ret,oneMatch;
  for (i=0;i<geo.size();i++)
    if (geo[i].cmap)
    {
      oneMatch=geo[i].cmap->match(quad);
      ret.flags|=oneMatch.flags;
      ret.numMatches+=oneMatch.numMatches;
      if (ret.numMatches>1)
	ret.sameQuad=nullptr;
      else if (ret.numMatches==oneMatch.numMatches)
	ret.sameQuad=oneMatch.sameQuad;
    }
  return ret;
}

void smallcircle::setradius(int r)
{
  radius=r;
  cosrad=cos(r);
}
  
smallcircle smallcircle::complement()
{
  smallcircle ret;
  ret.center=center;
  ret.setradius(DEG90-radius);
  return ret;
}

double smallcircle::farin(xyz pt)
{
  return dot(pt,center)-cosrad*pt.length()*center.length();
}

bool smallcircle::in(xyz pt)
{
  return farin(pt)>0;
}

vector<xyz> gcscint(xyz gc,smallcircle sc)
/* Compute the intersections of a great circle, represented by its pole,
 * and a small circle. The result has length 1; multiply by EARTHRAD
 * to put it on the spherical earth's surface.
 */
{
  xyz nearest; // nearest point on gc to center of sc
  double nearestin,farthestin;
  double x,y;
  xyz sidebeam;
  vector<xyz> ret;
  sidebeam=cross(gc,sc.center);
  nearest=cross(sidebeam,gc);
  if (nearest.length())
  {
    nearest/=nearest.length();
    sidebeam/=sidebeam.length();
    nearestin=sc.farin(nearest);
    farthestin=sc.farin(-nearest);
    x=(farthestin+nearestin)/(farthestin-nearestin);
    if (x>=-1 && x<=1)
    {
      y=sqrt(1-sqr(x));
      ret.push_back(x*nearest+y*sidebeam);
      if (y)
	ret.push_back(x*nearest-y*sidebeam);
    }
  }
  return ret;
}

bool overlap(smallcircle sc,const geoquad &gq)
/* To determine whether a smallcircle and a geoquad overlap:
 * • If the center of either is inside the other, then they overlap; return true.
 * • Find the intersections of the great circles which bound the geoquad
 *   with the smallcircle. There are anywhere from 0 to 8 of them.
 *   It is possible for the smallcircle to intersect the bounds eight times
 *   and overlap the geoquad, yet no point halfway between the intersections
 *   is inside the geoquad. For instance, the geoquad is the entire Benin face,
 *   the smallcircle is centered at Galápagos, and the radius is 5311 km.
 * • For every pair of intersections, find the ends of the diameter of the
 *   smallcircle which bisects the two points, and check whether they are
 *   in the geoquad. If any are, return true.
 * 
 * If sc passes through a corner of gq, and intspole is a multimap, overlap
 * returns true, but if intspole is a map, it returns false. This occurs for
 * 24 geoquads and the athwi45d circle, which is tangent to those geoquads
 * at their corners, the North Pole for 12 and Galápagos for the other 12.
 * The intersections include the corner twice. With a multimap, both occurrences
 * of the corner remain, and halfway between them is the same corner, which
 * may be inside the geoquad depending on roundoff error. With a map, only one
 * occurrence of the corner remains, and no halfway point is in the geoquad.
 */
{
  vball scc=encodedir(sc.center);
  xyz gqc=decodedir(gq.vcenter());
  bool ret=sc.in(gqc)||gq.in(scc);
  array<vball,4> gqvbounds=gq.bounds();
  array<xyz,4> gqbounds;
  vector<xyz> intersections,ints1,bisectors;
  map<double,xyz> intspole;
  map<double,xyz>::iterator h;
  xyz crossrot;
  int rotangle;
  Quaternion unrot; // puts sc.center at the pole to sort intersections circularly
  int i,j,k;
  if (!ret)
  {
    for (i=0;i<4;i++)
    {
      gqbounds[i]=decodedir(gqvbounds[i]);
      ints1=gcscint(gqbounds[i],sc);
      for (j=0;j<ints1.size();j++)
        intersections.push_back(ints1[j]);
    }
    if (intersections.size()>3)
    {
      if (sc.center.getz()>0)
      {
        crossrot=sc.center*xyz(0,0,1);
        rotangle=DEG90-sc.center.lati();
      }
      else
      {
        crossrot=sc.center*xyz(0,0,-1);
        rotangle=DEG90+sc.center.lati();
      }
      unrot=versor(crossrot,rotangle);
      for (i=0;i<intersections.size();i++)
      {
        crossrot=unrot.rotate(intersections[i]);
        intspole[crossrot.lon()]=intersections[i];
      }
      intersections.clear();
      for (h=intspole.begin();h!=intspole.end();h++)
        intersections.push_back(h->second);
    }
    for (i=0;i<intersections.size();i++)
    {
      bisectors=gcscint(intersections[i]-intersections[(i+1)%intersections.size()],sc);
      for (k=0;k<bisectors.size();k++)
        if (gq.in(encodedir(bisectors[k])))
          ret=true;
    }
  }
  return ret;
}

cylinterval smallcircle::boundrect()
/* Returns the smallest rectangle in cylindrical projection which contains c.
 * This is done by computing where the complement of c intersects the equator.
 * If c contains a pole, the east and west bounds differ by 360°, and the north
 * or south bound is ±90°. If c passes through a pole, the east and west bounds
 * differ by 180°.
 */
{
  smallcircle comp=complement();
  cylinterval ret;
  vector<xyz> intersections;
  int clat=center.lati(),clon=center.loni();
  ret.nbd=clat+radius;
  ret.sbd=clat-radius;
  if (ret.nbd==DEG90 || ret.sbd==-DEG90)
  {
    ret.ebd=clon+DEG90;
    ret.wbd=clon-DEG90;
  }
  else if (ret.nbd>DEG90 || ret.sbd<-DEG90)
  {
    if (ret.nbd>DEG90)
      ret.nbd=DEG90;
    if (ret.sbd<-DEG90)
      ret.sbd=-DEG90;
    ret.ebd=clon+DEG180;
    ret.wbd=clon-DEG180;
  }
  else
  {
    intersections=gcscint(xyz(0,0,6371e3),comp);
    assert(intersections.size()==2);
    ret.ebd=radtobin(intersections[0].lon());
    ret.wbd=radtobin(intersections[1].lon());
    ret.ebd=clon+foldangle(ret.ebd-clon);
    ret.wbd=clon+foldangle(ret.wbd-clon);
    if (ret.ebd-ret.wbd>0)
      swap(ret.ebd,ret.wbd);
    ret.ebd+=DEG90;
    ret.wbd-=DEG90;
  }
  return ret;
}

geoid::geoid()
{
  cmap=nullptr;
  glat=nullptr;
  ghdr=nullptr;
}

geoid::~geoid()
{
  delete cmap;
  delete glat;
  delete ghdr;
}

geoid::geoid(const geoid &b)
{
  cmap=nullptr;
  glat=nullptr;
  ghdr=nullptr;
  if (b.cmap)
  {
    cmap=new cubemap;
    *cmap=*b.cmap;
  }
  if (b.glat)
  {
    glat=new geolattice;
    *glat=*b.glat;
  }
  if (b.ghdr)
  {
    ghdr=new geoheader;
    *ghdr=*b.ghdr;
  }
}

double geoid::elev(int lat,int lon)
{
  if (cmap)
    return cmap->undulation(lat,lon);
  else if (glat)
    return glat->elev(lat,lon);
  else
    return elev(Sphere.geoc(lat,lon,0));
}

double geoid::elev(xyz dir)
{
  if (cmap)
    return cmap->undulation(dir);
  else if (glat)
    return glat->elev(dir);
  else // fake geoid for testing
    return cos(dist(dir,xyz(3678298.565,3678298.565,3678298.565))/1e5)*30
          +cos(dist(dir,xyz(3678298.565,-3678298.-565,3678298.565))/1.1892e5)*36
          +cos(dist(dir,xyz(-3678298.565,3678298.565,-3678298.565))/1.4142e5)*42
          +cos(dist(dir,xyz(-3678298.565,-3678298.565,3678298.565))/1.6818e5)*50;
}

int geoid::getLatFineness()
{
  if (glat)
    return glat->getLatFineness();
  else
    return 0;
}

int geoid::getLonFineness()
{
  if (glat)
    return glat->getLonFineness();
  else
    return 0;
}

cylinterval geoid::boundrect()
{
  cylinterval ret;
  if (cmap)
    ret=cmap->boundrect();
  else if (glat)
    ret=glat->boundrect();
  else
    ret.setempty();
  return ret;
}

matrix autocorr(double qpoints[][16],int qsz)
/* Autocorrelation of the six undulation components, masked by which of qpoints
 * are finite. When all are finite, the matrix is diagonal-dominant, but when
 * only half are finite, it often isn't.
 */
{
  geoquad unitquad[6];
  int i,j,k,l;
  matrix ret(6,6);
  manysum sum;
  for (i=0;i<6;i++)
    for (j=0;j<6;j++)
      unitquad[i].und[j]=i==j;
  for (i=0;i<6;i++)
    for (j=0;j<=i;j++)
    {
      sum.clear();
      for (k=0;k<qsz;k++)
	for (l=0;l<qsz;l++)
	  if (std::isfinite(qpoints[k][l]))
	    sum+=unitquad[i].undulation(qscale(k,qsz),qscale(l,qsz))
	        *unitquad[j].undulation(qscale(k,qsz),qscale(l,qsz));
      ret[i][j]=ret[j][i]=sum.total();
    }
  return ret;
}

void dump256(double qpoints[][16],int qsz)
{
  int i,j;
  for (i=0;i<qsz;i++)
  {
    for (j=0;j<qsz;j++)
      if (std::isfinite(qpoints[i][j]))
	cout<<" *";
      else
	cout<<" -";
    cout<<endl;
  }
}

double qscale(int i,int qsz)
/* Input: i is in [0,qsz-1]
 * Output: in (-1,1)
 */
{
  return (2*i+1-qsz)/(double)qsz;
}

array<double,6> correction(geoquad &quad,double qpoints[][16],int qsz)
{
  array<double,6> ret;
  matrix preret(6,1);
  int i,j,k,qhash;
  double diff;
  geoquad unitquad;
  qhash=quadhash(qpoints,qsz);
  if (quadinv.count(qhash)==0)
    quadinv[qhash]=invert(autocorr(qpoints,qsz));
  for (i=0;i<6;i++)
    ret[i]=0;
  for (i=0;i<qsz;i++)
    for (j=0;j<qsz;j++)
      if (std::isfinite(qpoints[i][j]))
      {
	diff=qpoints[i][j]-quad.undulation(qscale(i,qsz),qscale(j,qsz));
	for (k=0;k<6;k++)
	{
	  unitquad.und[k]=1;
	  unitquad.und[(k+5)%6]=0;
	  preret[k][0]+=diff*unitquad.undulation(qscale(i,qsz),qscale(j,qsz));
	}
      }
  /*ret[0]=preret[0][0]/256;
  ret[1]=preret[1][0]/85;
  ret[2]=preret[2][0]/85;
  ret[3]=preret[3][0]*2304/51409;
  ret[4]=preret[4][0]*256/7225;
  ret[5]=preret[5][0]*2304/51409;*/
  preret=quadinv[qhash]*preret;
  for (i=0;i<6;i++)
    ret[i]=preret[i][0];
  return ret;
}

int quadhash(double qpoints[][16],int qsz)
/* Used to remember inverses of matrices for patterns of points in a geoquad
 * inside and outside the area being converted. Most of them can be formed by
 * running a straight line through a 16×16 lattice of points and taking all
 * those on one side. There are 20173 such patterns, all of which have different
 * hashes. This fills the hash table only 0.0000276, so other patterns will
 * probably not collide with them.
 */
{
  int i,j,ret;
  for (ret=i=0;i<qsz;i++)
    for (j=0;j<qsz;j++)
      if (std::isfinite(qpoints[i][j]))
	ret=(2*ret)%HASHPRIME;
      else
	ret=(2*ret+1)%HASHPRIME;
  return ret;
}

double maxerror(geoquad &quad,double qpoints[][16],int qsz)
{
  double ret=0;
  int i,j;
  double diff;
  geoquad unitquad;
  for (i=0;i<qsz;i++)
    for (j=0;j<qsz;j++)
      if (std::isfinite(qpoints[i][j]))
      {
	diff=fabs(qpoints[i][j]-quad.undulation(qscale(i,qsz),qscale(j,qsz)));
	if (diff>ret)
	  ret=diff;
      }
  //cout<<"maxerror "<<ret<<endl;
  return ret;
}
