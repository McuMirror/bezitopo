/******************************************************/
/*                                                    */
/* pointlist.cpp - list of points                     */
/*                                                    */
/******************************************************/
/* Copyright 2012,2013,2015,2016,2017 Pierre Abbat.
 * This file is part of Bezitopo.
 * 
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bezitopo. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include "angle.h"
#include "bezitopo.h"
#include "pointlist.h"

using namespace std;

//vector<pointlist> pointlists;

void pointlist::clear()
{
  points.clear();
  revpoints.clear();
}

void pointlist::clearmarks()
{
  map<int,edge>::iterator e;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.clearmarks();
}

bool pointlist::checkTinConsistency()
{
  bool ret=true;
  int i;
  long long totturn;
  ptlist::iterator p;
  vector<int> edgebearings;
  edge *ed;
  for (p=points.begin();p!=points.end();p++)
  {
    ed=p->second.line;
    if (ed->a!=&p->second && ed->b!=&p->second)
    {
      ret=false;
      cerr<<"Point "<<p->first<<" line pointer is wrong.\n";
    }
    edgebearings.clear();
    do
    {
      ed=ed->next(&p->second);
      edgebearings.push_back(ed->getsegment().chordbearing()+DEG180*(ed->b==&p->second));
    } while (ed!=p->second.line && edgebearings.size()<=edges.size());
    if (edgebearings.size()>=edges.size())
    {
      ret=false;
      cerr<<"Point "<<p->first<<" next pointers do not return to line pointer.\n";
    }
    for (totturn=i=0;i<edgebearings.size();i++)
      totturn+=(edgebearings[(i+1)%edgebearings.size()]-edgebearings[i])&(DEG360-1);
    if (totturn!=(long long)DEG360) // DEG360 is construed as positive when cast to long long
    {
      ret=false;
      cerr<<"Point "<<p->first<<" bearings do not wind once counterclockwise.\n";
    }
  }
  return ret;
}

void pointlist::addpoint(int numb,point pnt,bool overwrite)
// If numb<0, it's a point added by bezitopo.
{int a;
 if (points.count(numb))
    if (overwrite)
       points[a=numb]=pnt;
    else
       {if (numb<0)
           {a=points.begin()->first-1;
            if (a>=0)
               a=-1;
            }
        else
           {a=points.rbegin()->first+1;
            if (a<=0)
               a=1;
            }
        points[a]=pnt;
        }
 else
    points[a=numb]=pnt;
 revpoints[&(points[a])]=a;
 }

void pointlist::makeqindex()
{
  vector<xy> plist;
  ptlist::iterator i;
  qinx.clear();
  for (i=points.begin();i!=points.end();i++)
    plist.push_back(i->second);
  qinx.sizefit(plist);
  qinx.split(plist);
  qinx.settri(&triangles[0]);
}

double pointlist::elevation(xy location)
{
  triangle *t;
  t=qinx.findt(location);
  if (t)
    return t->elevation(location);
  else
    return nan("");
}

void pointlist::setgradient(bool flat)
{
  int i;
  for (i=0;i<triangles.size();i++)
    if (flat)
      triangles[i].flatten();
    else
    {
      triangles[i].setgradient(*triangles[i].a,triangles[i].a->gradient);
      triangles[i].setgradient(*triangles[i].b,triangles[i].b->gradient);
      triangles[i].setgradient(*triangles[i].c,triangles[i].c->gradient);
      triangles[i].setcentercp();
    }
}

double pointlist::dirbound(int angle)
/* angle=0x00000000: returns least easting.
 * angle=0x20000000: returns least northing.
 * angle=0x40000000: returns negative of greatest easting.
 */
{
  ptlist::iterator i;
  double bound=HUGE_VAL,turncoord;
  double s=sin(angle),c=cos(angle);
  for (i=points.begin();i!=points.end();i++)
  {
    turncoord=i->second.east()*c+i->second.north()*s;
    if (turncoord<bound)
      bound=turncoord;
  }
  return bound;
}

void pointlist::findcriticalpts()
{
  map<int,edge>::iterator e;
  map<int,triangle>::iterator t;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.findextrema();
  for (t=triangles.begin();t!=triangles.end();t++)
  {
    t->second.findcriticalpts();
    t->second.subdivide();
  }
}

void pointlist::addperimeter()
{
  map<int,triangle>::iterator t;
  for (t=triangles.begin();t!=triangles.end();t++)
    t->second.addperimeter();
}

void pointlist::removeperimeter()
{
  map<int,triangle>::iterator t;
  for (t=triangles.begin();t!=triangles.end();t++)
    t->second.removeperimeter();
}

triangle *pointlist::findt(xy pnt,bool clip)
{
  return qinx.findt(pnt,clip);
}

string pointlist::hitTestString(triangleHit hit)
{
  string ret;
  if (hit.cor)
    ret=to_string(revpoints[hit.cor])+' '+hit.cor->note;
  if (hit.edg)
    ret=to_string(revpoints[hit.edg->a])+'-'+to_string(revpoints[hit.edg->b]);
  if (hit.tri)
    ret='('+to_string(revpoints[hit.tri->a])+' '+
        to_string(revpoints[hit.tri->b])+' '+
        to_string(revpoints[hit.tri->c])+')';
  return ret;
}

void pointlist::writeXml(ofstream &ofile)
{
  int i;
  ptlist::iterator p;
  map<int,triangle>::iterator t;
  ofile<<"<Pointlist>";
  ofile<<"<Points>";
  for (p=points.begin(),i=0;p!=points.end();p++,i++)
  {
    if (i && (i%1)==0)
      ofile<<endl;
    p->second.writeXml(ofile,*this);
  }
  ofile<<"</Points>"<<endl;
  ofile<<"<TIN>";
  for (t=triangles.begin(),i=0;t!=triangles.end();t++,i++)
  {
    if (i && (i%1)==0)
      ofile<<endl;
    t->second.writeXml(ofile,*this);
  }
  ofile<<"</TIN>"<<endl;
  ofile<<"<Contours>";
  for (i=0;i<contours.size();i++)
    contours[i].writeXml(ofile);
  ofile<<"</Contours>";
  ofile<<"</Pointlist>"<<endl;
}

void pointlist::roscat(xy tfrom,int ro,double sca,xy tto)
{
  xy cs=cossin(ro);
  int i;
  ptlist::iterator j;
  for (i=0;i<contours.size();i++)
    contours[i]._roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
  for (j=points.begin();j!=points.end();j++)
    j->second._roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
}

