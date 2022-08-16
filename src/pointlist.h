/******************************************************/
/*                                                    */
/* pointlist.h - list of points                       */
/*                                                    */
/******************************************************/
/* Copyright 2012-2019,2022 Pierre Abbat.
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
#ifndef POINTLIST_H
#define POINTLIST_H

#include <map>
#include <string>
#include <vector>
#include <array>
#include <set>
#include "point.h"
#include "tin.h"
#include "bezier.h"
#include "qindex.h"
#include "polyline.h"
#include "contour.h"
#include "breakline.h"
#include "intloop.h"

#ifdef _MSC_VER
typedef long long ssize_t;
#endif

typedef std::map<int,point> ptlist;
typedef std::map<point*,int> revptlist;

class criterion
{
public:
  criterion();
  void clear();
  bool match(point &pnt,int num);
  std::string str;
  int lo,hi; // point number range
  double elo,ehi; // elevation range
  bool istopo;
  void writeXml(std::ostream &ofile);
};

typedef std::vector<criterion> criteria;
/* The last criterion that matches the point determines whether the point
 * is included in the topo. If none matches, it is not included.
 */

struct TriPolyLogEntry
{
  std::vector<point *> loop;
  std::array<int,3> tri; // indices to loop
};

class pointlist
{
private:
  std::vector<segment> break0;
public:
  ptlist points;
  revptlist revpoints;
  std::map<int,edge> edges;
  std::map<int,triangle> triangles;
  /* edges and triangles are logically arrays from 0 to size()-1, but are
   * implemented as maps, because they have pointers to each other, and points
   * point to edges, and the pointers would be messed up by moving memory
   * when a vector is resized.
   */
  std::vector<polyspiral> contours;
  std::set<point *> localPoints;
  std::set<edge *> localEdges;
  std::set<triangle *> localTriangles;
  /* localPoints, localEdges, and localTriangles are used to speed up repainting
   * when the view is of a small fraction of a huge TIN.
   */
  criteria crit;
  ContourInterval contourInterval;
  std::vector<Breakline0> type0Breaklines;
  std::vector<std::vector<xy> > type1Breaklines;
  int whichBreak0Valid;
  /* 0: means same as 3; it hasn't been set because there are no breaklines.
   * 1: type0Breaklines is the valid one (you just loaded breaklines from a file).
   * 2: edges is the valid one (you just clicked on an edge).
   * 3: both are valid (you just made a TIN, or you just saved breaklines to a file).
   */
  qindex qinx;
  std::vector<TriPolyLogEntry> triPolyLog;
  pointlist();
  void addpoint(int numb,point pnt,bool overwrite=false);
  int addtriangle(int n=1);
  void clear();
  int size();
  int lastPointNum();
  bool pointExists(int n);
  void clearmarks();
  void clearTin();
  std::map<ContourLayer,int> contourLayers();
  bool checkTinConsistency();
  bool checkFlower();
  bool shouldWrite(int n,int flags,bool contours);
  void logTriPoly(std::vector<point *> loop,int a,int b,int c);
  int1loop toInt1loop(std::vector<point *> ptrLoop);
  std::vector<point *> fromInt1loop(int1loop intLoop);
  intloop boundary();
  int readCriteria(std::string fname,Measure ms);
  void setgradient(bool flat=false);
  void findedgecriticalpts();
  void findcriticalpts();
  void addperimeter();
  void removeperimeter();
  triangle *findt(xy pnt,bool clip=false);
  bool join2break0();
  void joinBreaklines();
  void edgesToBreaklines();
  void stringToBreakline(std::string line); // for import, with hyphens
  void readBreaklines(std::string filename);
  std::string hitTestString(triangleHit hit);
  std::string hitTestPointString(xy pnt,double radius);
  void addIfIn(triangle *t,std::set<triangle *> &addenda,xy pnt,double radius);
  void setLocalSets(xy pnt,double radius);
  virtual void writeXml(std::ofstream &ofile);
  // the following methods are in tin.cpp
private:
  void dumpedges();
  void dumpnext_ps(PostScript &ps);
public:
  void dumpedges_ps(PostScript &ps,bool colorfibaster);
  void dumptriangles();
  void splitBreaklines();
  int checkBreak0(edge &e);
  bool shouldFlip(edge &e);
  bool tryStartPoint(PostScript &ps,xy &startpnt);
  int1loop convexHull();
  int flipPass(PostScript &ps,bool colorfibaster);
  void maketin(std::string filename="",bool colorfibaster=false);
  void makegrad(double corr);
  void maketriangles();
  void makeqindex();
  void updateqindex();
  void makeBareTriangles(std::vector<std::array<xyz,3> > bareTriangles);
  void triangulatePolygon(std::vector<point *> poly);
  void makeEdges();
  void deleteOrphanPoints();
  void fillInBareTin();
  double totalEdgeLength();
  double elevation(xy location);
  double dirbound(int angle);
  std::array<double,2> lohi();
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
};

#endif
