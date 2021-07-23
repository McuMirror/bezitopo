/******************************************************/
/*                                                    */
/* arc.cpp - horizontal circular arcs                 */
/*                                                    */
/******************************************************/
/* Copyright 2012-2017,2020,2021 Pierre Abbat.
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

#include <cstdio>
#include <cfloat>
#include "point.h"
#include "arc.h"
#include "vcurve.h"
#include "spiral.h"

using namespace std;

arc::arc()
{
  start=end=xyz(0,0,0);
  rchordbearing=control1=control2=delta=0;
}

arc::arc(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
  delta=0;
  control1=(2*start.elev()+end.elev())/3;
  control2=(start.elev()+2*end.elev())/3;
  rchordbearing=atan2(end.north()-start.north(),end.east()-start.east());
}

arc::arc(xyz kra,xyz mij,xyz fam)
{
  double p,q,r;
  start=kra;
  end=fam;
  delta=2*(atan2i(fam-mij)-atan2i(mij-kra));
  if (delta)
    p=(double)(2*(atan2i(fam-mij)-atan2i(fam-kra)))/delta;
  else
    p=dist(xy(kra),xy(mij))/dist(xy(kra),xy(fam));
  q=1-p;
  r=(mij.elev()-start.elev()-p*(end.elev()-start.elev()))/(p*q)/3;
  control1=(2*start.elev()+end.elev())/3+r;
  control2=(start.elev()+2*end.elev())/3+r;
  rchordbearing=atan2(end.north()-start.north(),end.east()-start.east());
}

arc::arc(xyz kra,xyz fam,int d)
{
  start=kra;
  end=fam;
  delta=d;
  control1=(2*start.elev()+end.elev())/3;
  control2=(start.elev()+2*end.elev())/3;
  rchordbearing=atan2(end.north()-start.north(),end.east()-start.east());
}

int arc::type()
{
  return OBJ_ARC;
}

void arc::setdelta(int d,int s) // s is for spirals and is ignored for circular arcs
{
  delta=d;
}

void arc::setcurvature(double startc,double endc)
{
  double sinhalfdelta=(startc+endc)/4*chordlength();
  if (fabs(sinhalfdelta)>1)
    delta=DEG360;
  else
    delta=twiceasini(sinhalfdelta);
}

xy arc::center()
{
  return ((xy(start)+xy(end))/2+turn90((xy(end)-xy(start))/2/tanhalf(delta)));
}

double arc::length() const
{
  if (delta)
    return chordlength()*bintorad(delta)/sinhalf(delta)/2;
  else
    return chordlength();
}

double arc::epsilon() const
{
  return sqrt((sqr(start.getx())+sqr(start.gety())+
	       sqr(end.getx())+sqr(end.gety()))/2)*DBL_EPSILON/
	 cosquarter(delta);
}

xy arc::pointOfIntersection()
{
  return ((xy(start)+xy(end))/2-turn90((xy(end)-xy(start))/2*tanhalf(delta)));
}

double arc::tangentLength(int which)
{
  return chordlength()/2/coshalf(delta);
}

double arc::diffarea()
{
  double r;
  if (delta)
  {
    r=radius(0);
    return r*r*(bintorad(delta)-sin(delta))/2;
    //FIXME fix numerical stability for small delta
  }
  else
    return 0;
}

xyz arc::station(double along) const
{
  double gnola,len,angalong,rdelta;
  len=length();
  if (delta)
  {
    rdelta=bintorad(delta);
    angalong=along/len*bintorad(delta);
    gnola=len-along;
    //printf("arc::station angalong=%f startbearing=%f\n",bintodeg(angalong),bintodeg(startbearing()));
    return xyz(xy(start)+cossin((angalong-rdelta)/2+rchordbearing)*sin(angalong/2)*radius(0)*2,
	      elev(along));
  }
  else
    return segment::station(along);
}

int arc::bearing(double along) const
{
  double len=length();
  int angalong;
  angalong=lrint((along/len-0.5)*delta);
  return chordbearing()+angalong;
}

bool arc::isCurly()
{
  return delta>=DEG180 || delta==DEG360;
}

bool arc::isTooCurly()
{
  return delta==DEG360;
}

void arc::split(double along,arc &a,arc &b)
{
  double dummy;
  int deltaa,deltab;
  xyz splitpoint=station(along);
  deltaa=lrint(delta*along/length());
  deltab=delta-deltaa;
  a=arc(start,splitpoint,deltaa);
  b=arc(splitpoint,end,deltab);
  vsplit(start.elev(),control1,control2,end.elev(),along/length(),a.control1,a.control2,dummy,b.control1,b.control2);
  //printf("split: %f,%f\n",a.end.east(),a.end.north());
}

void arc::lengthen(int which,double along)
/* Lengthens or shortens the arc, moving the specified end.
 * Used for extend, trim, trimTwo, and fillet (trimTwo is fillet with radius=0).
 */
{
  double oldSlope,newSlope=slope(along);
  double oldLength=length();
  double oldCurvature=curvature(0);
  xyz newEnd=station(along);
  if (which==START)
  {
    oldSlope=endslope();
    start=newEnd;
    delta=radtobin((oldLength-along)*oldCurvature);
    setslope(START,newSlope);
    setslope(END,oldSlope);
  }
  if (which==END)
  {
    oldSlope=startslope();
    end=newEnd;
    delta=radtobin(along*oldCurvature);
    setslope(END,newSlope);
    setslope(START,oldSlope);
  }
  rchordbearing=atan2(end.north()-start.north(),end.east()-start.east());
}

