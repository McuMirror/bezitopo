/* bezier3d.cpp
 * 3d Bézier splines, used for approximations to spirals and arcs for display.
 * Of course the 3d approximation of a vertical curve is exact.
 */
#include <cmath>
#include "bezier3d.h"

bezier3d::bezier3d(xyz kra,xyz con1,xyz con2,xyz fam)
{
  controlpoints.push_back(kra);
  controlpoints.push_back(con1);
  controlpoints.push_back(con2);
  controlpoints.push_back(fam);
}

bezier3d::bezier3d()
{
  controlpoints.push_back(xyz(0,0,0));
}

xyz bezier3d::station(double along)
{
  int segment;
  double p,q;
  xyz result;
  segment=floor(along);
  p=along-segment;
  q=1-p;
  p=1-q;
  result=controlpoints[3*segment]*q*q*q+3*controlpoints[3*segment+1]*p*q*q+3*controlpoints[3*segment+2]*p*p*q+controlpoints[3*segment+3]*p*p*p;
  return result;
}
