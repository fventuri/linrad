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
#include "llsqdef.h"

void mask_tophat_filter2(float *xin,float *xout,int len, int pa,int pb,int size)
{
int i, k, ia, ib, ic;
int mask;
float t1,t2;
mask=size-1;
t1=0;
t2=0;
ic=pa;
for(i=0; i<len; i++)
  {
  t1+=xin[2*ic];
  t2+=xin[2*ic+1];
  ic=(ic+1)&mask;
  }
k=len/2;
ib=pa;
for(i=0; i<k; i++)
  {
  xout[2*ib  ]=t1/(float)len;
  xout[2*ib+1]=t2/(float)len;
  ib=(ib+1)&mask;
  }
ia=pa;  
while(ic != pb)
  {
  xout[2*ib  ]=t1/(float)len;
  xout[2*ib+1]=t2/(float)len;
  t1+=xin[2*ic  ];
  t2+=xin[2*ic+1];
  ic=(ic+1)&mask;
  t1-=xin[2*ia  ];
  t2-=xin[2*ia+1];
  ib=(ib+1)&mask;
  ia=(ia+1)&mask;
  }
while(ib != pb)
  {  
  xout[2*ib  ]=t1/(float)len;
  xout[2*ib+1]=t2/(float)len;
  ib=(ib+1)&mask;
  }
}

void mask_tophat_filter1(float *xin,float *xout,int len, int pa,int pb,int size)
{
int i, k, ia, ib, ic;
int mask;
float t1;
mask=size-1;
t1=0;
ic=pa;
for(i=0; i<len; i++)
  {
  t1+=xin[ic];
  ic=(ic+1)&mask;
  }
k=len/2;
ib=pa;
for(i=0; i<k; i++)
  {
  xout[ib  ]=t1/(float)len;
  ib=(ib+1)&mask;
  }
ia=pa;  
while(ic != pb)
  {
  xout[ib  ]=t1/(float)len;
  t1+=xin[ic  ];
  ic=(ic+1)&mask;
  t1-=xin[ia  ];
  ib=(ib+1)&mask;
  ia=(ia+1)&mask;
  }
while(ib != pb)
  {  
  xout[ib  ]=t1/(float)len;
  ib=(ib+1)&mask;
  }
}




void parabolic_fit(float *amp, float *pos, float yy1, float yy2, float yy3)
{
float t3,t4;
// ***********************************
// Fit a parabola to the max point and it's nearest neighbours.
// As input we have 3 data points y(x)
// Find parameters a,b and c to place these points on the curve:
//  y(x)=a+b*(x-c)**2=a+b*x**2-2*x*b*c+b*c**2
// The equations to solve are:
// y(-1)=a + b + 2*b*c + b*c**2
// y( 0)=a     +         b*c**2
// y( 1)=a + b - 2*b*c + b*c**2
//  b has to be negative (for a positive peak)
//  abs(c) has to be below 0.5 since y(0)is the largest value.
// t4=y(-1)-y(1)=4*b*c
// t3=2*(y(-1)+y(1)-2*y(0))=4*b
// t4/t3=c=peak offset
// ***********************************

t4=yy1-yy3;
t3=2*(yy1+yy3-2*yy2);
if(t3 < 0)
  {
  amp[0]=yy2-0.5F*t4*t4/t3;
  t4=t4/t3;
  if(fabs(t4) > 1)t4/=(float)fabs(t4);
  pos[0]=t4;
  }
else
  {
  if(yy1 > yy3)
    {
    amp[0]=yy1;
    pos[0]=-1;
    }
  else
    {
    amp[0]=yy3;
    pos[0]=1;
    }
  }
}    