double arc::in(xy pnt)
{
  int beardiff;
  double ret=NAN;
  beardiff=2*(foldangle(dir(pnt,end)-dir(start,pnt)));
  if (pnt==start || pnt==end)
    ret=bintorot(delta)/2;
  if (std::isnan(ret) && delta && (abs(foldangle(beardiff-delta))<2 || beardiff==0))
  {
    spiralarc spi(*this);
    ret=spi.in(pnt); // This can return NaN on MSVC
  }
  if (std::isnan(ret))
    ret=(beardiff>0)+(beardiff>=0)-(beardiff>delta)-(beardiff>=delta);
  return ret;
}

/* To find the nearest point on the arc to a point:
   If delta is less than 0x1000000 (2°48'45") in absolute value, use linear
   interpolation to find a starting point. If it's between 0x1000000 and
   0x40000000 (180°), use the bearing from the center. Between 0x40000000
   and 0x7f000000 (357°11'15"), use the bearing from the center, but use
   calong() instead of along(). From 0x7f000000 to 0x80000000, use linear
   interpolation and calong(). Then use parabolic interpolation to find
   the closest point on the circle.
   */

string formatCurvature(double curvature,Measure ms,double precisionMagnitude)
/* The coherent unit of curvature is the diopter (not an SI unit, but coherent
 * with SI). For roads, the millidiopter is closer to the size. When roads
 * are measured in feet, however, curvature is expressed not in per feet,
 * but by stating the angle subtended by a 100-foot arc. Railroads use
 * a 100-foot chord, which I may add later (it'll require a flag somewhere).
 */
{
  double hundredFeet=ms.parseMeasurement("100 ft",LENGTH).magnitude;
  bool isFoot;
  string hundredFeetString=ms.formatMeasurement(hundredFeet,LENGTH);
  int i;
  for (i=0;i<hundredFeetString.length();i++)
  {
    if (hundredFeetString[i]=='1')
    {
      isFoot=true;
      break;
    }
    if (hundredFeetString[i]=='3')
    {
      isFoot=false;
      break;
    }
  }
  if (isFoot)
    return ms.formatMeasurementUnit(curvature*hundredFeet,ANGLE,0,precisionMagnitude*hundredFeet);
  else
    return ms.formatMeasurementUnit(curvature,CURVATURE,0,precisionMagnitude);
}

double parseCurvature(string curString,Measure ms)
/* Parses a curvature. If ms is in feet, accepts an angle and interprets it
 * as the angle subtended by a 100 ft arc. If ms is in meters or the input
 * is not an angle, parses it as a curvature. Can throw badUnits or badNumber.
 */
{
  double hundredFeet=ms.parseMeasurement("100 ft",LENGTH).magnitude;
  bool isFoot;
  string hundredFeetString=ms.formatMeasurement(hundredFeet,LENGTH);
  int i;
  double ret=NAN;
  Measurement meas;
  meas.unit=0;
  for (i=0;i<hundredFeetString.length();i++)
  {
    if (hundredFeetString[i]=='1')
    {
      isFoot=true;
      break;
    }
    if (hundredFeetString[i]=='3')
    {
      isFoot=false;
      break;
    }
  }
  if (isFoot)
    try
    {
      meas=ms.parseMeasurement(curString,ANGLE);
      ret=meas.magnitude/hundredFeet;
    }
    catch (...)
    {
    }
  if (!meas.unit)
    {
      meas=ms.parseMeasurement(curString,CURVATURE);
      ret=meas.magnitude;
    }
  return ret;
}
