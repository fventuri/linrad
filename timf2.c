
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
//#include "screendef.h"
//#include "sigdef.h"

void make_timf2(void)
{
float *x;
int i,k1,k2,k3,k4;
if(swfloat)
  {
  fft1_lowlevel_points=0;
  x=&fft1_float[fft1_px];
  if(ui.rx_rf_channels==1)
    {
    for(i=0; i<fft1_first_point; i++)
      {
      fft1_split_float[4*i  ]=0;
      fft1_split_float[4*i+1]=0;
      fft1_split_float[4*i+2]=0;
      fft1_split_float[4*i+3]=0;
      }
    for(i=fft1_first_point; i<=fft1_last_point; i++)
      {
      if(liminfo[i] == 0)
        {
        fft1_lowlevel_points++;
        fft1_split_float[4*i  ]=x[2*i  ];
        fft1_split_float[4*i+1]=x[2*i+1];
        fft1_split_float[4*i+2]=0;
        fft1_split_float[4*i+3]=0;
        }
      else
        {
        fft1_split_float[4*i  ]=0;
        fft1_split_float[4*i+1]=0;
        fft1_split_float[4*i+2]=x[2*i  ];
        fft1_split_float[4*i+3]=x[2*i+1];
        }
      }
    for(i=fft1_last_point+1; i<fft1_size; i++)
      {
      fft1_split_float[4*i  ]=0;
      fft1_split_float[4*i+1]=0;
      fft1_split_float[4*i+2]=0;
      fft1_split_float[4*i+3]=0;
      }
    fft1back_one();
    }
  else
    {
    for(i=0; i<fft1_first_point; i++)
      {
      fft1_split_float[8*i  ]=0;
      fft1_split_float[8*i+1]=0;
      fft1_split_float[8*i+2]=0;
      fft1_split_float[8*i+3]=0;
      fft1_split_float[8*i+4]=0;
      fft1_split_float[8*i+5]=0;
      fft1_split_float[8*i+6]=0;
      fft1_split_float[8*i+7]=0;
      }
    for(i=fft1_first_point; i<=fft1_last_point; i++)
      {
      if(liminfo[i] == 0)
        {
        fft1_lowlevel_points++;
        fft1_split_float[8*i  ]=x[4*i  ];
        fft1_split_float[8*i+1]=x[4*i+1];
        fft1_split_float[8*i+2]=x[4*i+2];
        fft1_split_float[8*i+3]=x[4*i+3];
        fft1_split_float[8*i+4]=0;
        fft1_split_float[8*i+5]=0;
        fft1_split_float[8*i+6]=0;
        fft1_split_float[8*i+7]=0;
        }
      else
        {
        fft1_split_float[8*i  ]=0;
        fft1_split_float[8*i+1]=0;
        fft1_split_float[8*i+2]=0;
        fft1_split_float[8*i+3]=0;
        fft1_split_float[8*i+4]=x[4*i  ];
        fft1_split_float[8*i+5]=x[4*i+1];
        fft1_split_float[8*i+6]=x[4*i+2];
        fft1_split_float[8*i+7]=x[4*i+3];
        }
      }
    for(i=fft1_last_point+1; i<fft1_size; i++)
      {
      fft1_split_float[8*i  ]=0;
      fft1_split_float[8*i+1]=0;
      fft1_split_float[8*i+2]=0;
      fft1_split_float[8*i+3]=0;
      fft1_split_float[8*i+4]=0;
      fft1_split_float[8*i+5]=0;
      fft1_split_float[8*i+6]=0;
      fft1_split_float[8*i+7]=0;
      }
    fft1back_two();
    }
  fft1_px=(fft1_px+fft1_block)&fft1_mask;
  fft1_nx=(fft1_nx+1)&fft1n_mask;  
  }
else
  {  
  if(ui.rx_rf_channels==1)
    {
    split_one();
    fft1_nx=(fft1_nx+1)&fft1n_mask;  
    if(swmmx_fft2 && ampinfo_flag != 0)
      {
      k1=fft1_maxamp[0];
      k2=fft1_maxamp[1];
      for(i=0; i<fft1_size; i++)
        {
        if(abs(fft1_split_shi[4*i  ])>k1)k1=abs(fft1_split_shi[4*i  ]);
        if(abs(fft1_split_shi[4*i+1])>k1)k1=abs(fft1_split_shi[4*i+1]);
        if(abs(fft1_split_shi[4*i+2])>k2)k2=abs(fft1_split_shi[4*i+2]);
        if(abs(fft1_split_shi[4*i+3])>k2)k2=abs(fft1_split_shi[4*i+3]);
        }
      fft1_maxamp[0]=k1;
      fft1_maxamp[1]=k2;
      }
    if( fft_cntrl[FFT1_BCKCURMODE].mmx == 0)
      {
      fft1back_one();
      }    
    else
      {
#if CPU == CPU_INTEL      
      mmx_fft1back_one();
      fft1back_mmx_finish();
#else
      lirerr(886634);
#endif      
      }
    }
  else
    {
    split_two();
    fft1_nx=(fft1_nx+1)&fft1n_mask;  
    if(swmmx_fft2 && ampinfo_flag != 0)
      {
      k1=fft1_maxamp[0];
      k2=fft1_maxamp[2];
      k3=fft1_maxamp[1];
      k4=fft1_maxamp[3];
      for(i=0; i<fft1_size; i++)
        {
        if(abs(fft1_split_shi[8*i  ])>k1)k1=abs(fft1_split_shi[8*i  ]);
        if(abs(fft1_split_shi[8*i+1])>k1)k1=abs(fft1_split_shi[8*i+1]);
        if(abs(fft1_split_shi[8*i+2])>k2)k2=abs(fft1_split_shi[8*i+2]);
        if(abs(fft1_split_shi[8*i+3])>k2)k2=abs(fft1_split_shi[8*i+3]);
        if(abs(fft1_split_shi[8*i+4])>k3)k3=abs(fft1_split_shi[8*i+4]);
        if(abs(fft1_split_shi[8*i+5])>k3)k3=abs(fft1_split_shi[8*i+5]);
        if(abs(fft1_split_shi[8*i+6])>k4)k4=abs(fft1_split_shi[8*i+6]);
        if(abs(fft1_split_shi[8*i+7])>k4)k4=abs(fft1_split_shi[8*i+7]);
        }
      fft1_maxamp[0]=k1;
      fft1_maxamp[2]=k2;
      fft1_maxamp[1]=k3;
      fft1_maxamp[3]=k4;
      }
    if( fft_cntrl[FFT1_BCKCURMODE].mmx == 0)
      {
      fft1back_two();
      }  
    else
      {
#if CPU == CPU_INTEL      
      mmx_fft1back_two();
      fft1back_mmx_finish();
#else
      lirerr(886635);
#endif      
      }
    }
  }
fft1_lowlevel_fraction=0.02*(49*fft1_lowlevel_fraction+fft1_lowlevel_points/
             ((float)(fft1_last_point-fft1_first_point)));
timf2_pa=(timf2_pa+timf2_input_block)&timf2_mask;
}

