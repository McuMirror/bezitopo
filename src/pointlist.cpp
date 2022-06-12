/******************************************************/
/*                                                    */
/* pointlist.cpp - list of points                     */
/*                                                    */
/******************************************************/
/* Copyright 2012-2013,2015-2019,2022 Pierre Abbat.
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

#include <cmath>
#include "angle.h"
#include "globals.h"
#include "pointlist.h"
#include "csv.h"
#include "breakline.h"
#include "ldecimal.h"
#include "except.h"
#include "stl.h"
#include "dxf.h"

using namespace std;

criterion::criterion()
{
  lo=hi=0;
  elo=ehi=NAN;
  istopo=false;
}

void criterion::clear()
{
  lo=hi=0;
  elo=ehi=NAN;
  istopo=false;
}

bool criterion::match(point &pnt,int num)
{
  return (str.length()==0 || pnt.note.find(str)!=string::npos) &&
    ((lo==0 && hi==0) || (num>=lo && num<=hi)) && ((std::isnan(elo) || std::isnan(ehi)) || (pnt.elev()>=elo && pnt.elev()<=ehi));
}

void criterion::writeXml(ostream &ofile)
{
  ofile<<"<Criterion pointRange=\""<<lo<<':'<<hi;
  ofile<<"\" string=\""<<xmlEscape(str);
  ofile<<"\" elevRange=\""<<ldecimal(elo)<<':'<<ldecimal(ehi);
  ofile<<"\" topo=\""<<istopo;
  ofile<<"\"/>"<<endl;
}

pointlist::pointlist()
{
  initStlTable();
}

void pointlist::clear()
{
  contours.clear();
  triangles.clear();
  edges.clear();
  points.clear();
  revpoints.clear();
  triPolyLog.clear();
}

void pointlist::clearTin()
{
  triangles.clear();
  edges.clear();
}

int pointlist::size()
{
  return points.size();
}

void pointlist::clearmarks()
{
  map<int,edge>::iterator e;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.clearmarks();
}

int symhash(int a,int b)
/* symhash(a,b)=symhash(b,a). Otherwise similar to skewsym.
 */
{
  int a1,a2,b1,b2;
  a1=a*0x69969669;
  a2=a*PHITURN;
  b1=b*0x69969669;
  b2=b*PHITURN;
  a1=((a1&0xffe00000)>>21)|(a1&0x1ff800)|((a1&0x7ff)<<21);
  a2=((a2&0xffff0000)>>16)|((a2&0xffff)<<16);
  b1=((b1&0xffe00000)>>21)|(b1&0x1ff800)|((b1&0x7ff)<<21);
  b2=((b2&0xffff0000)>>16)|((b2&0xffff)<<16);
  return (a*b1*a2)^(b*a1*b2);
}


