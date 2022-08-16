/******************************************************/
/*                                                    */
/* boundrect.cpp - bounding rectangles                */
/*                                                    */
/******************************************************/
/* Copyright 2018,2020,2022 Pierre Abbat.
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
#include "boundrect.h"
using namespace std;

BoundRect::BoundRect()
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
  orientation=0;
}

BoundRect::BoundRect(int ori)
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
  orientation=ori;
}

void BoundRect::clear()
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
}

void BoundRect::setOrientation(int ori)
{
  orientation=ori;
}

int BoundRect::getOrientation()
{
  return orientation;
}

void BoundRect::include(xy obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj.dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
  if (0<bounds[4])
    bounds[4]=0;
  if (-0<bounds[5])
    bounds[5]=-0;
}

void BoundRect::include(xyz obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj.dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
  if (obj.elev()<bounds[4])
    bounds[4]=obj.elev();
  if (-obj.elev()<bounds[5])
    bounds[5]=-obj.elev();
}

void BoundRect::include(drawobj *obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj->dirbound(i*DEG90-orientation,bounds[i]);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  } // TODO handle elevations
}

void BoundRect::include(shared_ptr<drawobj> obj)
{
  include(obj.get());
}


#ifdef POINTLIST
void BoundRect::include(pointlist *obj)
{
  int i;
  double newbound,elev;
  for (i=0;i<4;i++)
  {
    newbound=obj->dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
  for (i=1;i<=obj->points.size();i++)
  {
    elev=obj->points[i].elev();
    if (elev<bounds[4])
      bounds[4]=elev;
    if (-elev<bounds[5])
      bounds[5]=-elev;
  }
}
#endif

