/******************************************************/
/*                                                    */
/* manysum.cpp - add many numbers                     */
/*                                                    */
/******************************************************/
/* Copyright 2015,2016,2019,2021 Pierre Abbat.
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
#include <cfloat>
#include <cstring>
#include <iostream>
#include <cmath>
#include <vector>
#include "manysum.h"
using namespace std;

int manysum0::cnt=0;

void manysum0::clear()
{
  bucket.clear();
}

double manysum0::total()
{
  map<int,double>::iterator i;
  double t;
  for (t=0,i=bucket.begin();i!=bucket.end();i++)
    t+=i->second;
  return t;
}

void manysum0::dump()
{
  map<int,double>::iterator i;
  for (i=bucket.begin();i!=bucket.end();i++)
    cout<<i->first<<' '<<i->second<<endl;
}

void manysum0::prune()
{
  vector<int> delenda;
  int j;
  map<int,double>::iterator i;
  for (i=bucket.begin();i!=bucket.end();i++)
    if (i->second==0)
      delenda.push_back(i->first);
  for (j=0;j<delenda.size();j++)
    bucket.erase(delenda[j]);
}

manysum0& manysum0::operator+=(double x)
{
  int i=DBL_MAX_EXP+3,j=DBL_MAX_EXP+3;
  double d;
  while (x!=0)
  {
    /* frexp(NAN) on Linux sets i to 0. On DragonFly BSD,
     * it leaves i unchanged. This causes the program to hang
     * if j>i. Setting it to DBL_MAX_EXP+5 insures that NAN
     * uses a bucket separate from finite numbers.
     */
    if (std::isfinite(x))
      frexp(x,&i);
    else
      i=DBL_MAX_EXP+5;
    bucket[i]+=x;
    frexp(d=bucket[i],&j);
    if (j>i)
    {
      x=d;
      bucket[i]=0;
      if (((++cnt)&0xff)==0)
	prune();
    }
    else
      x=0;
  }
  return *this;
}

manysum0& manysum0::operator-=(double x)
{
  int i=DBL_MAX_EXP+3,j=DBL_MAX_EXP+3;
  double d;
  x=-x;
  while (x!=0)
  {
    if (std::isfinite(x))
      frexp(x,&i);
    else
      i=DBL_MAX_EXP+5;
    bucket[i]+=x;
    frexp(d=bucket[i],&j);
    if (j>i)
    {
      x=d;
      bucket[i]=0;
      if (((++cnt)&0xff)==0)
	prune();
    }
    else
      x=0;
  }
  return *this;
}

double pairwisesum(double *a,unsigned n)
{
  unsigned i,j,b;
  double sums[32],sum=0;
  for (i=0;i+7<n;i+=8)
  {
    b=i^(i+8);
    if (b==8)
      sums[3]=(((a[i]+a[i+1])+(a[i+2]+a[i+3]))+((a[i+4]+a[i+5])+(a[i+6]+a[i+7])));
    else
    {
      sums[3]+=(((a[i]+a[i+1])+(a[i+2]+a[i+3]))+((a[i+4]+a[i+5])+(a[i+6]+a[i+7])));
      for (j=4;b>>(j+1);j++)
	sums[j]+=sums[j-1];
      sums[j]=sums[j-1];
    }
  }
  for (;i<n;i++)
  {
    b=i^(i+1);
    if (b==1)
      sums[0]=a[i];
    else
    {
      sums[0]+=a[i];
      for (j=1;b>>(j+1);j++)
	sums[j]+=sums[j-1];
      sums[j]=sums[j-1];
    }
  }
  for (i=0;i<32;i++)
    if ((n>>i)&1)
      sum+=sums[i];
  return sum;
}

long double pairwisesum(long double *a,unsigned n)
{
  unsigned i,j,b;
  long double sums[32],sum=0;
  for (i=0;i+7<n;i+=8)
  {
    b=i^(i+8);
    if (b==8)
      sums[3]=(((a[i]+a[i+1])+(a[i+2]+a[i+3]))+((a[i+4]+a[i+5])+(a[i+6]+a[i+7])));
    else
    {
      sums[3]+=(((a[i]+a[i+1])+(a[i+2]+a[i+3]))+((a[i+4]+a[i+5])+(a[i+6]+a[i+7])));
      for (j=4;b>>(j+1);j++)
	sums[j]+=sums[j-1];
      sums[j]=sums[j-1];
    }
  }
  for (;i<n;i++)
  {
    b=i^(i+1);
    if (b==1)
      sums[0]=a[i];
    else
    {
      sums[0]+=a[i];
      for (j=1;b>>(j+1);j++)
	sums[j]+=sums[j-1];
      sums[j]=sums[j-1];
    }
  }
  for (i=0;i<32;i++)
    if ((n>>i)&1)
      sum+=sums[i];
  return sum;
}

double pairwisesum(vector<double> &a)
{
  if (a.size())
    return pairwisesum(&a[0],a.size());
  else
    return 0;
}

long double pairwisesum(vector<long double> &a)
{
  if (a.size())
    return pairwisesum(&a[0],a.size());
  else
    return 0;
}

manysum1::manysum1()
{
  clear();
}

void manysum1::clear()
{
  count=0;
  memset(stage0,0,sizeof(stage0));
  memset(stage1,0,sizeof(stage1));
  memset(stage2,0,sizeof(stage2));
  memset(stage3,0,sizeof(stage3));
  memset(stage4,0,sizeof(stage4));
}

double manysum1::total()
{
  return pairwisesum(stage0,8192)+pairwisesum(stage1,8192)+pairwisesum(stage2,8192)+
	 pairwisesum(stage3,8192)+pairwisesum(stage4,4096);
}

manysum1& manysum1::operator+=(double x)
{
  stage0[count&8191]=x;
  if ((count&8191)==8191)
  {
    stage1[(count>>13)&8191]=pairwisesum(stage0,8192);
    memset(stage0,0,sizeof(stage0));
  }
  if ((count&0x3ffffff)==0x3ffffff)
  {
    stage2[(count>>26)&8191]=pairwisesum(stage1,8192);
    memset(stage1,0,sizeof(stage1));
  }
  if ((count&0x7fffffffff)==0x7fffffffff)
  {
    stage3[(count>>39)&8191]=pairwisesum(stage2,8192);
    memset(stage2,0,sizeof(stage2));
  }
  if ((count&0xfffffffffffff)==0xfffffffffffff)
  {
    stage4[(count>>52)&8191]=pairwisesum(stage3,8192);
    memset(stage3,0,sizeof(stage2));
  }
  count++;
  return *this;
}

manysum1& manysum1::operator-=(double x)
{
  return operator+=(-x);
}