bool pointlist::checkTinConsistency()
{
  bool ret=true;
  int i,j,n,nInteriorEdges=0,nNeighborTriangles=0,turn1;
  double a;
  long long totturn;
  ptlist::iterator p;
  vector<int> edgebearings;
  array<int,2> orderedEdge;
  map<int,vector<array<int,2> > > edgeHash;
  map<int,vector<array<int,2> > >::iterator eh;
  edge *ed;
  for (p=points.begin();p!=points.end();p++)
  {
    ed=p->second.line;
    if (ed==nullptr || (ed->a!=&p->second && ed->b!=&p->second))
    {
      ret=false;
      cerr<<"Point "<<p->first<<" line pointer is wrong.\n";
    }
    edgebearings.clear();
    do
    {
      if (ed)
	ed=ed->next(&p->second);
      if (ed)
	edgebearings.push_back(ed->bearing(&p->second));
    } while (ed && ed!=p->second.line && edgebearings.size()<=edges.size());
    if (edgebearings.size()>=edges.size())
    {
      ret=false;
      cerr<<"Point "<<p->first<<" next pointers do not return to line pointer.\n";
    }
    for (totturn=i=0;i<edgebearings.size();i++)
    {
      turn1=(edgebearings[(i+1)%edgebearings.size()]-edgebearings[i])&(DEG360-1);
      totturn+=turn1;
      if (turn1==0)
      {
	ret=false;
	cerr<<"Point "<<p->first<<" has two equal bearings.\n";
      }
    }
    if (totturn!=(long long)DEG360) // DEG360 is construed as positive when cast to long long
    {
      ret=false;
      cerr<<"Point "<<p->first<<" bearings do not wind once counterclockwise.\n";
    }
  }
  for (i=0;i<edges.size();i++)
  {
    if (edges[i].isinterior())
      nInteriorEdges++;
    if ((edges[i].tria!=nullptr)+(edges[i].trib!=nullptr)!=1+edges[i].isinterior())
    {
      ed=&edges[i];
      ret=false;
      cerr<<"Edge "<<i<<" has wrong number of adjacent triangles.\n";
      cerr<<"a "<<revpoints[ed->a]<<" b "<<revpoints[ed->b]<<endl;
      cerr<<"tria "<<ed->tria<<" trib "<<ed->trib<<" isinterior "<<ed->isinterior()<<endl;
    }
    if (edges[i].tria)
    {
      a=n=0;
      if (edges[i].tria->a==edges[i].a || edges[i].tria->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->a);
      if (edges[i].tria->b==edges[i].a || edges[i].tria->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->b);
      if (edges[i].tria->c==edges[i].a || edges[i].tria->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->c);
      if (n!=2)
      {
        ret=false;
        cerr<<"Edge "<<i<<" triangle a does not have edge as a side.\n";
      }
      if (a>=0)
      {
        ret=false;
        cerr<<"Edge "<<i<<" triangle a is on the wrong side.\n";
      }
    }
    if (edges[i].trib)
    {
      a=n=0;
      if (edges[i].trib->a==edges[i].a || edges[i].trib->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->a);
      if (edges[i].trib->b==edges[i].a || edges[i].trib->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->b);
      if (edges[i].trib->c==edges[i].a || edges[i].trib->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->c);
      if (n!=2)
      {
        ret=false;
        cerr<<"Edge "<<i<<" triangle b does not have edge as a side.\n";
      }
      if (a<=0)
      {
        ret=false;
        cerr<<"Edge "<<i<<" triangle b is on the wrong side.\n";
      }
    }
  }
  for (i=0;i<triangles.size();i++)
  {
    if (triangles[i].aneigh)
    {
      nNeighborTriangles++;
      if ( triangles[i].aneigh->iscorner(triangles[i].a) ||
          !triangles[i].aneigh->iscorner(triangles[i].b) ||
          !triangles[i].aneigh->iscorner(triangles[i].c))
      {
        ret=false;
        cerr<<"Triangle "<<i<<" neighbor a is wrong.\n";
      }
    }
    if (triangles[i].bneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].bneigh->iscorner(triangles[i].a) ||
           triangles[i].bneigh->iscorner(triangles[i].b) ||
          !triangles[i].bneigh->iscorner(triangles[i].c))
      {
        ret=false;
        cerr<<"Triangle "<<i<<" neighbor b is wrong.\n";
      }
    }
    if (triangles[i].cneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].cneigh->iscorner(triangles[i].a) ||
          !triangles[i].cneigh->iscorner(triangles[i].b) ||
           triangles[i].cneigh->iscorner(triangles[i].c))
      {
        ret=false;
        cerr<<"Triangle "<<i<<" neighbor c is wrong.\n";
      }
    }
    /* Checks whether two triangles share an edge in the same direction.
     * This is less stringent than the edge check in readPtin, which requires
     * that another triangle have the same edge in the opposite direction,
     * unless the edge is in the convex hull.
     * It is possible for this to fail even if the rest of checkTinConsistency passes.
     * For example, arrange points 1-8 counterclockwise and make these triangles:
     * (1 2 3), (1 2 4), (1 4 5), (1 5 6), (1 6 7), (1 7 8).
     */
    orderedEdge[0]=revpoints[triangles[i].a];
    orderedEdge[1]=revpoints[triangles[i].b];
    edgeHash[symhash(orderedEdge[0],orderedEdge[1])].push_back(orderedEdge);
    orderedEdge[0]=revpoints[triangles[i].b];
    orderedEdge[1]=revpoints[triangles[i].c];
    edgeHash[symhash(orderedEdge[0],orderedEdge[1])].push_back(orderedEdge);
    orderedEdge[0]=revpoints[triangles[i].c];
    orderedEdge[1]=revpoints[triangles[i].a];
    edgeHash[symhash(orderedEdge[0],orderedEdge[1])].push_back(orderedEdge);
  }
  for (eh=edgeHash.begin();eh!=edgeHash.end();eh++)
    for (i=1;i<eh->second.size();i++)
      for (j=0;j<i;j++)
	if (eh->second[i][0]==eh->second[j][0] && eh->second[i][1]==eh->second[j][1])
	{
	  ret=false;
	  cerr<<"Two triangles have edge "<<eh->second[i][0]<<"->"<<eh->second[i][1]<<" in common.\n";
	}
  if (nInteriorEdges*2!=nNeighborTriangles)
  {
    ret=false;
    cerr<<"Interior edges and neighbor triangles don't match.\n";
  }
  return ret;
}

