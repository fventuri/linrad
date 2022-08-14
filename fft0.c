// Copyright (c) <2012> <Leif Asbrink>
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.


#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"

#define SQRTHALF 0.707106781186547524400844362104F
#define NATLOG 2.7182818285

void fft_real_to_hermitian(float *z, int size, int n, COSIN_TABLE *tab)
{
// decimation-in-time, split-radix 
int i, j;
int i1, i2, i3, i4, itab, itab3, inc, inc3;
float cc1, ss1, cc3, ss3;
register float t1, t2;
float t3, t4, t5, t6;
int mask, is, id,ik;
int n2, n4, n8, n5, n6;
int iz;
mask = size-1;
is = 0;
id = 4;
ik=8;
do
  {
  for (i=is; i<size; i+=ik)
   {
   t1 = z[i];
   z[i  ] += z[i+1];
   z[i+1] = t1 - z[i+1];
   t1 = z[i+id];
   z[i+id  ] += z[i+id+1];
   z[i+id+1] = t1 - z[i+id+1];
   }
 is = (id<<1)-2;
 id <<= 2;
 ik<<=2;
 } while (is<size/2);
t1=z[is];
z[is  ]+=z[is+1];
z[is+1]=t1-z[is+1];
is=0;
id=8;
do
  {
  for (i=is; i<size; i+=id)
    {
    t1=z[i+3]+z[i+2];
    z[i+3]-=z[i+2];
    z[i+2]=z[i]-t1;
    z[i]+=t1;
    }
  is=(id<<1)-4;
  id<<=2;
  } while (is<size);
n2=4;
for(iz=1; iz<n; iz++)
  {
  n5=n2;
  n4=n2>>1;
  n8=n2>>2;
  n2<<=1;
  n6=n4+n5;
  is=0;
  id=n2<<1;
  do
    {
    for (i=is; i<size; i+=id)
      {
      i2=i+n4;      
      i3=i2+n4;
      i4=i3+n4;
      i1=i+n8;
      t1=z[i4]+z[i3];
      z[i4]-=z[i3];
      z[i3]=z[i]-t1;
      z[i]+=t1;
      i3+=n8;
      i4+=n8;
      i2+=n8;
      t1=(z[i3]+z[i4])*SQRTHALF;
      t2=(z[i3]-z[i4])*SQRTHALF;
      z[i4]=z[i2]-t1;
      z[i3]=-z[i2]-t1;
      z[i2]=z[i1]-t2;
      z[i1]+=t2;
      }
    is=(id<<1)-n2;
    id<<=2;
    } while(is<size);
  inc = size/n2;
  itab = inc;
  inc3=inc+(inc<<1);
  itab3=inc3;
  for (j=1; j<n8; j++)
    {
    cc1=tab[itab].cos;
    ss1=tab[itab].sin;
    cc3=tab[itab3].cos;
    ss3=tab[itab3].sin;
    is = 0;
    id = n2<<1;
    itab = (itab+inc)&(mask);
    itab3 = (itab3+inc3)&(mask);
    do
      {
      for (i=is; i<size; i+=id)
        {
        t1=z[i+j+n5]*cc1+z[i-j+n6]*ss1;
        t3=z[i+j+n6]*cc3+z[i-j+n2]*ss3;
        t2=z[i-j+n6]*cc1-z[i+j+n5]*ss1;
        t4=z[i-j+n2]*cc3-z[i+j+n6]*ss3;
        t5=t1+t3;
        t3=t1-t3;
        t6=t2+t4;
        t4=t2-t4;
        t2=z[i-j+n5]+t6;
        z[i+j+n5]=t6-z[i-j+n5];
        z[i-j+n2]=t2;
        t2=z[i+j+n4]-t3;
        z[i-j+n6]=-z[i+j+n4]-t3;
        z[i+j+n6]=t2;
        t1=z[i+j]+t5;
        z[i-j+n5]=z[i+j]-t5;
        z[i+j]=t1;
        t1=z[i-j+n4]+t4;
        z[i-j+n4]-=t4;
        z[i+j+n4]=t1;
        }
      is = (id<<1) - n2;
      id <<= 2;
      } while (is<size);
    }
  }
}