void fft1back_two(void)
{
int ia, ib, ic, id, itab;
int inc,j,k,n;
int m1, m2, siz,siz_d4;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float u0,u1,u2,u3,u4,u5,u6,u7,u8,u9,u10,u11,u12;
float w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12;
float x0,x1,x2,x3,x4,x5,x6,x7;
float ya,yb,y2,y3,y4,y5,y6,y7;
float s1,s2,c1,c2;
siz=fft1_size;
siz_d4=fft1_size/4;
if(swfloat)
  {
  for(j=0; j<siz; j+=4)
    {
    ia=fft1_back_scramble[j  ]<<3;
    ib=fft1_back_scramble[j+1]<<3;
    ic=fft1_back_scramble[j+2]<<3;
    id=fft1_back_scramble[j+3]<<3;
    k=j<<3;
    t1=fft1_split_float[ia  ]+fft1_split_float[ib  ];
    t2=fft1_split_float[ia+1]+fft1_split_float[ib+1];
    r1=fft1_split_float[ia+2]+fft1_split_float[ib+2];
    r2=fft1_split_float[ia+3]+fft1_split_float[ib+3];

    t3=fft1_split_float[ic  ]+fft1_split_float[id  ];
    t4=fft1_split_float[ic+1]+fft1_split_float[id+1];
    r3=fft1_split_float[ic+2]+fft1_split_float[id+2];
    r4=fft1_split_float[ic+3]+fft1_split_float[id+3];

    timf2_tmp[k  ]=t1+t3;
    timf2_tmp[k+1]=t2+t4;
    timf2_tmp[k+2]=r1+r3;
    timf2_tmp[k+3]=r2+r4;

    timf2_tmp[k+16]=t1-t3;
    timf2_tmp[k+17]=t2-t4;
    timf2_tmp[k+18]=r1-r3;
    timf2_tmp[k+19]=r2-r4;

    u1=fft1_split_float[ia+4]+fft1_split_float[ib+4];
    u2=fft1_split_float[ia+5]+fft1_split_float[ib+5];
    w1=fft1_split_float[ia+6]+fft1_split_float[ib+6];
    w2=fft1_split_float[ia+7]+fft1_split_float[ib+7];

    u3=fft1_split_float[ic+4]+fft1_split_float[id+4];
    u4=fft1_split_float[ic+5]+fft1_split_float[id+5];
    w3=fft1_split_float[ic+6]+fft1_split_float[id+6];
    w4=fft1_split_float[ic+7]+fft1_split_float[id+7];

    timf2_tmp[k+4]=u1+u3;
    timf2_tmp[k+5]=u2+u4;
    timf2_tmp[k+6]=w1+w3;
    timf2_tmp[k+7]=w2+w4;

    timf2_tmp[k+20]=u1-u3;
    timf2_tmp[k+21]=u2-u4;
    timf2_tmp[k+22]=w1-w3;
    timf2_tmp[k+23]=w2-w4;

    t5=fft1_split_float[ia  ]-fft1_split_float[ib  ];
    t7=fft1_split_float[ia+1]-fft1_split_float[ib+1];
    r5=fft1_split_float[ia+2]-fft1_split_float[ib+2];
    r7=fft1_split_float[ia+3]-fft1_split_float[ib+3];

    t10=fft1_split_float[ic  ]-fft1_split_float[id  ];
    t6= fft1_split_float[ic+1]-fft1_split_float[id+1];
    r10=fft1_split_float[ic+2]-fft1_split_float[id+2];  
    r6= fft1_split_float[ic+3]-fft1_split_float[id+3];

    t11=t5-t6;
    t8=t7-t10;
    r11=r5-r6;
    r8=r7-r10;
  
    t12=t5+t6;
    t9=t7+t10;
    r12=r5+r6;
    r9=r7+r10;

    timf2_tmp[k+8]=t12;
    timf2_tmp[k+9]=t8;
    timf2_tmp[k+10]=r12;
    timf2_tmp[k+11]=r8;

    timf2_tmp[k+24]=t11;
    timf2_tmp[k+25]=t9;
    timf2_tmp[k+26]=r11;
    timf2_tmp[k+27]=r9;

    u5=fft1_split_float[ia+4]-fft1_split_float[ib+4];
    u7=fft1_split_float[ia+5]-fft1_split_float[ib+5];
    w5=fft1_split_float[ia+6]-fft1_split_float[ib+6];
    w7=fft1_split_float[ia+7]-fft1_split_float[ib+7];

    u10=fft1_split_float[ic+4]-fft1_split_float[id+4];
    u6= fft1_split_float[ic+5]-fft1_split_float[id+5];
    w10=fft1_split_float[ic+6]-fft1_split_float[id+6];  
    w6= fft1_split_float[ic+7]-fft1_split_float[id+7];

    u11=u5-u6;
    u8=u7-u10;
    w11=w5-w6;
    w8=w7-w10;
  
    u12=u5+u6;
    u9=u7+u10;
    w12=w5+w6;
    w9=w7+w10;

    timf2_tmp[k+12]=u12;
    timf2_tmp[k+13]=u8;
    timf2_tmp[k+14]=w12;
    timf2_tmp[k+15]=w8;

    timf2_tmp[k+28]=u11;
    timf2_tmp[k+29]=u9;
    timf2_tmp[k+30]=w11;
    timf2_tmp[k+31]=w9;
    }
  }
else
  {  
  for(j=0; j<siz; j+=4)
    {
    ia=fft1_back_scramble[j  ]<<3;
    ib=fft1_back_scramble[j+1]<<3;
    ic=fft1_back_scramble[j+2]<<3;
    id=fft1_back_scramble[j+3]<<3;
    k=j<<3;
    t1=fft1_split_shi[ia  ]+fft1_split_shi[ib  ];
    t2=fft1_split_shi[ia+1]+fft1_split_shi[ib+1];
    r1=fft1_split_shi[ia+2]+fft1_split_shi[ib+2];
    r2=fft1_split_shi[ia+3]+fft1_split_shi[ib+3];

    t3=fft1_split_shi[ic  ]+fft1_split_shi[id  ];
    t4=fft1_split_shi[ic+1]+fft1_split_shi[id+1];
    r3=fft1_split_shi[ic+2]+fft1_split_shi[id+2];
    r4=fft1_split_shi[ic+3]+fft1_split_shi[id+3];

    timf2_tmp[k  ]=t1+t3;
    timf2_tmp[k+1]=t2+t4;
    timf2_tmp[k+2]=r1+r3;
    timf2_tmp[k+3]=r2+r4;

    timf2_tmp[k+16]=t1-t3;
    timf2_tmp[k+17]=t2-t4;
    timf2_tmp[k+18]=r1-r3;
    timf2_tmp[k+19]=r2-r4;

    u1=fft1_split_shi[ia+4]+fft1_split_shi[ib+4];
    u2=fft1_split_shi[ia+5]+fft1_split_shi[ib+5];
    w1=fft1_split_shi[ia+6]+fft1_split_shi[ib+6];
    w2=fft1_split_shi[ia+7]+fft1_split_shi[ib+7];

    u3=fft1_split_shi[ic+4]+fft1_split_shi[id+4];
    u4=fft1_split_shi[ic+5]+fft1_split_shi[id+5];
    w3=fft1_split_shi[ic+6]+fft1_split_shi[id+6];
    w4=fft1_split_shi[ic+7]+fft1_split_shi[id+7];

    timf2_tmp[k+4]=u1+u3;
    timf2_tmp[k+5]=u2+u4;
    timf2_tmp[k+6]=w1+w3;
    timf2_tmp[k+7]=w2+w4;

    timf2_tmp[k+20]=u1-u3;
    timf2_tmp[k+21]=u2-u4;
    timf2_tmp[k+22]=w1-w3;
    timf2_tmp[k+23]=w2-w4;

    t5=fft1_split_shi[ia  ]-fft1_split_shi[ib  ];
    t7=fft1_split_shi[ia+1]-fft1_split_shi[ib+1];
    r5=fft1_split_shi[ia+2]-fft1_split_shi[ib+2];
    r7=fft1_split_shi[ia+3]-fft1_split_shi[ib+3];

    t10=fft1_split_shi[ic  ]-fft1_split_shi[id  ];
    t6= fft1_split_shi[ic+1]-fft1_split_shi[id+1];
    r10=fft1_split_shi[ic+2]-fft1_split_shi[id+2];  
    r6= fft1_split_shi[ic+3]-fft1_split_shi[id+3];

    t11=t5-t6;
    t8=t7-t10;
    r11=r5-r6;
    r8=r7-r10;
  
    t12=t5+t6;
    t9=t7+t10;
    r12=r5+r6;
    r9=r7+r10;

    timf2_tmp[k+8]=t12;
    timf2_tmp[k+9]=t8;
    timf2_tmp[k+10]=r12;
    timf2_tmp[k+11]=r8;

    timf2_tmp[k+24]=t11;
    timf2_tmp[k+25]=t9;
    timf2_tmp[k+26]=r11;
    timf2_tmp[k+27]=r9;

    u5=fft1_split_shi[ia+4]-fft1_split_shi[ib+4];
    u7=fft1_split_shi[ia+5]-fft1_split_shi[ib+5];
    w5=fft1_split_shi[ia+6]-fft1_split_shi[ib+6];
    w7=fft1_split_shi[ia+7]-fft1_split_shi[ib+7];

    u10=fft1_split_shi[ic+4]-fft1_split_shi[id+4];
    u6= fft1_split_shi[ic+5]-fft1_split_shi[id+5];
    w10=fft1_split_shi[ic+6]-fft1_split_shi[id+6];  
    w6= fft1_split_shi[ic+7]-fft1_split_shi[id+7];

    u11=u5-u6;
    u8=u7-u10;
    w11=w5-w6;
    w8=w7-w10;
  
    u12=u5+u6;
    u9=u7+u10;
    w12=w5+w6;
    w9=w7+w10;

    timf2_tmp[k+12]=u12;
    timf2_tmp[k+13]=u8;
    timf2_tmp[k+14]=w12;
    timf2_tmp[k+15]=w8;

    timf2_tmp[k+28]=u11;
    timf2_tmp[k+29]=u9;
    timf2_tmp[k+30]=w11;
    timf2_tmp[k+31]=w9;
    }
  }
m1=32;
m2=16;
inc=fft1_size/16;
for(n=3; n<fft1_n; n+=2)
  {
  if(yieldflag_timf2_fft1)lir_sched_yield();
  for(j=0; j<fft1_size; j+=m2)
    {
    itab=0;
    ia=j<<3;
lp1:;    
    s1= fft1_backtab[2*itab].sin; 
    c1= fft1_backtab[2*itab].cos;
    s2= fft1_backtab[itab].sin; 
    c2= fft1_backtab[itab].cos;

    x0=timf2_tmp[ia  ];
    ya=timf2_tmp[ia+1];
    u0=timf2_tmp[ia+2];
    w0=timf2_tmp[ia+3];
    ib=ia+m1;

    x1=timf2_tmp[ib  ];
    yb=timf2_tmp[ib+1];
    u1=timf2_tmp[ib+2];
    w1=timf2_tmp[ib+3];

    t1=c1*x1+s1*yb;      
    t2=c1*yb-s1*x1;      
    ic=ib+m1;
    r1=c1*u1+s1*w1;      
    r2=c1*w1-s1*u1;      
    id=ic+m1;

    x4=x0+t1;
    y4=ya+t2;
    u4=u0+r1;
    w4=w0+r2;

    x6=x0-t1;
    y6=ya-t2;
    u6=u0-r1;
    w6=w0-r2;

    x2=timf2_tmp[ic  ];
    y2=timf2_tmp[ic+1];
    u2=timf2_tmp[ic+2];
    w2=timf2_tmp[ic+3];

    x3=timf2_tmp[id  ];
    y3=timf2_tmp[id+1];
    u3=timf2_tmp[id+2];
    w3=timf2_tmp[id+3];

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

    timf2_tmp[ia  ]=x4+t5;
    timf2_tmp[ia+1]=y4+t6;
    timf2_tmp[ia+2]=u4+r5;
    timf2_tmp[ia+3]=w4+r6;

    timf2_tmp[ic  ]=x4-t5;
    timf2_tmp[ic+1]=y4-t6;
    timf2_tmp[ic+2]=u4-r5;
    timf2_tmp[ic+3]=w4-r6;

    t8=c2*x7+s2*y7;
    t7=c2*y7-s2*x7;
    r8=c2*u7+s2*w7;
    r7=c2*w7-s2*u7;

    timf2_tmp[ib  ]=x6+t7;
    timf2_tmp[ib+1]=y6-t8;
    timf2_tmp[ib+2]=u6+r7;
    timf2_tmp[ib+3]=w6-r8;

    timf2_tmp[id  ]=x6-t7;
    timf2_tmp[id+1]=y6+t8;
    timf2_tmp[id+2]=u6-r7;
    timf2_tmp[id+3]=w6+r8;

    x0=timf2_tmp[ia+4];
    ya=timf2_tmp[ia+5];
    u0=timf2_tmp[ia+6];
    w0=timf2_tmp[ia+7];

    x1=timf2_tmp[ib+4];
    yb=timf2_tmp[ib+5];
    u1=timf2_tmp[ib+6];
    w1=timf2_tmp[ib+7];

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

    x2=timf2_tmp[ic+4];
    y2=timf2_tmp[ic+5];
    u2=timf2_tmp[ic+6];
    w2=timf2_tmp[ic+7];

    x3=timf2_tmp[id+4];
    y3=timf2_tmp[id+5];
    u3=timf2_tmp[id+6];
    w3=timf2_tmp[id+7];

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


    timf2_tmp[ia+4]=x4+t5;
    timf2_tmp[ia+5]=y4+t6;
    timf2_tmp[ia+6]=u4+r5;
    timf2_tmp[ia+7]=w4+r6;

    timf2_tmp[ic+4]=x4-t5;
    timf2_tmp[ic+5]=y4-t6;
    timf2_tmp[ic+6]=u4-r5;
    timf2_tmp[ic+7]=w4-r6;

    t8=c2*x7+s2*y7;
    t7=c2*y7-s2*x7;
    r8=c2*u7+s2*w7;
    r7=c2*w7-s2*u7;

    timf2_tmp[ib+4]=x6+t7;
    timf2_tmp[ib+5]=y6-t8;
    timf2_tmp[ib+6]=u6+r7;
    timf2_tmp[ib+7]=w6-r8;

    timf2_tmp[id+4]=x6-t7;
    timf2_tmp[id+5]=y6+t8;
    timf2_tmp[id+6]=u6-r7;
    timf2_tmp[id+7]=w6+r8;

    itab+=inc;
    ia+=8; 
    if(itab<siz_d4)goto lp1;
    }
  inc/=4;
  m1*=4;
  m2*=4;
  }
if( (fft1_n&1)==1)
  {
  ib=fft1_size*4;
  ic=fft1_size/2;
  ia=0;
  for(itab=0; itab<ic; itab++)
    {
    s1= fft1_backtab[itab].sin; 
    c1= fft1_backtab[itab].cos;
    x0=timf2_tmp[ia  ];
    ya=timf2_tmp[ia+1];
    u0=timf2_tmp[ia+2];
    w0=timf2_tmp[ia+3];
    x1=timf2_tmp[ib  ];
    yb=timf2_tmp[ib+1];
    u1=timf2_tmp[ib+2];
    w1=timf2_tmp[ib+3];
    t1=c1*x1+s1*yb;
    t2=c1*yb-s1*x1;
    r1=c1*u1+s1*w1;
    r2=c1*w1-s1*u1;
    timf2_tmp[ib  ]=x0-t1;
    timf2_tmp[ib+1]=ya-t2;
    timf2_tmp[ib+2]=u0-r1;
    timf2_tmp[ib+3]=w0-r2;
    timf2_tmp[ia  ]=x0+t1;
    timf2_tmp[ia+1]=ya+t2;
    timf2_tmp[ia+2]=u0+r1;
    timf2_tmp[ia+3]=w0+r2;
    x0=timf2_tmp[ia+4];
    ya=timf2_tmp[ia+5];
    u0=timf2_tmp[ia+6];
    w0=timf2_tmp[ia+7];
    x1=timf2_tmp[ib+4];
    yb=timf2_tmp[ib+5];
    u1=timf2_tmp[ib+6];
    w1=timf2_tmp[ib+7];
    t1=c1*x1+s1*yb;
    t2=c1*yb-s1*x1;
    r1=c1*u1+s1*w1;
    r2=c1*w1-s1*u1;
    timf2_tmp[ib+4]=x0-t1;
    timf2_tmp[ib+5]=ya-t2;
    timf2_tmp[ib+6]=u0-r1;
    timf2_tmp[ib+7]=w0-r2;
    timf2_tmp[ia+4]=x0+t1;
    timf2_tmp[ia+5]=ya+t2;
    timf2_tmp[ia+6]=u0+r1;
    timf2_tmp[ia+7]=w0+r2;
    ia+=8;
    ib+=8;
    }
  }
if(yieldflag_timf2_fft1)lir_sched_yield();
fft1back_fp_finish();
}