bool pointlist::checkFlower()
/* Checks whether the points are in a flower used in the tripolygon test.
 * Something has been messing with the points during the test.
 */
{
  int i,sz=points.size(),bear;
  point *pnt;
  double mulbear;
  bool ret=true;
  for (i=0;i<sz;i++)
  {
    pnt=&points[i+1];
    bear=atan2i(*pnt);
    mulbear=bintorot(bear)*sz;
    if (mulbear<0)
      mulbear+=sz;
    if (fabs(mulbear-i)>0.01)
    {
      cout<<"Point "<<i+1<<" of "<<sz<<" bearing is "<<bintodeg(bear)<<" ("<<mulbear<<"), should be "<<36e1*i/sz<<endl;
      ret=false;
    }
  }
  return ret;
}

bool pointlist::shouldWrite(int n,int flags,bool contours)
// Stub function. See PerfectTIN.
{
  return true;
}

void pointlist::logTriPoly(vector<point *> loop,int a,int b,int c)
{
  TriPolyLogEntry entry;
  entry.loop=loop;
  entry.tri[0]=a;
  entry.tri[1]=b;
  entry.tri[2]=c;
  triPolyLog.push_back(entry);
}

int1loop pointlist::toInt1loop(vector<point *> ptrLoop)
{
  int i;
  int1loop ret;
  for (i=0;i<ptrLoop.size();i++)
    ret.push_back(revpoints[ptrLoop[i]]);
  return ret;
}

vector<point *> pointlist::fromInt1loop(int1loop intLoop)
{
  int i;
  vector<point *> ret;
  for (i=0;i<intLoop.size();i++)
    ret.push_back(&points[intLoop[i]]);
  return ret;
}

intloop pointlist::boundary()
/* The boundary is traced *clockwise*, with the triangles on the right,
 * so that, after combining with the convex hull, it will consist of
 * counterclockwise loops, which can then be triangulated.
 */
{
  int i;
  edge *e;
  intloop ret;
  int1loop bdy1;
  for (i=0;i<edges.size();i++)
  {
    if (!edges[i].tria)
      edges[i].reverse();
    edges[i].contour=0;
  }
  for (i=0;i<edges.size();i++)
    if (!edges[i].contour && !edges[i].trib)
    {
      bdy1.clear();
      e=&edges[i];
      while (!e->contour)
      {
	bdy1.push_back(revpoints[e->a]);
	e->contour++;
	e=e->nexta;
      }
      bdy1.reverse();
      ret.push_back(bdy1);
    }
  return ret;
}