void bulk_of_dif(int size, int n, float *x, COSIN_TABLE *SinCos, int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=2;
j2=size/2;
m2=size;
for(m1=1; m1<n-1; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[ib+1]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
}

void d_bulk_of_dif(int size, int n, double *x, D_COSIN_TABLE *SinCos, int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
double x1,x2,t1,t2,t3,t4;
inc=2;
j2=size/2;
m2=size;
for(m1=1; m1<n-1; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[ib+1]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
}

void bulk_of_dual_dif(int size, int n, float *x, COSIN_TABLE *SinCos, int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=2;
j2=size/2;
m2=size;
for(m1=1; m1<n-1; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[2*ia];
      t2=x[2*ib];
      x[2*ia  ]=t1+t2;
      t3=x[2*ia+1];
      t4=x[2*ib+1];
      x1=t1-t2;
      x[2*ia+1]=t3+t4;
      x2=t3-t4;
      x[2*ib  ]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[2*ib+1]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      t1=x[2*ia+2];
      t2=x[2*ib+2];
      x[2*ia+2]=t1+t2;
      t3=x[2*ia+3];
      t4=x[2*ib+3];
      x1=t1-t2;
      x[2*ia+3]=t3+t4;
      x2=t3-t4;
      x[2*ib+2]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[2*ib+3]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
}

void d_bulk_of_dual_dif(int size, int n, double *x, D_COSIN_TABLE *SinCos, int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
double x1,x2,t1,t2,t3,t4;
inc=2;
j2=size/2;
m2=size;
for(m1=1; m1<n-1; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[2*ia];
      t2=x[2*ib];
      x[2*ia  ]=t1+t2;
      t3=x[2*ia+1];
      t4=x[2*ib+1];
      x1=t1-t2;
      x[2*ia+1]=t3+t4;
      x2=t3-t4;
      x[2*ib  ]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[2*ib+1]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      t1=x[2*ia+2];
      t2=x[2*ib+2];
      x[2*ia+2]=t1+t2;
      t3=x[2*ia+3];
      t4=x[2*ib+3];
      x1=t1-t2;
      x[2*ia+3]=t3+t4;
      x2=t3-t4;
      x[2*ib+2]=SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[2*ib+3]=SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
}

void fft_iqshift(int size, float *x)
{
int i,k;
float t1;
for(i=0; i<size/2; i++) 
  {
  k=i+size/2;
  t1=x[2*i];
  x[2*i]=x[2*k];
  x[2*k]=t1;
  t1=x[2*i+1];
  x[2*i+1]=x[2*k+1];
  x[2*k+1]=t1;
  }
}

void dual_fftback(int size, 
                  int n, float *x, 
                  COSIN_TABLE *SinCos, 
                  unsigned short int *permute,
                  int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[2*ia];
      t2=x[2*ib];
      x[2*ia  ]=t1+t2;
      t3=x[2*ia+1];
      t4=x[2*ib+1];
      x1=t1-t2;
      x[2*ia+1]=t3+t4;
      x2=t3-t4;
      x[2*ib  ]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[2*ib+1]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      t1=x[2*ia+2];
      t2=x[2*ib+2];
      x[2*ia+2]=t1+t2;
      t3=x[2*ia+3];
      t4=x[2*ib+3];
      x1=t1-t2;
      x[2*ia+3]=t3+t4;
      x2=t3-t4;
      x[2*ib+2]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[2*ib+3]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {                               
    t1=x[4*ia];
    x[4*ia]=x[4*ib];
    x[4*ib]=t1;
    t1=x[4*ia+1];
    x[4*ia+1]=x[4*ib+1];
    x[4*ib+1]=t1;
    t1=x[4*ia+2];
    x[4*ia+2]=x[4*ib+2];
    x[4*ib+2]=t1;
    t1=x[4*ia+3];
    x[4*ia+3]=x[4*ib+3];
    x[4*ib+3]=t1;
    }
  }
}

void d_dual_fftback(int size, 
                  int n, double *x, 
                  D_COSIN_TABLE *SinCos, 
                  unsigned short int *permute,
                  int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
double x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[2*ia];
      t2=x[2*ib];
      x[2*ia  ]=t1+t2;
      t3=x[2*ia+1];
      t4=x[2*ib+1];
      x1=t1-t2;
      x[2*ia+1]=t3+t4;
      x2=t3-t4;
      x[2*ib  ]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[2*ib+1]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      t1=x[2*ia+2];
      t2=x[2*ib+2];
      x[2*ia+2]=t1+t2;
      t3=x[2*ia+3];
      t4=x[2*ib+3];
      x1=t1-t2;
      x[2*ia+3]=t3+t4;
      x2=t3-t4;
      x[2*ib+2]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[2*ib+3]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {                               
    t1=x[4*ia];
    x[4*ia]=x[4*ib];
    x[4*ib]=t1;
    t1=x[4*ia+1];
    x[4*ia+1]=x[4*ib+1];
    x[4*ib+1]=t1;
    t1=x[4*ia+2];
    x[4*ia+2]=x[4*ib+2];
    x[4*ib+2]=t1;
    t1=x[4*ia+3];
    x[4*ia+3]=x[4*ib+3];
    x[4*ib+3]=t1;
    }
  }
}

void fftback(int size, 
             int n, float *x, 
             COSIN_TABLE *SinCos, 
             unsigned short int *permute,
             int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[ib+1]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {                               
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
}

void big_fftback(int size, 
             int n, float *x, 
             COSIN_TABLE *SinCos, 
             unsigned int *permute,
             int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[ib+1]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {                               
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
}

void d_fftback(int size, 
             int n, double *x, 
             D_COSIN_TABLE *SinCos, 
             unsigned int *permute)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
double x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1+SinCos[it].sin*x2;
      x[ib+1]=-SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {                               
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
}


void fftforward(int size, 
             int n, float *x, 
             COSIN_TABLE *SinCos, 
             unsigned short int *permute,
             int yieldflag)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[ib+1]= SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
}

void d_fftforward(int size, 
             int n, double *x, 
             D_COSIN_TABLE *SinCos, 
             unsigned int *permute)
{
int inc, j2, m2, m1, ja;
int ia, ib,it;
double x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[ib+1]= SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
for(ia=0; ia < size; ia++)
  {
  ib=permute[ia  ];
  if(ib<ia)
    {
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
for(ia=0; ia < 2*size; ia++)
  {
  x[ia]/=size;
  }
}



void big_fftforward(int size, 
             int n, float *x, 
             COSIN_TABLE *SinCos, 
             unsigned int *permute,
             int yieldflag)
{
// This routine is only called from THREAD_FFT2 and that is the thread
// that we should give priority in a dual core machine.
// Allow it to run 100 % of the time if there is more than one CPU.
int inc, j2, m2, m1, ja;
int ia, ib,it;
float x1,x2,t1,t2,t3,t4;
inc=1;
j2=size;
m2=2*size;
for(m1=0; m1<n; m1++)
  {
  if(yieldflag)lir_sched_yield();
  for(ja=0; ja<=2*size-m2; ja+=m2)
    {
    it=0;
    for(ia=ja; ia<j2+ja; ia+=2)
      {
      ib=ia+j2;
      t1=x[ia];
      t2=x[ib];
      x[ia  ]=t1+t2;
      t3=x[ia+1];
      t4=x[ib+1];
      x1=t1-t2;
      x[ia+1]=t3+t4;
      x2=t3-t4;
      x[ib  ]= SinCos[it].cos*x1-SinCos[it].sin*x2;
      x[ib+1]= SinCos[it].sin*x1+SinCos[it].cos*x2;
      it+=inc;
      }
    }
  inc*=2;
  m2=m2/2;
  j2=j2/2;
  }
if(yieldflag)lir_sched_yield();
for(ia=0; ia < size; ia++)
  {
  ib=(int)permute[ia  ];
  if(ib<ia)
    {
    t1=x[2*ia];
    x[2*ia]=x[2*ib];
    x[2*ib]=t1;
    t1=x[2*ia+1];
    x[2*ia+1]=x[2*ib+1];
    x[2*ib+1]=t1;
    }
  }
}

extern int *baseband_handle;

void make_window(int mo, int sz, int n, float *win)
{
double x,z,sumsq;
double e1, e2;
float t1;
int i, size;
if(mo == 5 || mo == 6)
  {
  e1=3.2;
  e2=13.0/sz;
  for (i=0; i <= sz/2; i++)
    {
    win[i]=0.5F*(float)erfc(e1);
    e1-=e2;
    }
  if(mo == 5)return;
  for (i=0; i < sz/2; i++)
    {
    win[i]=(float)pow(win[i]+.0005,-2.0);
    win[sz-i-1]=win[i];
    }
  return;
  }
if(n == 0)return;
size=sz;
z=n;
if(mo == 2)size=2*sz;
sumsq=0;
// Produce a sin power n function over size/2 points for n=1 to 7.
// In case n equals 9, use the error function erfc instead, start at -192 dB.
if(n==9)
  {
  e1=4.4;
  e2=40.0/size;
  if(size < 128)e2/=1.5;
  if(size < 64)e2/=1.7;
  for (i=0; i <= size/2; i++)
    {
    win[i]=0.5F*(float)erfc(e1);
    sumsq+=win[i]*win[i];
    e1-=e2;
    }
  }
else
  {    
// In case n equals 8, use a Gaussian.
// Start at -208 dB.
  if(n==8)
    {
    e1=0;
    e2=9.8/size;
    for (i=size/2; i >= 0; i--)
      {
      win[i]=(float)pow(NATLOG,-e1*e1);
      sumsq+=win[i]*win[i];
      e1+=e2;
      }
    }
  else
    {
    x=0;
    for (i=0; i <= size/2; i++)
      {
      win[i]=(float)pow(sin(x),z);
      sumsq+=win[i]*win[i];
      x+=PI_L/size;
      }
    }  
  }
if(mo == 5)return;  
// Produce an inverted window over size/2 points
if(mo == 3)
  {
  win[0]=1;
  for (i=1; i <= size/2; i++)
    {
    win[i]=1/win[i];
    }
  return;
  }
z=1/sqrt(2*sumsq/size);
for (i=0; i <= size/2; i++)
  {
  win[i]*=(float)z;
  }
if(mo == 4)
  {
  for(i=size/2+1; i<size; i++)
    {
    win[i]=win[size-i];
    }
  return;
  }    
// Store the data in the order we want to use it
// This will save time by better cash usage for large transforms.
if(mo == 1)
  {
  t1=win[1];
  win[1]=win[size/2];  
  for (i=size/2-1; i >1; i--)
    {
    win[2*i]=win[i];
    }
  win[2]=t1;  
  for (i=0; i < size-3; i+=2)
    {
    win[i+3]=win[size-i-2];
    }
  }
}

void make_d_window(int mo, int sz, int n, double *win)
{
double x,z,sumsq;
double e1, e2;
double t1;
int i, size;
if(mo == 5 || mo == 6)
  {
  e1=3.2;
  e2=13.0/sz;
  for (i=0; i <= sz/2; i++)
    {
    win[i]=0.5*erfc(e1);
    e1-=e2;
    }
  if(mo == 5)return;
  for (i=0; i < sz/2; i++)
    {
    win[i]=pow(win[i]+.0005,-2.0);
    win[sz-i-1]=win[i];
    }
  return;
  }
if(n == 0)return;
size=sz;
z=n;
if(mo == 2)size=2*sz;
sumsq=0;
// Produce a sin power n function over size/2 points for n=1 to 7.
// In case n equals 9, use the error function erfc instead, start at -192 dB.
if(n==9)
  {
  e1=4.4;
  e2=40.0/size;
  if(size < 128)e2/=1.5;
  if(size < 64)e2/=1.7;
  for (i=0; i <= size/2; i++)
    {
    win[i]=0.5*erfc(e1);
    sumsq+=win[i]*win[i];
    e1-=e2;
    }
  }
else
  {    
// In case n equals 8, use a Gaussian.
// Start at -208 dB.
  if(n==8)
    {
    e1=0;
    e2=9.8/size;
    for (i=size/2; i >= 0; i--)
      {
      win[i]=pow(NATLOG,-e1*e1);
      sumsq+=win[i]*win[i];
      e1+=e2;
      }
    }
  else
    {
    x=0;
    for (i=0; i <= size/2; i++)
      {
      win[i]=pow(sin(x),z);
      sumsq+=win[i]*win[i];
      x+=PI_L/size;
      }
    }  
  }
if(mo == 5)return;  
// Produce an inverted window over size/2 points
if(mo == 3)
  {
  win[0]=1;
  for (i=1; i <= size/2; i++)
    {
    win[i]=1/win[i];
    }
  return;
  }
z=1/sqrt(2*sumsq/size);
for (i=0; i <= size/2; i++)
  {
  win[i]*=z;
  }
if(mo == 4)
  {
  for(i=size/2+1; i<size; i++)
    {
    win[i]=win[size-i];
    }
  return;
  }    
// Store the data in the order we want to use it
// This will save time by better cash usage for large transforms.
if(mo == 1)
  {
  t1=win[1];
  win[1]=win[size/2];  
  for (i=size/2-1; i >1; i--)
    {
    win[2*i]=win[i];
    }
  win[2]=t1;  
  for (i=0; i < size-3; i+=2)
    {
    win[i+3]=win[size-i-2];
    }
  }
}

void make_mmxwindow(int mo, int size, int n, short int *win)
{
double x,z,t1;
int i, k, iw;
double *tmpwin;
double e1, e2;
if(n <= 0)return;
// We only use MMX when the second fft is enabled.
tmpwin=(double*)timf2_shi;
// Produce a sin power n function over size points for n=1 to 7.
// In case n equals 9, use the error function erfc instead, start at -150 dB.
if(n==9)
  {
  e1=3.6;
  e2=80.0/size;
  if(size < 128)e2/=1.5;
  if(size < 64)e2/=1.7;
  for (i=0; i <= size/2; i++)
    {
    tmpwin[i]=0.5*erfc(e1);
    e1-=e2;
    }
  }
else
  {    
// In case n equals 8, use a Gaussian.
  if(n==8)
    {
    e1=0;
    e2=9.0/size;
    for (i=size/2; i >= 0; i--)
      {
      tmpwin[i]=pow(NATLOG,-e1*e1);
      e1+=e2;
      }
    }
  else
    {
    x=0;
    z=n;
    tmpwin[0]=0;
    for (i=1; i <= size/2; i++)
      {
      tmpwin[i]=pow(sin(x),z);
      x+=PI_L/size;
      }
    }  
  }
for(i=size-1; i>size/2; i--)
  {
  tmpwin[i]=tmpwin[size-i];
  }
// ************************************************
//            mo  ==  0
// Produce a sin power n function over size points
// Store each value once.
x=0;
z=n;
if(mo == 0)
  {
  for (i=0; i < size; i++)
    {
    t1=tmpwin[i];
    iw=(int)((0.5+0x8000)*t1);
    if(iw >= 0x8000)iw=0x7fff;
    win[i]=(short int)iw;
    x+=PI_L/size;
    }
  return;  
  }
// Produce a sin power n function over size/2 points
// or the inverted function over size/2+1 points
// Store the result 4 times for dual or quad transforms
k=size/2;
if(mo==3)k++;
for (i=0; i < k; i++)
  {
  t1=tmpwin[i];
  x+=PI_L/size;
  if(mo == 3)t1=0.5/t1;
  t1=floor(0x7f00*t1);
  iw=(int)t1;
  if(iw>0x7ffe)iw=0x7ffe;
  win[4*i  ]=(short int)iw;
  win[4*i+1]=(short int)iw;
  win[4*i+2]=(short int)iw;
  win[4*i+3]=(short int)iw;
  }
}


void init_mmxfft(  int size, MMX_COSIN_TABLE *tab)
{
double x;
int i,is,ic;
x=0;
for (i=0; i<size/2; i++)
  {
  is=(int)(32767.5*sin(x));
  ic=(int)(32767.5*cos(x));
  if(is>0x7fff)is=0x7fff;
  if(ic>0x7fff)ic=0x7fff;
  if(is<-0x7fff)is=-0x7fff;
  if(ic<-0x7fff)ic=-0x7fff;
  
  x+=(double)(PI_L/(size/2));
  tab[i].c1p=(short int)ic;
  tab[i].s2p=(short int)is;
  tab[i].c3p=(short int)-is;
  tab[i].s4m=(short int)ic;
  }
}

void make_permute(int mo, int nz, int sz, unsigned short int *perm)
{
unsigned int i, i2, j, k, l, m;
unsigned short int *tmp;
unsigned int n, size;
if(mo == 2)
  {
  n=(unsigned int)(nz+1);
  size=(unsigned int)(sz*2);
  }
else
  {
  n=(unsigned int)nz;
  size=(unsigned int)sz;
  }
if(!fft1_use_gpu)
  {
  if(size > 65536)
    {
    lirerr(937);
    return;
    }  
  }
for(i=0; i<size; i++)perm[i]=0;
tmp=malloc(size*sizeof(short int));
if(tmp == NULL)
  {
  lirerr(1032);
  return;
  }
for(i=0; i<size; i++)tmp[i]=(short unsigned int)i;
for(i=1; i<size; i++)
  {
  m=i;
  j=0;
  for(k=0; k<n; k++)
    {
    j=2*j;
    l=m/2;
    if(2*l != m)j=j+1;
    m=l;
    }
  if(j > i)
    {
    i2=tmp[i];
    tmp[i]=tmp[j];
    tmp[j]=(short unsigned int)i2;
    }
  }
if(mo == 1)
  {  
  for(i=0; i<size/2; i++) 
    {
    k=tmp[i];
    tmp[i]=tmp[i+size/2];
    tmp[i+size/2]=(short unsigned int)k;
    }
  }
for(i=0; i<size; i++)perm[tmp[i]]=(short unsigned int)i;
free(tmp);
}

void make_bigpermute(int mo, int nz, int sz, unsigned int *perm)
{
unsigned int i, i2, j, k, l, m;
unsigned int *tmp;
unsigned int n, size;
if(mo == 2)
  {
  n=(unsigned int)nz+1;
  size=(unsigned int)sz*2;
  }
else
  {
  n=(unsigned int)nz;
  size=(unsigned int)sz;
  }
for(i=0; i<size; i++)perm[i]=0;
tmp=malloc(size*sizeof(int));
if(tmp == NULL)
  {
  lirerr(1032);
  return;
  }
for(i=0; i<size; i++)tmp[i]=i;
for(i=0; i<size; i++)
  {
  m=i;
  j=0;
  for(k=0; k<n; k++)
    {
    j=2*j;
    l=m/2;
    if(2*l != m)j=j+1;
    m=l;
    }
  if(j > i)
    {
    i2=tmp[i];
    tmp[i]=tmp[j];
    tmp[j]=i2;
    }
  }
if(mo == 1)
  {  
  for(i=0; i<size/2; i++) 
    {
    k=tmp[i];
    tmp[i]=tmp[i+size/2];
    tmp[i+size/2]=k;
    }
  }
for(i=0; i<size; i++)perm[tmp[i]]=i;
free(tmp);
}

void make_sincos(int mo, int sz, COSIN_TABLE *tab) 
{
int i;
double x,step;
int size;
if(mo == 2)
  {
  size=sz*2;
  }
else
  {
  size=sz;
  }  
x=0;
step=(double)(PI_L/(size/2));
for (i=0; i<size/2; i++)
  {
  tab[i].sin=(float)sin(x);
  tab[i].cos=(float)cos(x);
  x+=step;
  }
}

void make_d_sincos(int mo, int sz, D_COSIN_TABLE *tab) 
{
int i;
double x,step;
int size;
if(mo == 2)
  {
  size=sz*2;
  }
else
  {
  size=sz;
  }  
x=0;
step=(double)(PI_L/(size/2));
for (i=0; i<size/2; i++)
  {
  tab[i].sin=sin(x);
  tab[i].cos=cos(x);
  x+=step;
  }
}

void init_fft(int mo, int nz, int sz, COSIN_TABLE *tab, 
                                               unsigned short int *perm)
{
make_permute(mo, nz, sz, perm);
make_sincos(mo, sz, tab); 
}

void init_d_fft(int mo, int nz, int sz, D_COSIN_TABLE *tab, 
                                               unsigned int *perm)
{
make_bigpermute(mo, nz, sz, perm);
make_d_sincos(mo, sz, tab); 
}


void init_big_fft(int mo, int nz, int sz, COSIN_TABLE *tab, 
                                               unsigned int *perm)
{
make_bigpermute(mo, nz, sz, perm);
make_sincos(mo, sz, tab); 
}

void bulk_of_dual_dit(int size, int n, 
                                   float *x, COSIN_TABLE *tab, int yieldflag)
{
int ia, ib, ic, id, itab;
int inc,j,k,m,nn;
int m1, m2;
float t1,t2,t3,t4,t5,t6,t7,t8;
float r1,r2,r3,r4,r5,r6,r7,r8;
float x0,x1,x2,x3,x4,x5,x6,x7;
float ya,yb,y2,y3,y4,y5,y6,y7;
float u0,u1,u2,u3,u4,u5,u6,u7;
float w0,w1,w2,w3,w4,w5,w6,w7;
float s1,s2,c1,c2;
nn=size;
m1=4;
m2=16;
inc=size/16;
m=n-2;
while(m>2)
  {
  if(yieldflag)lir_sched_yield();
  m-=2;
  for(j=0; j<nn; j+=m2)
    {
    itab=0;
    k=j;
lp1:;    
    ia=4*k;
    s1= tab[2*itab].sin; 
    c1= tab[2*itab].cos;
    s2= tab[itab].sin; 
    c2= tab[itab].cos;

    x0=x[ia  ];
    ya=x[ia+1];
    u0=x[ia+2];
    w0=x[ia+3];
    ia+=m2;

    x1=x[ia  ];
    yb=x[ia+1];
    u1=x[ia+2];
    w1=x[ia+3];
    ia+=m2;

    t1=c1*x1+s1*yb;      
    t2=c1*yb-s1*x1;      
    r1=c1*u1+s1*w1;      
    r2=c1*w1-s1*u1;      

    x4=x0+t1;
    y4=ya+t2;
    u4=u0+r1;
    w4=w0+r2;

    x6=x0-t1;
    y6=ya-t2;
    u6=u0-r1;
    w6=w0-r2;
 
    x2=x[ia  ];
    y2=x[ia+1];
    u2=x[ia+2];
    w2=x[ia+3];
    ia+=m2;

    x3=x[ia  ];
    y3=x[ia+1];
    u3=x[ia+2];
    w3=x[ia+3];
 
    t3=c1*x3+s1*y3;      
    t4=c1*y3-s1*x3;      
    r3=c1*u3+s1*w3;      
    r4=c1*w3-s1*u3;      

    x5=x2+t3;
    y5=y2+t4;
    u5=u2+r3;
    w5=w2+r4;
 
    x7=x2-t3;
    y7=y2-t4;
    u7=u2-r3;
    w7=w2-r4;

    t5=c2*x5+s2*y5;
    t6=c2*y5-s2*x5;
    r5=c2*u5+s2*w5;
    r6=c2*w5-s2*u5;

    ia=k;
    ib=ia+m1;
    ic=ia+2*m1;
    id=ia+3*m1;

    x[4*ia  ]=x4+t5;
    x[4*ia+1]=y4+t6;
    x[4*ia+2]=u4+r5;
    x[4*ia+3]=w4+r6;

    x[4*ic  ]=x4-t5;
    x[4*ic+1]=y4-t6;
    x[4*ic+2]=u4-r5;
    x[4*ic+3]=w4-r6;

    t8=c2*x7+s2*y7;
    t7=c2*y7-s2*x7;
    r8=c2*u7+s2*w7;
    r7=c2*w7-s2*u7;

    x[4*ib  ]=x6+t7;
    x[4*ib+1]=y6-t8;
    x[4*ib+2]=u6+r7;
    x[4*ib+3]=w6-r8;

    x[4*id  ]=x6-t7;
    x[4*id+1]=y6+t8;
    x[4*id+2]=u6-r7;
    x[4*id+3]=w6+r8;

    itab+=inc;
    k++; 
    if(itab<size/4)goto lp1;
    }
  inc/=4;
  m1*=4;
  m2*=4;
  }
if(yieldflag)lir_sched_yield();
if( (n&1) ==1 )
  {
  ib=nn/2;
  m=nn/2;
  for(ia=0; ia<m; ia++)
    {
    s1= tab[ia].sin; 
    c1= tab[ia].cos;
    x0=x[4*ia  ];
    ya=x[4*ia+1];
    u0=x[4*ia+2];
    w0=x[4*ia+3];
    x1=x[4*ib  ];
    yb=x[4*ib+1];
    u1=x[4*ib+2];
    w1=x[4*ib+3];
    t1=c1*x1+s1*yb;
    t2=c1*yb-s1*x1;
    r1=c1*u1+s1*w1;
    r2=c1*w1-s1*u1;
    x[4*ia  ]=x0-t1;
    x[4*ia+1]=-(ya-t2);
    x[4*ia+2]=u0-r1;
    x[4*ia+3]=-(w0-r2);
    x[4*ib  ]=x0+t1;
    x[4*ib+1]=-(ya+t2);
    x[4*ib+2]=u0+r1;
    x[4*ib+3]=-(w0+r2);
    ib++;
    }
  }
else
  {
  itab=0;
lp2:;
  ia=4*itab;
  ib=ia+nn;
  ic=ia+2*nn;
  id=ia+3*nn;

  s1= tab[2*itab].sin; 
  c1= tab[2*itab].cos;
  s2= tab[itab].sin; 
  c2= tab[itab].cos;

  x0=x[ia  ];
  ya=x[ia+1];
  u0=x[ia+2];
  w0=x[ia+3];

  x1=x[ib  ];
  yb=x[ib+1];
  u1=x[ib+2];
  w1=x[ib+3];

  t1=c1*x1+s1*yb;      
  t2=c1*yb-s1*x1;      
  r1=c1*u1+s1*w1;      
  r2=c1*w1-s1*u1;      

  x4=x0+t1;
  y4=ya+t2;
  u4=u0+r1;
  w4=w0+r2;

  x6=x0-t1;
  y6=ya-t2;
  u6=u0-r1;
  w6=w0-r2;
 
  x2=x[ic  ];
  y2=x[ic+1];
  u2=x[ic+2];
  w2=x[ic+3];

  x3=x[id  ];
  y3=x[id+1];
  u3=x[id+2];
  w3=x[id+3];
 
  t3=c1*x3+s1*y3;      
  t4=c1*y3-s1*x3;      
  r3=c1*u3+s1*w3;      
  r4=c1*w3-s1*u3;      

  x5=x2+t3;
  y5=y2+t4;
  u5=u2+r3;
  w5=w2+r4;
 
  x7=x2-t3;
  y7=y2-t4;
  u7=u2-r3;
  w7=w2-r4;

  t5=c2*x5+s2*y5;
  t6=c2*y5-s2*x5;
  r5=c2*u5+s2*w5;
  r6=c2*w5-s2*u5;

  x[ic  ]=x4+t5;
  x[ic+1]=-(y4+t6);
  x[ic+2]=u4+r5;
  x[ic+3]=-(w4+r6);

  x[ia  ]=x4-t5;
  x[ia+1]=-(y4-t6);
  x[ia+2]=u4-r5;
  x[ia+3]=-(w4-r6);

  t8=c2*x7+s2*y7;
  t7=c2*y7-s2*x7;
  r8=c2*u7+s2*w7;
  r7=c2*w7-s2*u7;

  x[id  ]=x6+t7;
  x[id+1]=-(y6-t8);
  x[id+2]=u6+r7;
  x[id+3]=-(w6-r8);

  x[ib  ]=x6-t7;
  x[ib+1]=-(y6+t8);
  x[ib+2]=u6-r7;
  x[ib+3]=-(w6+r8);
  itab++;
  if(itab<size/4)goto lp2;
  }
}

void bulk_of_dit(int size, int n, float *x, COSIN_TABLE *tab, int yieldflag)
{
int ia, ib, ic, id, itab;
int inc,j,k,m,nn;
int m1, m2;
float t1,t2,t3,t4,t5,t6,t7,t8;
float x0,x1,x2,x3,x4,x5,x6,x7;
float ya,yb,y2,y3,y4,y5,y6,y7;
float s1,s2,c1,c2;
nn=size;
m1=4;
m2=16;
inc=size/16;
m=n-2;
while(m>2)
  {
  if(yieldflag)lir_sched_yield();
  m-=2;
  for(j=0; j<nn; j+=m2)
    {
    itab=0;
    k=j;
lp1:;    
    ia=2*k;
    s1= tab[2*itab].sin; 
    c1= tab[2*itab].cos;
    s2= tab[itab].sin; 
    c2= tab[itab].cos;

    x0=x[ia  ];
    ya=x[ia+1];
    ia+=m2/2;

    x1=x[ia  ];
    yb=x[ia+1];
    ia+=m2/2;

    t1=c1*x1+s1*yb;      
    t2=c1*yb-s1*x1;      

    x4=x0+t1;
    y4=ya+t2;

    x6=x0-t1;
    y6=ya-t2;
 
    x2=x[ia  ];
    y2=x[ia+1];
    ia+=m2/2;

    x3=x[ia  ];
    y3=x[ia+1];
 
    t3=c1*x3+s1*y3;      
    t4=c1*y3-s1*x3;      

    x5=x2+t3;
    y5=y2+t4;
 
    x7=x2-t3;
    y7=y2-t4;

    t5=c2*x5+s2*y5;
    t6=c2*y5-s2*x5;

    ia=k;
    ib=ia+m1;
    ic=ia+2*m1;
    id=ia+3*m1;

    x[2*ia  ]=x4+t5;
    x[2*ia+1]=y4+t6;

    x[2*ic  ]=x4-t5;
    x[2*ic+1]=y4-t6;

    t8=c2*x7+s2*y7;
    t7=c2*y7-s2*x7;

    x[2*ib  ]=x6+t7;
    x[2*ib+1]=y6-t8;

    x[2*id  ]=x6-t7;
    x[2*id+1]=y6+t8;

    itab+=inc;
    k++; 
    if(itab<size/4)goto lp1;
    }
  inc/=4;
  m1*=4;
  m2*=4;
  }
if(yieldflag)lir_sched_yield();
if( (n&1) ==1 )
  {
  ib=nn/2;
  m=nn/2;
  for(ia=0; ia<m; ia++)
    {
    s1= tab[ia].sin; 
    c1= tab[ia].cos;
    x0=x[2*ia  ];
    ya=x[2*ia+1];
    x1=x[2*ib  ];
    yb=x[2*ib+1];
    t1=c1*x1+s1*yb;
    t2=c1*yb-s1*x1;
    x[2*ia  ]=x0-t1;
    x[2*ia+1]=-(ya-t2);
    x[2*ib  ]=x0+t1;
    x[2*ib+1]=-(ya+t2);
    ib++;
    }
  }
else
  {
  itab=0;
lp2:;
  ia=2*itab;
  ib=ia+m2/2;
  ic=ia+m2;
  id=ia+3*m2/2;

  s1= tab[2*itab].sin; 
  c1= tab[2*itab].cos;
  s2= tab[itab].sin; 
  c2= tab[itab].cos;

  x0=x[ia  ];
  ya=x[ia+1];

  x1=x[ib  ];
  yb=x[ib+1];

  t1=c1*x1+s1*yb;      
  t2=c1*yb-s1*x1;      

  x4=x0+t1;
  y4=ya+t2;

  x6=x0-t1;
  y6=ya-t2;
 
  x2=x[ic  ];
  y2=x[ic+1];

  x3=x[id  ];
  y3=x[id+1];
 
  t3=c1*x3+s1*y3;      
  t4=c1*y3-s1*x3;      

  x5=x2+t3;
  y5=y2+t4;
 
  x7=x2-t3;
  y7=y2-t4;

  t5=c2*x5+s2*y5;
  t6=c2*y5-s2*x5;

  x[ic  ]=x4+t5;
  x[ic+1]=-(y4+t6);

  x[ia  ]=x4-t5;
  x[ia+1]=-(y4-t6);

  t8=c2*x7+s2*y7;
  t7=c2*y7-s2*x7;

  x[id  ]=x6+t7;
  x[id+1]=-(y6-t8);

  x[ib  ]=x6-t7;
  x[ib+1]=-(y6+t8);
  itab++;
  if(itab<size/4)goto lp2;
  }
}