void fft1back_one(void)
{
int ia, ib, ic, id, itab;
int inc,j,k,n;
int m1, m2, siz, siz_d4, siz_m4, siz_m2, siz_d2;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float x0,x1,x2,x3,x4,x5,x6,x7;
float ya,yb,y2,y3,y4,y5,y6,y7;
float u0,u1,u2,u3,u4,u5,u6,u7;
float w0,w1,w2,w3,w4,w5,w6,w7;
float s1,s2,c1,c2;
siz_d4=fft1_size/4;
siz_m4=fft1_size*4;
siz=fft1_size;
if(swfloat)
  {
  for(j=0; j<siz; j+=4)
    {
    ia=fft1_back_scramble[j  ]<<2;
    ib=fft1_back_scramble[j+1]<<2;
    ic=fft1_back_scramble[j+2]<<2;
    id=fft1_back_scramble[j+3]<<2;
  
    t1=fft1_split_float[ia  ]+fft1_split_float[ib  ];
    t2=fft1_split_float[ia+1]+fft1_split_float[ib+1];
    r1=fft1_split_float[ia+2]+fft1_split_float[ib+2];
    r2=fft1_split_float[ia+3]+fft1_split_float[ib+3];
    k=j<<2;
    t3=fft1_split_float[ic  ]+fft1_split_float[id  ];
    t4=fft1_split_float[ic+1]+fft1_split_float[id+1];
    r3=fft1_split_float[ic+2]+fft1_split_float[id+2];
    r4=fft1_split_float[ic+3]+fft1_split_float[id+3];

    t5=fft1_split_float[ia  ]-fft1_split_float[ib  ];
    t7=fft1_split_float[ia+1]-fft1_split_float[ib+1];
    r5=fft1_split_float[ia+2]-fft1_split_float[ib+2];
    r7=fft1_split_float[ia+3]-fft1_split_float[ib+3];

    t10=fft1_split_float[ic  ]-fft1_split_float[id  ];
    t6= fft1_split_float[ic+1]-fft1_split_float[id+1];
    r10=fft1_split_float[ic+2]-fft1_split_float[id+2];  
    r6= fft1_split_float[ic+3]-fft1_split_float[id+3];

    timf2_tmp[k  ]=t1+t3;
    timf2_tmp[k+1]=t2+t4;
    timf2_tmp[k+2]=r1+r3;
    timf2_tmp[k+3]=r2+r4;

    timf2_tmp[k+8]=t1-t3;
    timf2_tmp[k+9]=t2-t4;
    timf2_tmp[k+10]=r1-r3;
    timf2_tmp[k+11]=r2-r4;

    t11=t5-t6;
    t8=t7-t10;
    r11=r5-r6;
    r8=r7-r10;
  
    t12=t5+t6;
    t9=t7+t10;
    r12=r5+r6;
    r9=r7+r10;

    timf2_tmp[k+4]=t12;
    timf2_tmp[k+5]=t8;
    timf2_tmp[k+6]=r12;
    timf2_tmp[k+7]=r8;

    timf2_tmp[k+12]=t11;
    timf2_tmp[k+13]=t9;
    timf2_tmp[k+14]=r11;
    timf2_tmp[k+15]=r9;
    }
  }
else
  {  
  for(j=0; j<siz; j+=4)
    {
    ia=fft1_back_scramble[j  ]<<2;
    ib=fft1_back_scramble[j+1]<<2;
    ic=fft1_back_scramble[j+2]<<2;
    id=fft1_back_scramble[j+3]<<2;
  
    t1=fft1_split_shi[ia  ]+fft1_split_shi[ib  ];
    t2=fft1_split_shi[ia+1]+fft1_split_shi[ib+1];
    r1=fft1_split_shi[ia+2]+fft1_split_shi[ib+2];
    r2=fft1_split_shi[ia+3]+fft1_split_shi[ib+3];
    k=j<<2;
    t3=fft1_split_shi[ic  ]+fft1_split_shi[id  ];
    t4=fft1_split_shi[ic+1]+fft1_split_shi[id+1];
    r3=fft1_split_shi[ic+2]+fft1_split_shi[id+2];
    r4=fft1_split_shi[ic+3]+fft1_split_shi[id+3];

    t5=fft1_split_shi[ia  ]-fft1_split_shi[ib  ];
    t7=fft1_split_shi[ia+1]-fft1_split_shi[ib+1];
    r5=fft1_split_shi[ia+2]-fft1_split_shi[ib+2];
    r7=fft1_split_shi[ia+3]-fft1_split_shi[ib+3];

    t10=fft1_split_shi[ic  ]-fft1_split_shi[id  ];
    t6= fft1_split_shi[ic+1]-fft1_split_shi[id+1];
    r10=fft1_split_shi[ic+2]-fft1_split_shi[id+2];  
    r6= fft1_split_shi[ic+3]-fft1_split_shi[id+3];

    timf2_tmp[k  ]=t1+t3;
    timf2_tmp[k+1]=t2+t4;
    timf2_tmp[k+2]=r1+r3;
    timf2_tmp[k+3]=r2+r4;

    timf2_tmp[k+8]=t1-t3;
    timf2_tmp[k+9]=t2-t4;
    timf2_tmp[k+10]=r1-r3;
    timf2_tmp[k+11]=r2-r4;

    t11=t5-t6;
    t8=t7-t10;
    r11=r5-r6;
    r8=r7-r10;
  
    t12=t5+t6;
    t9=t7+t10;
    r12=r5+r6;
    r9=r7+r10;

    timf2_tmp[k+4]=t12;
    timf2_tmp[k+5]=t8;
    timf2_tmp[k+6]=r12;
    timf2_tmp[k+7]=r8;

    timf2_tmp[k+12]=t11;
    timf2_tmp[k+13]=t9;
    timf2_tmp[k+14]=r11;
    timf2_tmp[k+15]=r9;
    }
  }  
m1=64;
m2=16;
inc=fft1_size/16;
for(n=2; n<fft1_n-1; n+=2)
  {
  if(yieldflag_timf2_fft1)lir_sched_yield();
  for(j=0; j<siz_m4; j+=m1)
    {
    itab=0;
    ia=j;
lp1:;    
    ib=ia+m2;
    s1= fft1_backtab[2*itab].sin; 
    c1= fft1_backtab[2*itab].cos;
    s2= fft1_backtab[itab].sin; 
    c2= fft1_backtab[itab].cos;

    x0=timf2_tmp[ia  ];
    ya=timf2_tmp[ia+1];
    u0=timf2_tmp[ia+2];
    w0=timf2_tmp[ia+3];

    x1=timf2_tmp[ib  ];
    yb=timf2_tmp[ib+1];
    u1=timf2_tmp[ib+2];
    w1=timf2_tmp[ib+3];

    t1=c1*x1+s1*yb;      
    t2=c1*yb-s1*x1;      
    r1=c1*u1+s1*w1;      
    r2=c1*w1-s1*u1;      
    ic=ib+m2;
    id=ic+m2;

    x4=x0+t1;
    y4=ya+t2;
    u4=u0+r1;
    w4=w0+r2;

    x6=x0-t1;
    y6=ya-t2;
    u6=u0-r1;
    w6=w0-r2;

    x2=timf2_tmp[ic  ];
    y2=timf2_tmp[ic+1];
    u2=timf2_tmp[ic+2];
    w2=timf2_tmp[ic+3];

    x3=timf2_tmp[id  ];
    y3=timf2_tmp[id+1];
    u3=timf2_tmp[id+2];
    w3=timf2_tmp[id+3];

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

    timf2_tmp[ia  ]=x4+t5;
    timf2_tmp[ia+1]=y4+t6;
    timf2_tmp[ia+2]=u4+r5;
    timf2_tmp[ia+3]=w4+r6;

    timf2_tmp[ic  ]=x4-t5;
    timf2_tmp[ic+1]=y4-t6;
    timf2_tmp[ic+2]=u4-r5;
    timf2_tmp[ic+3]=w4-r6;

    t8=c2*x7+s2*y7;
    t7=c2*y7-s2*x7;
    r8=c2*u7+s2*w7;
    r7=c2*w7-s2*u7;

    timf2_tmp[ib  ]=x6+t7;
    timf2_tmp[ib+1]=y6-t8;
    timf2_tmp[ib+2]=u6+r7;
    timf2_tmp[ib+3]=w6-r8;

    timf2_tmp[id  ]=x6-t7;
    timf2_tmp[id+1]=y6+t8;
    timf2_tmp[id+2]=u6-r7;
    timf2_tmp[id+3]=w6+r8;

    itab+=inc;
    ia+=4; 
    if(itab<siz_d4)goto lp1;
    }
  inc/=4;
  m1*=4;
  m2*=4;
  }
if( (fft1_n&1)==1)
  {
  siz_m2=fft1_size*2;
  siz_d2=fft1_size/2;
  ib=siz_m2;
  ia=0;
  for(itab=0; itab<siz_d2; itab++)
    {
    s1= fft1_backtab[itab].sin; 
    c1= fft1_backtab[itab].cos;
    x0=timf2_tmp[ia  ];
    ya=timf2_tmp[ia+1];
    u0=timf2_tmp[ia+2];
    w0=timf2_tmp[ia+3];
    x1=timf2_tmp[ib  ];
    yb=timf2_tmp[ib+1];
    u1=timf2_tmp[ib+2];
    w1=timf2_tmp[ib+3];
    t1=c1*x1+s1*yb;
    t2=c1*yb-s1*x1;
    r1=c1*u1+s1*w1;
    r2=c1*w1-s1*u1;
    timf2_tmp[ib  ]=x0-t1;
    timf2_tmp[ib+1]=ya-t2;
    timf2_tmp[ib+2]=u0-r1;
    timf2_tmp[ib+3]=w0-r2;
    timf2_tmp[ia  ]=x0+t1;
    timf2_tmp[ia+1]=ya+t2;
    timf2_tmp[ia+2]=u0+r1;
    timf2_tmp[ia+3]=w0+r2;
    ia+=4;
    ib+=4;
    }
  }
if(yieldflag_timf2_fft1)lir_sched_yield();
fft1back_fp_finish();
}