int pointlist::readCriteria(string fname,Measure ms)
{
  ifstream infile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,ncrit;
  criterion crit1;
  vector<string> words;
  string line,minstr,maxstr,eminstr,emaxstr,d,instr;
  infile.open(fname);
  ncrit=-(!infile.is_open());
  if (infile.is_open())
  {
    crit.clear();
    do
    {
      getline(infile,line);
      while (line.back()=='\n' || line.back()=='\r')
	line.pop_back();
      words=parsecsvline(line);
      if (words.size()==6)
      {
	crit1.clear();
	minstr=words[0];
	maxstr=words[1];
	eminstr=words[2];
	emaxstr=words[3];
	d=words[4];
	instr=words[5];
	try
	{
	  if (minstr.length())
            crit1.lo=stoi(minstr);
	  if (maxstr.length())
            crit1.hi=stoi(maxstr);
          if (eminstr.length())
            crit1.elo=ms.parseMeasurement(eminstr,LENGTH).magnitude;
          if (emaxstr.length())
            crit1.ehi=ms.parseMeasurement(emaxstr,LENGTH).magnitude;
          crit1.str=d;
          crit1.istopo=atoi(instr.c_str())!=0;
          crit.push_back(crit1);
	  ncrit++;
	}
	catch (...)
	{
	  cerr<<"Couldn't parse numbers in line: "<<line<<endl;
	}
	//puts(d.c_str());
      }
      else if (words.size()==0 || (words.size()==1 && words[0].length() && words[0][0]<32))
	; // blank line or end-of-file character
      else
	cerr<<"Ignored line: "<<line<<endl;
    } while (infile.good());
    infile.close();
  }
  return ncrit;
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

int pointlist::addtriangle(int n)
{
  int i;
  int newTriNum=triangles.size();
  for (i=0;i<n;i++)
  {
    triangles[newTriNum+i].sarea=0;
    //revtriangles[&triangles[newTriNum+i]]=newTriNum+i;
  }
  return newTriNum;
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
  if (triangles.size())
    qinx.settri(&triangles[0]);
}

void pointlist::updateqindex()
/* Use this when you already have a quad index, split to cover all the points,
 * but the leaves don't point to the right triangles because you've flipped
 * some edges.
 */
{
  if (triangles.size())
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

void pointlist::findedgecriticalpts()
{
  map<int,edge>::iterator e;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.findextrema();
}

void pointlist::findcriticalpts()
{
  map<int,triangle>::iterator t;
  findedgecriticalpts();
  for (t=triangles.begin();t!=triangles.end();t++)
  {
    t->second.findcriticalpts();
    t->second.subdivide();
  }
}

void pointlist::addperimeter()
{
  map<int,triangle>::iterator t;
  cout<<"Adding perimeter to "<<triangles.size()<<" triangles\n";
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

bool pointlist::join2break0()
/* Joins two fragments of type-0 breakline and returns true,
 * or returns false if there are none that can be joined.
 */
{
  int i,j;
  int sz=type0Breaklines.size();
  Breakline0 cat;
  for (i=0;i<sz;i++)
    for (j=i+1;j<sz;j++)
      if (jungible(type0Breaklines[i],type0Breaklines[j]))
        goto jung;
  jung:
  if (i<sz && j<sz)
  {
    cat=type0Breaklines[i]+type0Breaklines[j];
    type0Breaklines[j]=cat;
    while (j+1<sz && type0Breaklines[j].size()>type0Breaklines[j+1].size())
    {
      swap(type0Breaklines[j],type0Breaklines[j+1]);
      j++;
    }
    while (i+1<sz)
    {
      swap(type0Breaklines[i],type0Breaklines[i+1]);
      i++;
    }
    type0Breaklines.resize(sz-1);
    return true;
  }
  else
    return false;
}

void pointlist::joinBreaklines()
{
  while (join2break0());
}

void pointlist::edgesToBreaklines()
{
  int i;
  type0Breaklines.clear();
  for (i=0;i<edges.size();i++)
    if (!edges[i].delaunay() || (edges[i].broken&1))
      type0Breaklines.push_back(Breakline0(revpoints[edges[i].a],revpoints[edges[i].b]));
  joinBreaklines();
  whichBreak0Valid=3;
}

void pointlist::stringToBreakline(string line)
/* Insert one line read from a breakline file into the breaklines.
 * Comments begin with '#'. Blank lines are ignored.
 * Type-0 breaklines look like "5-6-7-8-9-5".
 * Type-1 breaklines look like "1,0;-.5,.866;-.5,.866;1,0".
 * Can throw badBreaklineFormat.
 */
{
  size_t hashpos=line.find('#');
  vector<string> lineWords;
  int i;
  if (hashpos<line.length())
    line.erase(hashpos);
  if (line.length())
  {
    if (line.find(',')<line.length())
      ; // type-1 is not implemented yet
    else
      type0Breaklines.push_back(Breakline0(parseBreakline(line,'-')));
  }
}

void pointlist::readBreaklines(string filename)
{
  fstream file(filename,fstream::in);
  string line;
  type0Breaklines.clear();
  while (!file.eof() && !file.fail())
  {
    getline(file,line);
    stringToBreakline(line);
  }
  if (!file.eof())
    throw BeziExcept(fileError);
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

string pointlist::hitTestPointString(xy pnt,double radius)
{
  ptlist::iterator i;
  string ret;
  for (i=points.begin();i!=points.end();++i)
  {
    if (dist(i->second,pnt)<radius)
    {
      if (ret.length())
        ret+=' ';
      ret+=to_string(i->first)+' '+i->second.note;
    }
  }
  return ret;
}

void pointlist::addIfIn(triangle *t,set<triangle *> &addenda,xy pnt,double radius)
{
  if (t && !localTriangles.count(t) && t->inCircle(pnt,radius))
    addenda.insert(t);
}

void pointlist::setLocalSets(xy pnt,double radius)
/* If the sets are set to {nullptr}, this means one of two things:
 * • The area in the window is too large; it would be faster to loop through
 *   all the edges.
 * • There are no triangles. A qindex is an index of triangles.
 * An empty qindex would produce {}, so this condition has to be checked.
 */
{
  set<point *>::iterator i;
  set<edge *>::iterator j;
  set<triangle *>::iterator k;
  int n;
  vector<edge *> pointEdges;
  set<triangle *> addenda;
  localTriangles.clear();
  localEdges.clear();
  localPoints.clear();
  if (triangles.size())
    localTriangles=qinx.localTriangles(pnt,radius,triangles.size()/64+100);
  else
    localTriangles.insert(nullptr);
  if (localTriangles.count(nullptr))
  {
    localEdges.insert(nullptr);
    localPoints.insert(nullptr);
    //cout<<"No triangles or view is too big\n";
  }
  else
  {
    do
    {
      addenda.clear();
      for (k=localTriangles.begin();k!=localTriangles.end();++k)
      {
	addIfIn((*k)->aneigh,addenda,pnt,radius);
	addIfIn((*k)->bneigh,addenda,pnt,radius);
	addIfIn((*k)->cneigh,addenda,pnt,radius);
      }
      for (k=addenda.begin();k!=addenda.end();++k)
	localTriangles.insert(*k);
    } while (addenda.size());
    for (k=localTriangles.begin();k!=localTriangles.end();++k)
    {
      localPoints.insert((*k)->a);
      localPoints.insert((*k)->b);
      localPoints.insert((*k)->c);
    }
    for (i=localPoints.begin();i!=localPoints.end();++i)
    {
      pointEdges=(*i)->incidentEdges();
      for (n=0;n<pointEdges.size();n++)
	localEdges.insert(pointEdges[n]);
    }
    for (j=localEdges.begin();j!=localEdges.end();++j)
    { // localTriangles() usually doesn't find all triangles, and may even miss a point.
      if ((*j)->tria)
	localTriangles.insert((*j)->tria);
      if ((*j)->trib)
	localTriangles.insert((*j)->trib);
      localPoints.insert((*j)->a);
      localPoints.insert((*j)->b);
    }
    assert(!localPoints.count(nullptr));
    assert(!localEdges.count(nullptr));
    assert(!localTriangles.count(nullptr));
    //cout<<localPoints.size()<<" points "<<localEdges.size()<<" edges "<<localTriangles.size()<<" triangles\n";
  }
}

void pointlist::writeXml(ofstream &ofile)
{
  int i;
  ptlist::iterator p;
  map<int,triangle>::iterator t;
  ofile<<"<Pointlist><Criteria>";
  for (i=0;i<crit.size();i++)
    crit[i].writeXml(ofile);
  ofile<<"</Criteria><Points>";
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
  ofile<<"<Breaklines>";
  for (i=0;i<type0Breaklines.size();i++)
    type0Breaklines[i].writeXml(ofile);
  ofile<<"</Breaklines>";
  contourInterval.writeXml(ofile);
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

