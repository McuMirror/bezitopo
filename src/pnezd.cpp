/******************************************************/
/*                                                    */
/* pnezd.cpp - file i/o in                            */
/* point-northing-easting-z-description format        */
/*                                                    */
/******************************************************/
/* Copyright 2012,2015-2020,2024 Pierre Abbat.
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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include "globals.h"
#include "pnezd.h"
#include "measure.h"
#include "pointlist.h"
#include "ldecimal.h"
#include "document.h"
#include "csv.h"
using namespace std;

/* The file produced by Total Open Station has a first line consisting of column
 * labels, which must be ignored. It is in CSV format; the quotation marks need
 * to be stripped. The file downloaded from the Nikon total station has a last line
 * consisting of ^Z; it must be ignored.
 *
 * The read routines call parseMeasurement, which can throw badNumber if a string
 * is empty or badUnits if there are garbage characters or exponents (e.g. 7776e3)
 * in a number.
 */

int readpnezd(document *doc,string fname,Measure ms,bool overwrite)
{
  ifstream infile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  infile.open(fname);
  npoints=-(!infile.is_open());
  if (infile.is_open())
  {
    do
    {
      getline(infile,line);
      while (line.length() && (line.back()=='\n' || line.back()=='\r'))
	line.pop_back();
      words=parsecsvline(line);
      if (words.size()==5)
      {
	pstr=words[0];
	nstr=words[1];
	estr=words[2];
	zstr=words[3];
	d=words[4];
	if (zstr!="z" && zstr!="Elevation")
	{
	  p=atoi(pstr.c_str());
	  n=ms.parseMeasurement(nstr,LENGTH).magnitude;
	  e=ms.parseMeasurement(estr,LENGTH).magnitude;
	  z=ms.parseMeasurement(zstr,LENGTH).magnitude;
	  doc->pl[0].addpoint(p,point(e,n,z,d),overwrite);
	  npoints++;
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
  return npoints;
}

int writepnezd(document *doc,string fname,Measure ms)
{
  ofstream outfile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  ptlist::iterator i;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  outfile.open(fname);
  npoints=-(!outfile.is_open());
  if (outfile.is_open())
  {
    for (i=doc->pl[0].points.begin();i!=doc->pl[0].points.end();i++)
    {
      p=i->first;
      n=i->second.north();
      e=i->second.east();
      z=i->second.elev();
      d=i->second.note;
      pstr=to_string(p);
      nstr=ldecimal(ms.fromCoherent(n,LENGTH),0,true);
      estr=ldecimal(ms.fromCoherent(e,LENGTH),0,true);
      zstr=ldecimal(ms.fromCoherent(z,LENGTH),0,true);
      words.clear();
      words.push_back(pstr);
      words.push_back(nstr);
      words.push_back(estr);
      words.push_back(zstr);
      words.push_back(d);
      line=makecsvline(words);
      outfile<<line<<endl;
      if (outfile.good())
	npoints++;
    }
    outfile.close();
  }
  return npoints;
}

int readpenzd(document *doc,string fname,Measure ms,bool overwrite)
{
  ifstream infile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  infile.open(fname);
  npoints=-(!infile.is_open());
  if (infile.is_open())
  {
    do
    {
      getline(infile,line);
      while (line.back()=='\n' || line.back()=='\r')
	line.pop_back();
      words=parsecsvline(line);
      if (words.size()==5)
      {
	pstr=words[0];
	estr=words[1];
	nstr=words[2];
	zstr=words[3];
	d=words[4];
	if (zstr!="z" && zstr!="Elevation")
	{
	  p=atoi(pstr.c_str());
	  n=ms.parseMeasurement(nstr,LENGTH).magnitude;
	  e=ms.parseMeasurement(estr,LENGTH).magnitude;
	  z=ms.parseMeasurement(zstr,LENGTH).magnitude;
	  doc->pl[0].addpoint(p,point(e,n,z,d),overwrite);
	  npoints++;
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
  return npoints;
}

int writepenzd(document *doc,string fname,Measure ms)
{
  ofstream outfile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  ptlist::iterator i;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  outfile.open(fname);
  npoints=-(!outfile.is_open());
  if (outfile.is_open())
  {
    for (i=doc->pl[0].points.begin();i!=doc->pl[0].points.end();i++)
    {
      p=i->first;
      n=i->second.north();
      e=i->second.east();
      z=i->second.elev();
      d=i->second.note;
      pstr=to_string(p);
      nstr=ldecimal(ms.fromCoherent(n,LENGTH));
      estr=ldecimal(ms.fromCoherent(e,LENGTH));
      zstr=ldecimal(ms.fromCoherent(z,LENGTH));
      words.clear();
      words.push_back(pstr);
      words.push_back(estr);
      words.push_back(nstr);
      words.push_back(zstr);
      words.push_back(d);
      line=makecsvline(words);
      outfile<<line<<endl;
      if (outfile.good())
	npoints++;
    }
    outfile.close();
  }
  return npoints;
}

// Zoom10 CSV format is PDENZ but with an extra empty column.

int readzoom(document *doc,string fname,Measure ms,bool overwrite)
{
  ifstream infile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  infile.open(fname);
  npoints=-(!infile.is_open());
  if (infile.is_open())
  {
    do
    {
      getline(infile,line);
      while (line.back()=='\n' || line.back()=='\r')
	line.pop_back();
      words=parsecsvline(line);
      if (words.size()>=5)
      {
	pstr=words[0];
	d=words[1];
	estr=words[2];
	nstr=words[3];
	zstr=words[4];
	if (zstr!="z" && zstr!="Elevation")
	{
	  p=atoi(pstr.c_str());
	  n=ms.parseMeasurement(nstr,LENGTH).magnitude;
	  e=ms.parseMeasurement(estr,LENGTH).magnitude;
	  z=ms.parseMeasurement(zstr,LENGTH).magnitude;
	  doc->pl[0].addpoint(p,point(e,n,z,d),overwrite);
	  npoints++;
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
  return npoints;
}

int writezoom(document *doc,string fname,Measure ms)
{
  ofstream outfile;
  size_t size=0,pos1,pos2;
  ssize_t len;
  int p,npoints;
  double n,e,z;
  ptlist::iterator i;
  vector<string> words;
  string line,pstr,nstr,estr,zstr,d;
  outfile.open(fname);
  npoints=-(!outfile.is_open());
  if (outfile.is_open())
  {
    for (i=doc->pl[0].points.begin();i!=doc->pl[0].points.end();i++)
    {
      p=i->first;
      n=i->second.north();
      e=i->second.east();
      z=i->second.elev();
      d=i->second.note;
      pstr=to_string(p);
      nstr=ldecimal(ms.fromCoherent(n,LENGTH));
      estr=ldecimal(ms.fromCoherent(e,LENGTH));
      zstr=ldecimal(ms.fromCoherent(z,LENGTH));
      words.clear();
      words.push_back(pstr);
      words.push_back(d);
      words.push_back(estr);
      words.push_back(nstr);
      words.push_back(zstr);
      line=makecsvline(words);
      outfile<<line<<endl;
      if (outfile.good())
	npoints++;
    }
    outfile.close();
  }
  return npoints;
}