void fft1back_fp_finish(void)
{
int i, ia, ib;
int j, k, siz;
int p0, pp, kk, m, mm;
float t1, ampfac;
ampfac=1.0/(1<<genparm[FIRST_BCKFFT_ATT_N]);
p0=timf2_pa;  
mm=4*ui.rx_rf_channels;
pp=p0/mm;
m=timf2_mask;
siz=fft1_size;
if(swfloat)
  {
  if(ui.rx_rf_channels==1)
    {
    if(fft1_interleave_points==0)
      {
// Without window we just transfer the points.
      for(i=0; i<siz; i++)
        {
        timf2_float[p0  ]=ampfac*timf2_tmp[mm*i  ];
        timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
        timf2_float[p0+1]=ampfac*timf2_tmp[mm*i+1];
        timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
        timf2_float[p0+2]=ampfac*timf2_tmp[mm*i+2];
        timf2_float[p0+3]=ampfac*timf2_tmp[mm*i+3];
        pp++;
        p0+=4;
        }  
      }
    else
      {
      if(fft1_interleave_points==fft1_size/2)
        {
    // With a sin squared window we note that sin*sin+cos*cos=1 and add data.
        k=fft1_size/2;
        for(i=0; i<k; i++)
          {
          timf2_float[p0  ]+=ampfac*timf2_tmp[mm*i  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]+=ampfac*timf2_tmp[mm*i+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]+=ampfac*timf2_tmp[mm*i+2];
          timf2_float[p0+3]+=ampfac*timf2_tmp[mm*i+3];
          pp++;
          p0+=4;
          }
        j=k*4;
        k=fft1_size*4;
        p0=p0&m;
        for(i=j; i<k; i++)
          {
          timf2_float[p0]=timf2_tmp[i]*ampfac; 
          p0++;  
          }
        }
      else
        {
    // With an arbitrary window we multiply by the inverted window function
    // and take the center portion of each transform.
        ia=fft1_interleave_points/2;
        ib=fft1_size/2;
        kk=ia*4;
        for(i=ia; i<ib; i++)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>2;
          timf2_float[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]=t1*timf2_tmp[kk+2];
          timf2_float[p0+3]=t1*timf2_tmp[kk+3];
          pp++;
          p0=(p0+4)&m;  
          kk+=4;
          }
        for(i=ib; i>ia; i--)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>2;
          timf2_float[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]=t1*timf2_tmp[kk+2];
          timf2_float[p0+3]=t1*timf2_tmp[kk+3];
          pp++;
          p0=(p0+4)&m;  
          kk+=4;
          }
        }
      }
    }  
  else
    {  
    if(fft1_interleave_points==0)
      {
    // Without window we just transfer the points.
      for(i=0; i<siz; i++)
        {
        timf2_float[p0  ]=ampfac*timf2_tmp[mm*i  ];
        timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
        timf2_float[p0+1]=ampfac*timf2_tmp[mm*i+1];
        timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
        timf2_float[p0+2]=ampfac*timf2_tmp[mm*i+2];
        timf2_pwr_float[pp]+=timf2_float[p0+2]*timf2_float[p0+2];
        timf2_float[p0+3]=ampfac*timf2_tmp[mm*i+3];
        timf2_pwr_float[pp]+=timf2_float[p0+3]*timf2_float[p0+3];
        timf2_float[p0+4]=ampfac*timf2_tmp[mm*i+4];
        timf2_float[p0+5]=ampfac*timf2_tmp[mm*i+5];
        timf2_float[p0+6]=ampfac*timf2_tmp[mm*i+6];
        timf2_float[p0+7]=ampfac*timf2_tmp[mm*i+7];
        pp++;
        p0+=8;
        }  
      }
    else
      {
      if(fft1_interleave_points==fft1_size/2)
        {
    // With a sin squared window we note that sin*sin+cos*cos=1 and add data.
        k=fft1_size/2;
        for(i=0; i<k; i++)
          {
          timf2_float[p0  ]+=ampfac*timf2_tmp[mm*i  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]+=ampfac*timf2_tmp[mm*i+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]+=ampfac*timf2_tmp[mm*i+2];
          timf2_pwr_float[pp]+=timf2_float[p0+2]*timf2_float[p0+2];
          timf2_float[p0+3]+=ampfac*timf2_tmp[mm*i+3];
          timf2_pwr_float[pp]+=timf2_float[p0+3]*timf2_float[p0+3];
          timf2_float[p0+4]+=ampfac*timf2_tmp[mm*i+4];
          timf2_float[p0+5]+=ampfac*timf2_tmp[mm*i+5];
          timf2_float[p0+6]+=ampfac*timf2_tmp[mm*i+6];
          timf2_float[p0+7]+=ampfac*timf2_tmp[mm*i+7];
          pp++;
          p0+=8;
          }
        j=k*8;
        k=fft1_size*8;
        p0=p0&m;
        for(i=j; i<k; i++)
          {
          timf2_float[p0]=timf2_tmp[i]*ampfac; 
          p0++;  
          }
        }
      else
        {
    // With an arbitrary window we multiply by the inverted window function
    // and take the center portion of each transform.
        ia=fft1_interleave_points/2;
        ib=fft1_size/2;
        kk=ia*8;
        for(i=ia; i<ib; i++)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>3;
          timf2_float[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]=t1*timf2_tmp[kk+2];
          timf2_pwr_float[pp]+=timf2_float[p0+2]*timf2_float[p0+2];
          timf2_float[p0+3]=t1*timf2_tmp[kk+3];
          timf2_pwr_float[pp]+=timf2_float[p0+3]*timf2_float[p0+3];
          timf2_float[p0+4]=t1*timf2_tmp[kk+4];
          timf2_float[p0+5]=t1*timf2_tmp[kk+5];
          timf2_float[p0+6]=t1*timf2_tmp[kk+6];
          timf2_float[p0+7]=t1*timf2_tmp[kk+7];
          pp++;
          p0=(p0+8)&m;  
          kk+=8;
          }
        for(i=ib; i>ia; i--)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>3;
          timf2_float[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_float[pp]=timf2_float[p0]*timf2_float[p0];
          timf2_float[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_float[pp]+=timf2_float[p0+1]*timf2_float[p0+1];
          timf2_float[p0+2]=t1*timf2_tmp[kk+2];
          timf2_pwr_float[pp]+=timf2_float[p0+2]*timf2_float[p0+2];
          timf2_float[p0+3]=t1*timf2_tmp[kk+3];
          timf2_pwr_float[pp]+=timf2_float[p0+3]*timf2_float[p0+3];
          timf2_float[p0+4]=t1*timf2_tmp[kk+4];
          timf2_float[p0+5]=t1*timf2_tmp[kk+5];
          timf2_float[p0+6]=t1*timf2_tmp[kk+6];
          timf2_float[p0+7]=t1*timf2_tmp[kk+7];
          pp++;
          p0=(p0+8)&m;  
          kk+=8;
          }
        }
      }
    }
  }