int llsq1(void)
{
float aux[2*LLSQ_MAXPAR];
int ipiv[2*LLSQ_MAXPAR];
int kpiv;
int i,j,k;
float t1,sig,piv,beta;
if(llsq_npar > LLSQ_MAXPAR)
  {
  lirerr(1051);
  return -1;
  }
kpiv=0;
piv=0;
for(k=0; k<llsq_npar; k++)
  {
  ipiv[k]=k;
  t1=0;
  for(i=0; i<llsq_neq; i++)
    {
    t1+=(float)pow(llsq_derivatives[k*llsq_neq+i],2.0);
    }
  aux[k]=t1;
  if(t1 > piv)
    {
    piv=t1;
    kpiv=k;
    }
  }
if(piv == 0)return -1;
sig=(float)sqrt(piv);
for(k=0; k<llsq_npar; k++)
  {
  if(kpiv>k)
    {
    t1=aux[k];
    aux[k]=aux[kpiv];
    aux[kpiv]=t1;
    for(i=k; i<llsq_neq; i++)
      {
      t1=llsq_derivatives[k*llsq_neq+i];
      llsq_derivatives[k*llsq_neq+i]=llsq_derivatives[kpiv*llsq_neq+i];
      llsq_derivatives[kpiv*llsq_neq+i]=t1;
      }
    }
  if(k > 0)
    {
    sig=0;
    for(i=k; i<llsq_neq; i++)
      {
      sig+=llsq_derivatives[k*llsq_neq+i]*llsq_derivatives[k*llsq_neq+i];
      }
    sig=(float)sqrt(sig);
    }
  t1=llsq_derivatives[k*llsq_neq+k];
  if(t1 < 0 )sig=-sig;
  ipiv[kpiv]=ipiv[k];
  ipiv[k]=kpiv;
  beta=t1+sig;
  llsq_derivatives[k*llsq_neq+k]=beta;
  beta=1/(sig*beta);
  j=llsq_npar+k;
  aux[j]=-sig;
  if(k<llsq_npar-1)
    {
    piv=0;
    kpiv=k+1;
    for(j=k+1; j<llsq_npar; j++)
      {
      t1=0;
      for(i=k; i<llsq_neq; i++)
        {
        t1+=llsq_derivatives[k*llsq_neq+i]*llsq_derivatives[j*llsq_neq+i];
        }
      t1=beta*t1;
      for(i=k; i<llsq_neq; i++)
        {
        llsq_derivatives[j*llsq_neq+i]-=llsq_derivatives[k*llsq_neq+i]*t1;
        }
      t1=aux[j]-llsq_derivatives[j*llsq_neq+k]*llsq_derivatives[j*llsq_neq+k];
      aux[j]=t1;
      if(t1 > piv)
        {
        piv=t1;
        kpiv=j;
        }
      }
    }
  t1=0;
  for(i=k; i<llsq_neq; i++)
    {
    t1+=llsq_derivatives[k*llsq_neq+i]*llsq_errors[i];
    }
  t1*=beta;
  for(i=k; i<llsq_neq; i++)
    {
    llsq_errors[i]-=llsq_derivatives[k*llsq_neq+i]*t1;
    }
  }
llsq_steps[llsq_npar-1]=llsq_errors[llsq_npar-1]/aux[2*llsq_npar-1];
if(llsq_npar == 1)return 0;
for(k=llsq_npar-2; k>=0; k--)
  {
  t1=llsq_errors[k  ];
  for(i=k+1; i<llsq_npar; i++)
    {
    t1-=llsq_derivatives[i*llsq_neq+k]*llsq_steps[i];
    }
  t1/=aux[llsq_npar+k];
  llsq_steps[k]=llsq_steps[ipiv[k]];
  llsq_steps[ipiv[k]  ]=t1;
  }
return 0;
}

