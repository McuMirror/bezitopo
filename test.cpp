/******************************************************/
/*                                                    */
/* test.cpp - test patterns and functions             */
/*                                                    */
/******************************************************/

#include <cmath>
#include "bezitopo.h"
#include "test.h"

using std::map;

double rugae(xy pnt)
{double z;
 z=sin(pnt.east())+pnt.north()/50;
 return z;
 }

double (*testsurface)(xy pnt)=rugae;

void aster(int n)
/* Fill points with asteraceous pattern. Pattern invented by H. Vogel in 1979
   and later by me, not knowing of Vogel. */
{int i;
 double angle=(sqrt(5)-1)*M_PI;
 xy pnt;
 for (i=0;i<n;i++)
     {pnt=xy(cos(angle*i)*sqrt(i+0.5),sin(angle*i)*sqrt(i+0.5));
      addpoint(i+1,point(pnt,testsurface(pnt),"test"));
      }
 }

void ring(int n)
/* Points in a circle, for most ambiguous case of the Delaunay algorithm. */
{int i;
 double angle=(sqrt(5)-1)*M_PI;
 xy pnt;
 for (i=0;i<n;i++)
     {pnt=xy(cos(angle*i)*sqrt(n+0.5),sin(angle*i)*sqrt(n+0.5));
      addpoint(i+1,point(pnt,testsurface(pnt),"test"));
      }
 }

void ellipse(int n)
/* Points in an ellipse, for worst case of the Delaunay algorithm. */
{int i;
 double angle=(sqrt(5)-1)*M_PI;
 xy pnt;
 for (i=0;i<n;i++)
     {pnt=xy(cos(angle*i)*sqrt(n+0.5)*0.99,sin(angle*i)*sqrt(n+0.5)*1.01);
      addpoint(i+1,point(pnt,testsurface(pnt),"test"));
      }
 }

void lozenge(int n)
// Add points on the short diagonal of a rhombus, then add the two other points.
{int i;
 double angle;
 xy pnt;
 for (i=0;i<n;i++)
     {angle=(2.0*i/(n-1)-1)*M_PI/6;
      pnt=xy(0,sqrt(n)*tan(angle));
      addpoint(i+1,point(pnt,testsurface(pnt),"test"));
      }
 pnt=xy(-sqrt(n),0);
 addpoint(n+1,point(pnt,testsurface(pnt),"test"));
 pnt=xy(sqrt(n),0);
 addpoint(n+2,point(pnt,testsurface(pnt),"test"));
 }

void rotate(int n)
{int i;
 double tmpx,tmpy;
 map<int,point>::iterator j;
 for (j=points.begin();j!=points.end();j++)
     for (i=0;i<n;i++)
         {tmpx=j->second.x*0.6-j->second.y*0.8;
          tmpy=j->second.y*0.6+j->second.x*0.8;
          j->second.x=tmpx;
          j->second.y=tmpy;
          }
 }