else
  {  
  if(ui.rx_rf_channels==1)
    {
    if(fft1_interleave_points==0)
      {
  // Without window we just transfer the points.
      for(i=0; i<siz; i++)
        {
        timf2_shi[p0  ]=ampfac*timf2_tmp[mm*i  ];
        timf2_pwr_int[pp]=timf2_shi[p0]*timf2_shi[p0];
        timf2_shi[p0+1]=ampfac*timf2_tmp[mm*i+1];
        timf2_pwr_int[pp]+=timf2_shi[p0+1]*timf2_shi[p0+1];
        timf2_shi[p0+2]=ampfac*timf2_tmp[mm*i+2];
        timf2_shi[p0+3]=ampfac*timf2_tmp[mm*i+3];
        pp++;
        p0+=4;
        }  
      }
    else
      {
      if(fft1_interleave_points==fft1_size/2)
        {
    // With a sin squared window we note that sin*sin+cos*cos=1 and add data.
        k=fft1_size/2;
        for(i=0; i<k; i++)
          {
          timf2_shi[p0  ]+=ampfac*timf2_tmp[mm*i  ];
          timf2_pwr_int[pp]=timf2_shi[p0]*timf2_shi[p0];
          timf2_shi[p0+1]+=ampfac*timf2_tmp[mm*i+1];
          timf2_pwr_int[pp]+=timf2_shi[p0+1]*timf2_shi[p0+1];
          timf2_shi[p0+2]+=ampfac*timf2_tmp[mm*i+2];
          timf2_shi[p0+3]+=ampfac*timf2_tmp[mm*i+3];
          pp++;
          p0+=4;
          }
        j=k*4;
        k=fft1_size*4;
        p0=p0&m;
        for(i=j; i<k; i++)
          {
          timf2_shi[p0]=timf2_tmp[i]*ampfac; 
          p0++;  
          }
        }
      else
        {
    // With an arbitrary window we multiply by the inverted window function
    // and take the center portion of each transform.
        ia=fft1_interleave_points/2;
        ib=fft1_size/2;
        kk=ia*4;
        for(i=ia; i<ib; i++)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>2;
          timf2_shi[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_int[pp]=timf2_shi[p0]*timf2_shi[p0];
          timf2_shi[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_int[pp]+=timf2_shi[p0+1]*timf2_shi[p0+1];
          timf2_shi[p0+2]=t1*timf2_tmp[kk+2];
          timf2_shi[p0+3]=t1*timf2_tmp[kk+3];
          pp++;
          p0=(p0+4)&m;  
          kk+=4;
          }
        for(i=ib; i>ia; i--)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>2;
          timf2_shi[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_int[pp]=timf2_shi[p0]*timf2_shi[p0];
          timf2_shi[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_int[pp]+=timf2_shi[p0+1]*timf2_shi[p0+1];
          timf2_shi[p0+2]=t1*timf2_tmp[kk+2];
          timf2_shi[p0+3]=t1*timf2_tmp[kk+3];
          pp++;
          p0=(p0+4)&m;  
          kk+=4;
          }
        }
      }
    }  
  else
    {  
    if(fft1_interleave_points==0)
      {
    // Without window we just transfer the points.
      for(i=0; i<siz; i++)
        {
        timf2_shi[p0  ]=ampfac*timf2_tmp[mm*i  ];
        timf2_pwr_int[pp]=(timf2_shi[p0]*timf2_shi[p0])>>1;
        timf2_shi[p0+1]=ampfac*timf2_tmp[mm*i+1];
        timf2_pwr_int[pp]+=(timf2_shi[p0+1]*timf2_shi[p0+1])>>1;
        timf2_shi[p0+2]=ampfac*timf2_tmp[mm*i+2];
        timf2_pwr_int[pp]+=(timf2_shi[p0+2]*timf2_shi[p0+2])>>1;
        timf2_shi[p0+3]=ampfac*timf2_tmp[mm*i+3];
        timf2_pwr_int[pp]+=(timf2_shi[p0+3]*timf2_shi[p0+3])>>1;
        timf2_shi[p0+4]=ampfac*timf2_tmp[mm*i+4];
        timf2_shi[p0+5]=ampfac*timf2_tmp[mm*i+5];
        timf2_shi[p0+6]=ampfac*timf2_tmp[mm*i+6];
        timf2_shi[p0+7]=ampfac*timf2_tmp[mm*i+7];
        pp++;
        p0+=8;
        }  
      }
    else
      {
      if(fft1_interleave_points==fft1_size/2)
        {
    // With a sin squared window we note that sin*sin+cos*cos=1 and add data.
        k=fft1_size/2;
        for(i=0; i<k; i++)
          {
          timf2_shi[p0  ]+=ampfac*timf2_tmp[mm*i  ];
          timf2_pwr_int[pp]=(timf2_shi[p0]*timf2_shi[p0])>>1;
          timf2_shi[p0+1]+=ampfac*timf2_tmp[mm*i+1];
          timf2_pwr_int[pp]+=(timf2_shi[p0+1]*timf2_shi[p0+1])>>1;
          timf2_shi[p0+2]+=ampfac*timf2_tmp[mm*i+2];
          timf2_pwr_int[pp]+=(timf2_shi[p0+2]*timf2_shi[p0+2])>>1;
          timf2_shi[p0+3]+=ampfac*timf2_tmp[mm*i+3];
          timf2_pwr_int[pp]+=(timf2_shi[p0+3]*timf2_shi[p0+3])>>1;
          timf2_shi[p0+4]+=ampfac*timf2_tmp[mm*i+4];
          timf2_shi[p0+5]+=ampfac*timf2_tmp[mm*i+5];
          timf2_shi[p0+6]+=ampfac*timf2_tmp[mm*i+6];
          timf2_shi[p0+7]+=ampfac*timf2_tmp[mm*i+7];
          pp++;
          p0+=8;
          }
        j=k*8;
        k=fft1_size*8;
        p0=p0&m;
        for(i=j; i<k; i++)
          {
          timf2_shi[p0]=timf2_tmp[i]*ampfac; 
          p0++;  
          }
        }
      else
        {
    // With an arbitrary window we multiply by the inverted window function
    // and take the center portion of each transform.
        ia=fft1_interleave_points/2;
        ib=fft1_size/2;
        kk=ia*8;
        for(i=ia; i<ib; i++)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>3;
          timf2_shi[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_int[pp]=(timf2_shi[p0]*timf2_shi[p0])>>1;
          timf2_shi[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_int[pp]+=(timf2_shi[p0+1]*timf2_shi[p0+1])>>1;
          timf2_shi[p0+2]=t1*timf2_tmp[kk+2];
          timf2_pwr_int[pp]+=(timf2_shi[p0+2]*timf2_shi[p0+2])>>1;
          timf2_shi[p0+3]=t1*timf2_tmp[kk+3];
          timf2_pwr_int[pp]+=(timf2_shi[p0+3]*timf2_shi[p0+3])>>1;
          timf2_shi[p0+4]=t1*timf2_tmp[kk+4];
          timf2_shi[p0+5]=t1*timf2_tmp[kk+5];
          timf2_shi[p0+6]=t1*timf2_tmp[kk+6];
          timf2_shi[p0+7]=t1*timf2_tmp[kk+7];
          pp++;
          p0=(p0+8)&m;  
          kk+=8;
          }
        for(i=ib; i>ia; i--)
          {
          t1=fft1_inverted_window[i]*ampfac;
          pp=p0>>3;
          timf2_shi[p0  ]=t1*timf2_tmp[kk  ];
          timf2_pwr_int[pp]=(timf2_shi[p0]*timf2_shi[p0])>>1;
          timf2_shi[p0+1]=t1*timf2_tmp[kk+1];
          timf2_pwr_int[pp]+=(timf2_shi[p0+1]*timf2_shi[p0+1])>>1;
          timf2_shi[p0+2]=t1*timf2_tmp[kk+2];
          timf2_pwr_int[pp]+=(timf2_shi[p0+2]*timf2_shi[p0+2])>>1;
          timf2_shi[p0+3]=t1*timf2_tmp[kk+3];
          timf2_pwr_int[pp]+=(timf2_shi[p0+3]*timf2_shi[p0+3])>>1;
          timf2_shi[p0+4]=t1*timf2_tmp[kk+4];
          timf2_shi[p0+5]=t1*timf2_tmp[kk+5];
          timf2_shi[p0+6]=t1*timf2_tmp[kk+6];
          timf2_shi[p0+7]=t1*timf2_tmp[kk+7];
          pp++;
          p0=(p0+8)&m;  
          kk+=8;
          }
        }
      }
    }
  }
}