int llsq2(void)
{
float aux[2*LLSQ_MAXPAR];
int ipiv[2*LLSQ_MAXPAR];
int kpiv;
int i,j,k;
float t1,t2,sig,piv,beta;
if(llsq_npar > LLSQ_MAXPAR)
  {
  lirerr(1051);
  return -1;
  }
kpiv=0;
piv=0;
for(k=0; k<llsq_npar; k++)
  {
  ipiv[k]=k;
  t1=0;
  for(i=0; i<llsq_neq; i++)
    {
    t1+=(float)pow(llsq_derivatives[k*llsq_neq+i],2.0);
    }
  aux[k]=t1;
  if(t1 > piv)
    {
    piv=t1;
    kpiv=k;
    }
  }
if(piv == 0)return -1;
sig=(float)sqrt(piv);
for(k=0; k<llsq_npar; k++)
  {
  if(kpiv>k)
    {
    t1=aux[k];
    aux[k]=aux[kpiv];
    aux[kpiv]=t1;
    for(i=k; i<llsq_neq; i++)
      {
      t1=llsq_derivatives[k*llsq_neq+i];
      llsq_derivatives[k*llsq_neq+i]=llsq_derivatives[kpiv*llsq_neq+i];
      llsq_derivatives[kpiv*llsq_neq+i]=t1;
      }
    }
  if(k > 0)
    {
    sig=0;
    for(i=k; i<llsq_neq; i++)
      {
      sig+=llsq_derivatives[k*llsq_neq+i]*llsq_derivatives[k*llsq_neq+i];
      }
    sig=(float)sqrt(sig);
    }
  t1=llsq_derivatives[k*llsq_neq+k];
  if(t1 < 0 )sig=-sig;
  ipiv[kpiv]=ipiv[k];
  ipiv[k]=kpiv;
  beta=t1+sig;
  llsq_derivatives[k*llsq_neq+k]=beta;
  beta=1/(sig*beta);
  j=llsq_npar+k;
  aux[j]=-sig;
  if(k<llsq_npar-1)
    {
    piv=0;
    kpiv=k+1;
    for(j=k+1; j<llsq_npar; j++)
      {
      t1=0;
      for(i=k; i<llsq_neq; i++)
        {
        t1+=llsq_derivatives[k*llsq_neq+i]*llsq_derivatives[j*llsq_neq+i];
        }
      t1=beta*t1;
      for(i=k; i<llsq_neq; i++)
        {
        llsq_derivatives[j*llsq_neq+i]-=llsq_derivatives[k*llsq_neq+i]*t1;
        }
      t1=aux[j]-llsq_derivatives[j*llsq_neq+k]*llsq_derivatives[j*llsq_neq+k];
      aux[j]=t1;
      if(t1 > piv)
        {
        piv=t1;
        kpiv=j;
        }
      }
    }
  t1=0;
  t2=0;
  for(i=k; i<llsq_neq; i++)
    {
    t1+=llsq_derivatives[k*llsq_neq+i]*llsq_errors[2*i  ];
    t2+=llsq_derivatives[k*llsq_neq+i]*llsq_errors[2*i+1];
    }
  t1*=beta;
  t2*=beta;
  for(i=k; i<llsq_neq; i++)
    {
    llsq_errors[2*i  ]-=llsq_derivatives[k*llsq_neq+i]*t1;
    llsq_errors[2*i+1]-=llsq_derivatives[k*llsq_neq+i]*t2;
    }
  }
llsq_steps[2*(llsq_npar-1)  ]=
                        llsq_errors[2*(llsq_npar-1)  ]/aux[2*llsq_npar-1];
llsq_steps[2*(llsq_npar-1)+1]=
                        llsq_errors[2*(llsq_npar-1)+1]/aux[2*llsq_npar-1];
if(llsq_npar == 1)return 0;
for(k=llsq_npar-2; k>=0; k--)
  {
  t1=llsq_errors[2*k  ];
  t2=llsq_errors[2*k+1];
  for(i=k+1; i<llsq_npar; i++)
    {
    t1-=llsq_derivatives[i*llsq_neq+k]*llsq_steps[2*i  ];
    t2-=llsq_derivatives[i*llsq_neq+k]*llsq_steps[2*i+1];
    }
  t1/=aux[llsq_npar+k];
  t2/=aux[llsq_npar+k];
  llsq_steps[2*k  ]=llsq_steps[2*ipiv[k]  ];
  llsq_steps[2*k+1]=llsq_steps[2*ipiv[k]+1];
  llsq_steps[2*ipiv[k]  ]=t1;
  llsq_steps[2*ipiv[k]+1]=t2;
  }
return 0;
}

