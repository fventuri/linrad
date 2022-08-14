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
#include "screendef.h"
#include "fft1def.h"
#include "blnkdef.h"
#include "fft2def.h"
#include "thrdef.h"

int blnk_pbeg, blnk_pend;

float subtract_onechan_pulse(int p_max, int sub_size)
{
int pa,pb,p0,mask;
int i, j, k, m, wid, imax;
float t1,t2,t3,t4,t5,t6;
float blanker_phase_c1,blanker_phase_c2;
int re_x_int, im_x_int;
float re_x_float, im_x_float;
float retval;
// **************************
// Max amplitude is at p_max in timf2 
// Use blanker_phasefunc to compensate for the phase
// rotation of the strongest part of the pulse.
k=refpul_size-2*blanker_pulsewidth;
mask=timf2_mask&-4;
pa=(4*(p_max-blanker_pulsewidth)+mask)&mask;
pb=(4*(p_max+blanker_pulsewidth) )&mask;
i=0;
if(swfloat)
  {
  while(pa != pb)
    {
    pa=(pa+4)&mask;
    t1=timf2_float[pa  ];
    t2=timf2_float[pa+1];
    t3=blanker_phasefunc[k  ];
    t4=blanker_phasefunc[k+1];
    t5=t1*t3+t2*t4;
    k+=2;
    t6=t2*t3-t1*t4;
    blanker_input[i  ]=t5;
    blanker_input[i+1]=t6;
    i+=2;
    }
  }  
else
  {
  while(pa != pb)
    {
    pa=(pa+4)&mask;
    t1=timf2_shi[pa  ];
    t2=timf2_shi[pa+1];
    t3=blanker_phasefunc[k  ];
    t4=blanker_phasefunc[k+1];
    t5=t1*t3+t2*t4;
    k+=2;
    t6=t2*t3-t1*t4;
    blanker_input[i  ]=t5;
    blanker_input[i+1]=t6;
    i+=2;
    }
  }  
// Collect the power averaged phase of the 3 center points.
imax=blanker_pulsewidth;
blanker_phase_c1=0;
blanker_phase_c2=0;
t4=0;
for(i=imax-1; i<=imax+1; i++)
  {
  t1=blanker_input[2*i  ];
  t2=blanker_input[2*i+1];
  t3=sqrt(t1*t1+t2*t2);
  blanker_phase_c1+=t3*t1;
  blanker_phase_c2+=t3*t2;
  }
t1=blanker_phase_c1*blanker_phase_c1+blanker_phase_c2*blanker_phase_c2;
// Return if the pulse is extremely small (should never happen)
if(t1 < 32)return -1;  
t1=sqrt(t1);
blanker_phase_c1/=t1;
blanker_phase_c2/=t1;
// Rotate the phase of the input pulse for zero average phase.
// Collect the I and Q powers 
wid=2*blanker_pulsewidth;
t3=0;
t4=0;
for(i=0; i<=wid; i++)
  {
  t1=blanker_input[2*i  ];
  t2=blanker_input[2*i+1];
  blanker_input[2*i  ]=blanker_phase_c1*t1+blanker_phase_c2*t2;
  blanker_input[2*i+1]=blanker_phase_c1*t2-blanker_phase_c2*t1;
  t3+=blanker_input[2*i  ]*blanker_input[2*i  ];
  t4+=blanker_input[2*i+1]*blanker_input[2*i+1];
  }
if(t4>0.25*t3)return -1.;
// Now imax points to the maximum value for the current pulse.
// Find the decimals of the position by a parabolic fit.
// Check blanker_init (buf.c) for details.
// Use decimals of position to get index to standard pulses
t4=blanker_input[2*imax-2]-blanker_input[2*imax+2];
t3=2*(blanker_input[2*imax-2]+blanker_input[2*imax+2]-2*blanker_input[2*imax]);
if(t3 == 0)return -2.;
t4/=t3;
if(t4<0)
  {
  t4=-sqrt(0.5)*sqrt(-t4);    
  }
else
  {
  t4=sqrt(0.5)*sqrt(t4);    
  }
j=MAX_REFPULSES*(t4+0.5)+0.5;
if(j<0)j=0;
if(j>=MAX_REFPULSES)j=MAX_REFPULSES-1;
m=2*blanker_pulindex[j]*refpul_size;
mask=timf2_mask&-4;
p0=(4*(p_max-sub_size/2)+mask)&mask;
pb=(4*(p_max+sub_size/2) )&mask;
k=refpul_size-sub_size;
// scale phase coefficients so we get the total pulse
// from a multiplication of two coefficients.
blanker_phase_c1*=blanker_input[2*imax]*liminfo_amplitude_factor;
blanker_phase_c2*=blanker_input[2*imax]*liminfo_amplitude_factor;
// Subtract the standard pulse and make a new power function.
// Collect new and old total power.
t3=0;
t4=0;
if(swfloat)
  {
  while(p0 != pb)
    {
    p0=(p0+4)&mask;
    t1=blanker_refpulse[m+k];
    t2=blanker_refpulse[m+k+1];
    re_x_float=timf2_float[p0  ]-blanker_phase_c1*t1+blanker_phase_c2*t2;
    im_x_float=timf2_float[p0+1]-blanker_phase_c1*t2-blanker_phase_c2*t1;
    timf2_float[p0  ]=re_x_float;
    timf2_float[p0+1]=im_x_float;
    t1=re_x_float*re_x_float+im_x_float*im_x_float;
    t3+=timf2_pwr_float[p0>>2];
    timf2_pwr_float[p0>>2]=t1;
    k+=2;
    t4+=t1;
    }
  }  
else
  {
  while(p0 != pb)
    {
    p0=(p0+4)&mask;
    t1=blanker_refpulse[m+k];
    t2=blanker_refpulse[m+k+1];
    re_x_int=timf2_shi[p0  ]-blanker_phase_c1*t1+blanker_phase_c2*t2;
    im_x_int=timf2_shi[p0+1]-blanker_phase_c1*t2-blanker_phase_c2*t1;
    timf2_shi[p0  ]=re_x_int;
    timf2_shi[p0+1]=im_x_int;
    j=re_x_int*re_x_int+im_x_int*im_x_int;
    t3+=timf2_pwr_int[p0>>2];
    timf2_pwr_int[p0>>2]=j;
    k+=2;
    t4+=j;
    }
  }  
retval=t4/t3;
if(retval > 0.5)
  {
// Pulse subtraction was not sucessful. 
// Restore original data and report failure.
  p0=(4*(p_max-sub_size/2)+mask)&mask;
  k=refpul_size-sub_size;
  if(swfloat)
    {
    while(p0 != pb)
      {
      p0=(p0+4)&mask;
      t1=blanker_refpulse[m+k];
      t2=blanker_refpulse[m+k+1];
      re_x_float=timf2_float[p0  ]+blanker_phase_c1*t1+blanker_phase_c2*t2;
      im_x_float=timf2_float[p0+1]+blanker_phase_c1*t2-blanker_phase_c2*t1;
      timf2_float[p0  ]=re_x_float;
      timf2_float[p0+1]=im_x_float;
      timf2_pwr_float[p0>>2]=re_x_float*re_x_float+im_x_float*im_x_float;
      k+=2;
      }
    }  
  else
    {
    while(p0 != pb)
      {
      p0=(p0+4)&mask;
      t1=blanker_refpulse[m+k];
      t2=blanker_refpulse[m+k+1];
      re_x_int=timf2_shi[p0  ]+blanker_phase_c1*t1+blanker_phase_c2*t2;
      im_x_int=timf2_shi[p0+1]+blanker_phase_c1*t2-blanker_phase_c2*t1;
      timf2_shi[p0  ]=re_x_int;
      timf2_shi[p0+1]=im_x_int;
      timf2_pwr_int[p0>>2]=re_x_int*re_x_int+im_x_int*im_x_int;
      k+=2;
      }
    }  
  return -5;
  }
return retval;
}

float subtract_twochan_pulse(int p_max, int sub_size)
{
int pb,p0,mask;
int i, j, k, m, wid, imax;
float pw;
float t1,t2,t3,t4,t5,t6;
float blanker_phase_c1,blanker_phase_c2,c1,c2;
float re_x,im_x,re_y,im_y,re_a,im_a;
float retval;
// **************************
// Max amplitude is at p_max in timf2 which corresponds to 
// blanker_pulsewidth/2 in blanker_input.
// Use blanker_phasefunc to compensate for the phase
// rotation of the strongest part of the pulse
// and collect the average phase.
k=refpul_size-2*blanker_pulsewidth;
wid=2*blanker_pulsewidth;
for(i=0; i<=wid; i++)
  {
  t1=blanker_input[2*i  ];
  t2=blanker_input[2*i+1];
  t3=blanker_phasefunc[k  ];
  t4=blanker_phasefunc[k+1];
  t5=t1*t3+t2*t4;
  t6=t2*t3-t1*t4;
  blanker_input[2*i  ]=t5;
  blanker_input[2*i+1]=t6;
  k+=2;
  }
// Collect the power averaged phase of the 3 center points.
imax=blanker_pulsewidth;
blanker_phase_c1=0;
blanker_phase_c2=0;
t4=0;
for(i=imax-1; i<=imax+1; i++)
  {
  t1=blanker_input[2*i  ];
  t2=blanker_input[2*i+1];
  t3=sqrt(t1*t1+t2*t2);
  blanker_phase_c1+=t3*t1;
  blanker_phase_c2+=t3*t2;
  }
t1=sqrt(blanker_phase_c1*blanker_phase_c1+blanker_phase_c2*blanker_phase_c2);
if(t1 < 4)return -1;
blanker_phase_c1/=t1;
blanker_phase_c2/=t1;
// Rotate the phase of the input pulse for zero average phase.
// Collect the I and Q powers 
t3=0;
t4=0;
for(i=0; i<=wid; i++)
  {
  t1=blanker_input[2*i  ];
  t2=blanker_input[2*i+1];
  blanker_input[2*i  ]=blanker_phase_c1*t1+blanker_phase_c2*t2;
  blanker_input[2*i+1]=blanker_phase_c1*t2-blanker_phase_c2*t1;
  t3+=blanker_input[2*i  ]*blanker_input[2*i  ];
  t4+=blanker_input[2*i+1]*blanker_input[2*i+1];
  }
if(t4>0.25*t3)return -1.;
// Now imax points to the maximum value for the current pulse.
// Find the decimals of the position by a parabolic fit.
// Check blanker_init (buf.c) for details (pulses are stored
// in order to be fitted this way).
// Use decimals of position to get index to standard pulses
t4=blanker_input[2*imax-2]-blanker_input[2*imax+2];
t3=2*(blanker_input[2*imax-2]+blanker_input[2*imax+2]-2*blanker_input[2*imax]);
if(t3 == 0)return -2.;
t4/=2*t3;
if(t4<0)
  {
  t4=-sqrt(-t4);    
  }
else
  {
  t4=sqrt(t4);    
  }
j=MAX_REFPULSES*(t4+0.5)+0.5;
if(j<0)j=0;
if(j>=MAX_REFPULSES)j=MAX_REFPULSES-1;
m=2*blanker_pulindex[j]*refpul_size;
mask=timf2_mask&-8;
p0=(8*(p_max-sub_size/2)+mask)&mask;
pb=(8*(p_max+sub_size/2) )&mask;
k=refpul_size-sub_size;
// scale phase coefficients so we get the total pulse
// from a multiplication of two coefficients.
// liminfo_amplitude_factor corrects for the fraction of the total
// pulse energy that is lost among the strong signals.


c1=blanker_phase_c1*blanker_input[2*imax]*liminfo_amplitude_factor;
c2=blanker_phase_c2*blanker_input[2*imax]*liminfo_amplitude_factor;
t3=0;
t4=0;
if(swfloat)
  {
  while(p0 != pb)
    {
    p0=(p0+8)&mask;
    t1=blanker_refpulse[m+k];
    t2=blanker_refpulse[m+k+1];
    re_a=c1*t1-c2*t2;
    im_a=c1*t2+c2*t1;
    re_x=timf2_float[p0  ]-blanker_pol_c1*re_a;
    im_x=timf2_float[p0+1]-blanker_pol_c1*im_a;
    re_y=timf2_float[p0+2]-(blanker_pol_c2*re_a+blanker_pol_c3*im_a);
    im_y=timf2_float[p0+3]-(blanker_pol_c2*im_a-blanker_pol_c3*re_a);
    timf2_float[p0  ]=re_x;
    timf2_float[p0+1]=im_x;
    timf2_float[p0+2]=re_y;
    timf2_float[p0+3]=im_y;

    pw=re_x*re_x+im_x*im_x+re_y*re_y+im_y*im_y;
    t3+=timf2_pwr_float[p0>>3];
    timf2_pwr_float[p0>>3]=pw;
    k+=2;
    t4+=pw;
    }
  }  
else
  {
  while(p0 != pb)
    {
    p0=(p0+8)&mask;
    t1=blanker_refpulse[m+k];
    t2=blanker_refpulse[m+k+1];
    re_a=c1*t1-c2*t2;
    im_a=c1*t2+c2*t1;
    re_x=timf2_shi[p0  ]-blanker_pol_c1*re_a;
    im_x=timf2_shi[p0+1]-blanker_pol_c1*im_a;
    re_y=timf2_shi[p0+2]-(blanker_pol_c2*re_a+blanker_pol_c3*im_a);
    im_y=timf2_shi[p0+3]-(blanker_pol_c2*im_a-blanker_pol_c3*re_a);
    timf2_shi[p0  ]=re_x;
    timf2_shi[p0+1]=im_x;
    timf2_shi[p0+2]=re_y;
    timf2_shi[p0+3]=im_y;
    pw=0.5*(re_x*re_x+im_x*im_x+re_y*re_y+im_y*im_y);
    t3+=timf2_pwr_int[p0>>3];
    timf2_pwr_int[p0>>3]=pw;
    k+=2;
    t4+=pw;
    }
  }  
retval=t4/t3;
if(retval > 0.5)
  {
// Pulse subtraction was not sucessful. 
// Restore original data and report failure.
  p0=(8*(p_max-sub_size/2)+mask)&mask;
  k=refpul_size-sub_size;
  if(swfloat)
    {
    while(p0 != pb)
      {
      p0=(p0+8)&mask;
      t1=blanker_refpulse[m+k];
      t2=blanker_refpulse[m+k+1];
      re_a=c1*t1-c2*t2;
      im_a=c1*t2+c2*t1;
      re_x=timf2_float[p0  ]+blanker_pol_c1*re_a;
      im_x=timf2_float[p0+1]+blanker_pol_c1*im_a;
      re_y=timf2_float[p0+2]+(blanker_pol_c2*re_a+blanker_pol_c3*im_a);
      im_y=timf2_float[p0+3]+(blanker_pol_c2*im_a-blanker_pol_c3*re_a);
      timf2_float[p0  ]=re_x;
      timf2_float[p0+1]=im_x;
      timf2_float[p0+2]=re_y;
      timf2_float[p0+3]=im_y;
      pw=re_x*re_x+im_x*im_x+re_y*re_y+im_y*im_y;
      timf2_pwr_float[p0>>3]=pw;
      k+=2;
      }
    }  
  else
    {
    while(p0 != pb)
      {
      p0=(p0+8)&mask;
      t1=blanker_refpulse[m+k];
      t2=blanker_refpulse[m+k+1];
      re_a=blanker_phase_c1*t1-blanker_phase_c2*t2;
      im_a=blanker_phase_c1*t2+blanker_phase_c2*t1;
      re_x=timf2_shi[p0  ]+blanker_pol_c1*re_a;
      im_x=timf2_shi[p0+1]+blanker_pol_c1*im_a;
      re_y=timf2_shi[p0+2]+(blanker_pol_c2*re_a+blanker_pol_c3*im_a);
      im_y=timf2_shi[p0+3]+(blanker_pol_c2*im_a-blanker_pol_c3*re_a);
      timf2_shi[p0  ]=re_x;
      timf2_shi[p0+1]=im_x;
      timf2_shi[p0+2]=re_y;
      timf2_shi[p0+3]=im_y;
      pw=0.5*(re_x*re_x+im_x*im_x+re_y*re_y+im_y*im_y);
      timf2_pwr_int[p0>>3]=pw;
      k+=2;
      }
    }  
  return -5;
  }
return retval;
}


int get_pulse_pol(int p_max)
{
int pa,pb,mask;
float t1,t2,re_x,im_x,re_y,im_y,x2,y2,sina,noi2,x2s,y2s;
float re_xy,im_xy;
// *********************************
//Assume that everything in timf2 (weak signal part) is interference
//from a common origin.
//We have 2 complex arrays X and Y.
//(  /X  is the complex conjugate of X  )
//Find the total signals, X2=(X * /X) and (Y * /Y).
//Get the total sum of squares, norm=X2+Y2
//Get the normalised signals squared x2 = (X * /X) / norm
//                               and y2 = (Y * /Y) / norm
//Get the normalised 'overlap'       xy = (X * /Y) / norm
// ******************************
mask=timf2_mask&-8;
pa=(8*(p_max-blanker_pulsewidth)+mask)&mask;
pb=(8*(p_max+blanker_pulsewidth) )&mask;
x2=0;
y2=0;
re_xy=0;
im_xy=0;
if(swfloat)
  {
  while(pa != pb)
    {
    pa=(pa+8)&mask;
    re_x=timf2_float[pa  ];
    im_x=timf2_float[pa+1];
    re_y=timf2_float[pa+2];
    im_y=timf2_float[pa+3];
    x2+=re_x*re_x+im_x*im_x;
    y2+=re_y*re_y+im_y*im_y;
    re_xy+=re_x*re_y+im_x*im_y;
    im_xy+=im_x*re_y-re_x*im_y;
    }
  }  
else
  {
  while(pa != pb)
    {
    pa=(pa+8)&mask;
    re_x=timf2_shi[pa  ];
    im_x=timf2_shi[pa+1];
    re_y=timf2_shi[pa+2];
    im_y=timf2_shi[pa+3];
    x2+=re_x*re_x+im_x*im_x;
    y2+=re_y*re_y+im_y*im_y;
    re_xy+=re_x*re_y+im_x*im_y;
    im_xy+=im_x*re_y-re_x*im_y;
    }
  }  
t1=x2+y2;
x2/=t1;
y2/=t1;
re_xy/=t1;
im_xy/=t1;
// *****************************************
//Now we have x2,y2 (real values) and xy (complex).
//For explanation purposes, assume im_xy == 0, which corresponds to linear
//polarization. The signal vill then be polarised in a plane.
//a = angle between polarization plane and the horisontal antenna.
//Assume that the noise level n is the same in the two antennas, and that
//the noise is uncorrelated.
//We then find:
//            x2 = cos(a)**2 + n**2
//            y2 = sin(a)**2 + n**2
//            xy = sin(a)*cos(a)
//From this we find: x2 * y2 - xy*xy = n**2 + n**4
//Neglect n**4:
//cos(a)=sqr( x2 - ( x2 * y2 - xy*xy) )
//sin(a)=sqr( y2 - ( x2 * y2 - xy*xy) )
//The transformation formula to use for rotating the polarization
//plane to produce new signals A and B, where A has all the signal and B
//only noise, will then be:
// A = X * cos(a) + Y * sin(a)
// B = Y * cos(a) - X * sin(a)
//Extending to im_xy != 0 the transformation becomes
//re_A=C1*re_X+C2*re_Y-C3*im_Y
//im_A=C1*im_X+C2*im_Y+C3*re_Y
//re_B=C1*re_Y-C2*re_X-C3*im_X
//im_B=C1*im_Y-C2*im_X+C3*re_X
//C1 = cos(a)
//C2 = sin(a) * re_xy / sqr( re_xy**2 + im_xy**2)
//C3 = sin(a) * im_xy / sqr( re_xy**2 + im_xy**2)
// **************************************
t2=re_xy*re_xy+im_xy*im_xy;
noi2=x2*y2-t2;
if(noi2 > 0.15)return -1;
x2s=x2-noi2;
y2s=y2-noi2;
if(x2s > 0)
  {
  blanker_pol_c1=sqrt(x2s);
  if(y2s > 0 && t2 > 0)
    {
    sina=sqrt(y2s);
    blanker_pol_c2=sina*re_xy/sqrt(t2);
    blanker_pol_c3=sina*im_xy/sqrt(t2);
    t1=sqrt(blanker_pol_c1*blanker_pol_c1+
            blanker_pol_c2*blanker_pol_c2+
            blanker_pol_c3*blanker_pol_c3);
    blanker_pol_c1/=t1;
    blanker_pol_c2/=t1;
    blanker_pol_c3/=t1;
    }
  else
    {
    if(x2 > y2)
      {
      blanker_pol_c1=1;
      blanker_pol_c2=0;
      }
    else
      {
      blanker_pol_c1=0;
      blanker_pol_c2=1;
      }
    blanker_pol_c3=0;
    }
  }
else
  {
  blanker_pol_c1=0;
  blanker_pol_c2=1;
  blanker_pol_c3=0;
  }
return 1;
}


void transform_timf2_pol(int p_max) 
{
int i,pa,pb,mask;
float re_x,im_x,re_y,im_y;
mask=timf2_mask&-8;
pa=(8*(p_max-blanker_pulsewidth)+mask)&mask;
pb=(8*(p_max+blanker_pulsewidth) )&mask;
i=0;
if(swfloat)
  {
  while(pa != pb)
    {
    pa=(pa+8)&mask;
    re_x=timf2_float[pa  ];
    im_x=timf2_float[pa+1];
    re_y=timf2_float[pa+2];
    im_y=timf2_float[pa+3];
    blanker_input[i  ]= blanker_pol_c1*re_x
                       +blanker_pol_c2*re_y
                       -blanker_pol_c3*im_y;
    blanker_input[i+1]= blanker_pol_c1*im_x
                       +blanker_pol_c2*im_y
                       +blanker_pol_c3*re_y;
    i+=2;
    }
  }
else
  {
  while(pa != pb)
    {
    pa=(pa+8)&mask;
    re_x=timf2_shi[pa  ];
    im_x=timf2_shi[pa+1];
    re_y=timf2_shi[pa+2];
    im_y=timf2_shi[pa+3];
    blanker_input[i  ]= blanker_pol_c1*re_x
                       +blanker_pol_c2*re_y
                       -blanker_pol_c3*im_y;
    blanker_input[i+1]= blanker_pol_c1*im_x
                       +blanker_pol_c2*im_y
                       +blanker_pol_c3*re_y;
    i+=2;
    }
  }
}

void set_flag(char value, int p_max)
{
int i, pa, pb, p0;
int begin_flag, end_flag;
// This pulse can not be fitted.
// Set flag to "value" over +/- blanker pulsewidth and for as 
// long as power decreases.
blanker_flag[p_max]=value;
pa=p_max;
pb=p_max;
for(i=0; i<blanker_pulsewidth; i++)
  {
  pb=(pb+timf2pow_mask)&timf2pow_mask;
  pa=(pa+1)&timf2pow_mask;
  blanker_flag[pa]=value;
  blanker_flag[pb]=value;
  }
p0=pb;
pb=(pb+timf2pow_mask)&timf2pow_mask;
begin_flag=( ((pb-blnk_pbeg+timf2pow_mask)&timf2pow_mask) > timf2pow_mask/2);
if(!begin_flag)
  {
  if(swfloat)
    {
    while( timf2_pwr_float[pb] < timf2_pwr_float[p0] && pb!=blnk_pbeg)
      {
      blanker_flag[pb]=value;
      p0=pb;
      pb=(pb+timf2pow_mask)&timf2pow_mask;
      }
    }       
  else
    {
    while( timf2_pwr_int[pb] < timf2_pwr_int[p0] && pb!=blnk_pbeg)
      {
      blanker_flag[pb]=value;
      p0=pb;
      pb=(pb+timf2pow_mask)&timf2pow_mask;
      }
    }
  }         
p0=pa;
pa=(pa+1)&timf2pow_mask;
end_flag=( ((blnk_pend-pa+timf2pow_mask)&timf2pow_mask) > timf2pow_mask/2);
if(!end_flag)
  {
  if(swfloat)
    {
    while( timf2_pwr_float[pa] < timf2_pwr_float[p0] && pa!=blnk_pend)
      {
      p0=pa;
      blanker_flag[pa]=value;
      pa=(pa+1)&timf2pow_mask;
      }
    }
  else
    {
    while( timf2_pwr_int[pa] < timf2_pwr_int[p0] && pa!=blnk_pend)
      {
      p0=pa;
      blanker_flag[pa]=value;
      pa=(pa+1)&timf2pow_mask;
      }
    }
  }
}
// ****************************************************************


// *******************************************************************



void first_noise_blanker(void)
{
int i, j, k, m, ifirst;
int stupid_clr1, stupid_clr2, mm;
int bln_no, p0, pa, pb, pf, pk;
int cleared_points, total_points; 
int fitted_pulses, p_max;
float t1,t2,t3,t4;
float avgpwr[BLN_INFO_SIZE];
unsigned int nfl;
float totnoise;
float sizlim;
unsigned int powermax_uint, pwrlim_uint;
float powermax_float, pwrlim_float;
int timf2_findmax;
unsigned int pulmax;
float pulmax_float;
// timf2_pa is the latest point in timf2 containing valid data. 
// timf2p_fit is the latest point for which the blanker has been 
// run already.
mm=4*ui.rx_rf_channels; 
blnk_pbeg=timf2p_fit;
pf=blnk_pbeg;
// Leave blnfit_range+1 points for next time. (step size=mm=4*ui.rx_rf_channels)  
blnk_pend=(timf2_pa/mm-blnfit_range+timf2pow_mask)&timf2pow_mask;
blnk_pend&=0xfffffffc;
total_points=(blnk_pend-blnk_pbeg+1+timf2pow_mask)&timf2pow_mask;
// Do not run this routine more often than 50 times per second.
if(total_points < min_delay_time*ui.rx_ad_speed)
  {
  return;
  }
if(swmmx_fft2 && ampinfo_flag != 0)
  {
  p0=blnk_pbeg;
  if(sw_onechan)
    {
    while(p0 != blnk_pend )
      {
      p0=(p0+1)&timf2pow_mask;
      k=p0<<2;
      i=abs(timf2_shi[k  ]);
      if(timf2_maxamp[0]<i)timf2_maxamp[0]=i;
      i=abs(timf2_shi[k+1]);
      if(timf2_maxamp[0]<i)timf2_maxamp[0]=i;
      i=abs(timf2_shi[k+2]);
      if(timf2_maxamp[2]<i)timf2_maxamp[2]=i;
      i=abs(timf2_shi[k+3]);
      if(timf2_maxamp[2]<i)timf2_maxamp[2]=i;
      }
    }  
  else
    {
    while(p0 != blnk_pend )
      {
      p0=(p0+1)&timf2pow_mask;
      k=p0<<3;
      i=abs(timf2_shi[k  ]);
      if(timf2_maxamp[0]<i)timf2_maxamp[0]=i;
      i=abs(timf2_shi[k+1]);
      if(timf2_maxamp[0]<i)timf2_maxamp[0]=i;
      i=abs(timf2_shi[k+2]);
      if(timf2_maxamp[1]<i)timf2_maxamp[1]=i;
      i=abs(timf2_shi[k+3]);
      if(timf2_maxamp[1]<i)timf2_maxamp[1]=i;
      i=abs(timf2_shi[k+4]);
      if(timf2_maxamp[2]<i)timf2_maxamp[2]=i;
      i=abs(timf2_shi[k+5]);
      if(timf2_maxamp[2]<i)timf2_maxamp[2]=i;
      i=abs(timf2_shi[k+6]);
      if(timf2_maxamp[3]<i)timf2_maxamp[3]=i;
      i=abs(timf2_shi[k+7]);
      if(timf2_maxamp[3]<i)timf2_maxamp[3]=i;
      }
    }
  }
fitted_pulses=0;
cleared_points=0;
timf2_findmax=0;
if(hg.clever_bln_mode == 0)
  {
  goto skip_fit;
  }
// Clear blanker_flag. This is a cyclic buffer char variable
p0=blnk_pbeg;
blanker_flag[p0]=0;
while(p0 != blnk_pend)
  {
  p0=(p0+1)&timf2pow_mask;
  blanker_flag[p0]=0;
  }     
// Set the limit well below the noise floor in case the noise floor
// is raised because we used too few points before.
nfl=hg.clever_bln_limit;
sizlim=0.1*timf2_noise_floor;
fitted_pulses=0;
find_pulse:;
// Step until we find power above the limit.
if(swfloat)
  {  
  while( (timf2_pwr_float[pf] <= nfl || blanker_flag[pf]>64)
                                        && pf != blnk_pend)pf=(pf+1)&timf2pow_mask;
  }
else
  {  
  while( (timf2_pwr_int[pf] <= nfl || blanker_flag[pf]>64)
                                         && pf != blnk_pend)pf=(pf+1)&timf2pow_mask;
  }
if(pf == blnk_pend)goto clever_x;
p0=pf-1;
p_max=pf;
// Anything below 10 in summed power is irrelevant.
powermax_uint=10;
powermax_float=10;
m=blnfit_range;
// ****************************************************
// blanker_flag bit 0 is set each time a value is found
// that is larger than preceding points in current range.
// Values above 64 indicate point is already very much changed.
if(swfloat)
  {
  while(p0 != blnk_pend && m>0)
    {
    p0=(p0+1)&timf2pow_mask;
    if(timf2_pwr_float[p0] > powermax_float && blanker_flag[p0]<64)
      {
      blanker_flag[p0]=1;
      powermax_float=timf2_pwr_float[p0];
      p_max=p0;
      m=blnfit_range;
      }
    m--;
    }
  }  
else
  {
  while(p0 != blnk_pend && m>0)
    {
    p0=(p0+1)&timf2pow_mask;
    if(timf2_pwr_int[p0] > powermax_uint && blanker_flag[p0]<64)
      {
      blanker_flag[p0]=1;
      powermax_uint=timf2_pwr_int[p0];
      p_max=p0;
      m=blnfit_range;
      }
    m--;
    }
  }  
if(m>0)
  {
  goto clever_x;
  }  
// Verify that p_max is a maximum and not a point next to a region
// with flag above 64
p0=(p_max+timf2pow_mask)&timf2pow_mask;
if(blanker_flag[p0] >= 64)
  {
  pf=p_max;
no_pulse:;  
  if(swfloat)
    {  
    while( (timf2_pwr_float[pf] <= powermax_float || blanker_flag[pf]>64)
                                                   && pf != blnk_pend)
      {
      powermax_float=timf2_pwr_float[pf];
      pf=(pf+1)&timf2pow_mask;
      }
    }
  else
    {  
    while( (timf2_pwr_int[pf] <= powermax_uint || blanker_flag[pf]>64)
                                         && pf != blnk_pend)
      {
      powermax_uint=timf2_pwr_int[pf];
      pf=(pf+1)&timf2pow_mask;
      }
    }
  if(pf == blnk_pend)goto clever_x;
  goto find_pulse;
  }
p0=(p_max+1)&timf2pow_mask;
if(p0 == blnk_pend)goto clever_x;
if(blanker_flag[p0] >= 64)goto no_pulse;
if(timf2_findmax == 0)
  {
  if(swfloat)
    {
    if(timf2_oscilloscope_powermax_float < powermax_float)
      {
      timf2_oscilloscope_powermax_float=powermax_float;
      timf2_oscilloscope_maxpoint=p_max;
      }
    if(sw_onechan)
      {
      k=p_max<<2;
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k  ]))
                               timf2_oscilloscope_maxval_float=fabs(timf2_float[k  ]);
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+1]))
                               timf2_oscilloscope_maxval_float=fabs(timf2_float[k+1]);
      }
    else
      {
      k=p_max<<3;
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k  ]))
                              timf2_oscilloscope_maxval_float=fabs(timf2_float[k  ]);
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+1]))
                              timf2_oscilloscope_maxval_float=fabs(timf2_float[k+1]);
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+2]))
                              timf2_oscilloscope_maxval_float=fabs(timf2_float[k+2]);
      if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+3]))
                              timf2_oscilloscope_maxval_float=fabs(timf2_float[k+3]);
      }
    }
  else
    {
    if(timf2_oscilloscope_powermax_uint < powermax_uint)
      {
      timf2_oscilloscope_powermax_uint=powermax_uint;
      timf2_oscilloscope_maxpoint=p_max;
      }
    i=timf2_oscilloscope_maxval_uint;
    if(sw_onechan)
      {
      k=p_max<<2;
      if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
      if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
      }
    else
      {
      k=p_max<<3;
      if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
      if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
      if(i<abs(timf2_shi[k+2]))i=abs(timf2_shi[k+2]);
      if(i<abs(timf2_shi[k+3]))i=abs(timf2_shi[k+3]);
      }
    if(timf2_oscilloscope_maxval_uint<(unsigned int)(i))
      {
      timf2_oscilloscope_maxval_uint=i;
      }
    }
  }  
// We do not want to handle unresolved multiple pulses with
// this rather slow method. Check the average power over bln.size points
// for the widths that may be useful.
bln_no=0;
pa=(p_max+1)&timf2pow_mask;
pb=(p_max+timf2pow_mask)&timf2pow_mask;
k=2;
check_avgpwr:;
if(swfloat)
  {
  powermax_float=timf2_pwr_float[p_max]; 
  t1=powermax_float*bln[bln_no].rest;
  }
else
  {
  powermax_uint=timf2_pwr_int[p_max]; 
  t1=powermax_uint*bln[bln_no].rest;
  }
if(t1 < sizlim)
  {
  goto get_fit_size;
  }
t1=0;
if(swfloat)
  {
  while(k < bln[bln_no].size)
    {
    t1+=timf2_pwr_float[pa]+timf2_pwr_float[pb]; 
    pa=(pa+1)&timf2pow_mask;
    pb=(pb+timf2pow_mask)&timf2pow_mask;
    k+=2;
    }
  avgpwr[bln_no]=t1/powermax_float;  
  }
else
  {
  while(k < bln[bln_no].size)
    {
    t1+=(float)(timf2_pwr_int[pa])+(float)(timf2_pwr_int[pb]); 
    pa=(pa+1)&timf2pow_mask;
    pb=(pb+timf2pow_mask)&timf2pow_mask;
    k+=2;
    }
  avgpwr[bln_no]=t1/powermax_uint;  
  }
bln_no++;
if(bln_no <=largest_blnfit)goto check_avgpwr; 
get_fit_size:;
bln_no--;
// Remember how strong the pulse is by setting sizeflag.
// Skip the use of large fit sizes in case the pulse is ugly.
while( bln_no>=0 && avgpwr[bln_no] > bln[bln_no].avgmax ) bln_no--;
if(bln_no < 0)
  {
bad_pulse:;
  set_flag(65,p_max);
  goto find_pulse;
  }
if(ui.rx_rf_channels == 2)
  {
  t1=get_pulse_pol(p_max);
  if(t1 >=0)
    {
    transform_timf2_pol(p_max); 
    t1=subtract_twochan_pulse(p_max,bln[bln_no].size);
    }  
  }
else
  {
  t1=subtract_onechan_pulse(p_max, bln[bln_no].size);
  }
if(t1 < 0)goto bad_pulse;
fitted_pulses++;
  set_flag(66,p_max);
  goto find_pulse;
clever_x:;
if(fitted_pulses > 0)timf2_findmax=1;
skip_fit:;
if(hg.stupid_bln_mode == 0)
  {
  goto skip_stupid;
  }
nfl=hg.stupid_bln_limit;
p0=blnk_pbeg;
i=timf2_oscilloscope_maxval_uint;
t1=timf2_oscilloscope_maxval_float;
ifirst=0;
stupid_clr1=(blanker_pulsewidth+1)>>1;
stupid_clr2=blanker_pulsewidth+1;
pulmax=0;
pulmax_float=0;
totnoise=timf2_noise_floor*ui.rx_rf_channels;
pk=p0;
if(ui.rx_rf_channels==1)
  {
  if(swfloat)
    {
    while(p0!=blnk_pend)
      {
      p0=(p0+1)&timf2pow_mask;
      if(timf2_pwr_float[p0]>nfl)
        {
        if(ifirst==0)pk=p0;
        if(timf2_pwr_float[p0]>pulmax_float)pulmax_float=timf2_pwr_float[p0];
        ifirst++;
        k=p0<<2;
        if(timf2_findmax == 0)
          {
          if(timf2_oscilloscope_powermax_float < timf2_pwr_float[p0])
            {
            timf2_oscilloscope_powermax_float=timf2_pwr_float[p0];
            timf2_oscilloscope_maxpoint=p0;
            }
          if(t1<fabs(timf2_float[k  ]))t1=fabs(timf2_float[k  ]);
          if(t1<fabs(timf2_float[k+1]))t1=fabs(timf2_float[k+1]);
          }
        cleared_points++;
        timf2_pwr_float[p0]=0;
        timf2_float[k  ]=0;    
        timf2_float[k+1]=0;    
        }
      else
        {
        if(ifirst != 0)
          {
          ifirst=0;
          t1=pulmax_float/totnoise;
          pulmax_float=0;
          if(t1 > 4)
            {
// Set a limit at 40 dB.            
            if(t1 > 10000)t1=10000;
            t1=sqrt(t1)/100;
// Clear some points before the first cleared.
            pa=pk;
            i=stupid_clr1*t1+0.5;
            for(j=0; j<i; j++)
              {
              pa=(pa+timf2pow_mask)&timf2pow_mask;
              k=pa<<2;
              timf2_pwr_float[pa]=0;
              timf2_float[k  ]=0;    
              timf2_float[k+1]=0;    
              cleared_points++;
              }
// Clear some points after the last cleared.
            pa=p0;
            i=stupid_clr2*t1+0.5;
            for(j=0; j<i; j++)
              {
              k=pa<<2;
              timf2_pwr_float[pa]=0;
              timf2_float[k  ]=0;    
              timf2_float[k+1]=0;    
              pa=(pa+1)&timf2pow_mask;
              cleared_points++;
              }
            }
          }  
        }
      }
    }
  else
    {
    while(p0!=blnk_pend)
      {
      p0=(p0+1)&timf2pow_mask;
      if(timf2_pwr_int[p0]>nfl)
        {
        if(ifirst==0)pk=p0;
        if(timf2_pwr_int[p0]>pulmax)pulmax=timf2_pwr_int[p0];
        ifirst++;
        k=p0<<2;
        if(timf2_findmax == 0)
          {
          if(timf2_oscilloscope_powermax_uint < timf2_pwr_int[p0])
            {
            timf2_oscilloscope_powermax_uint=timf2_pwr_int[p0];
            timf2_oscilloscope_maxpoint=p0;
            }
          if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
          if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
          }
        cleared_points++;
        timf2_pwr_int[p0]=0;
        timf2_shi[k  ]=0;    
        timf2_shi[k+1]=0;    
        }
      else
        {
        if(ifirst != 0)
          {
          ifirst=0;
          t1=pulmax/totnoise;
          pulmax=0;
          if(t1 > 4)
            {
// Set a limit at 40 dB.            
            if(t1 > 10000)t1=10000;
            t1=sqrt(t1)/100;
// Clear some points before the first cleared.
            pa=pk;
            i=stupid_clr1*t1+0.5;
            for(j=0; j<i; j++)
              {
              pa=(pa+timf2pow_mask)&timf2pow_mask;
              k=pa<<2;
              timf2_pwr_int[pa]=0;
              timf2_shi[k  ]=0;    
              timf2_shi[k+1]=0;    
              cleared_points++;
              }
// Clear some points after the last cleared.
            pa=p0;
            i=stupid_clr2*t1+0.5;
            for(j=0; j<i; j++)
              {
              k=pa<<2;
              timf2_pwr_int[pa]=0;
              timf2_shi[k  ]=0;    
              timf2_shi[k+1]=0;    
              pa=(pa+1)&timf2pow_mask;
              cleared_points++;
              }
            }
          }
        }
      }
    }
  }
else
  {
  if(swfloat)
    {
    while(p0!=blnk_pend)
      {
      p0=(p0+1)&timf2pow_mask;
      if(timf2_pwr_float[p0]>nfl)
        {
        if(ifirst==0)pk=p0;
        if(timf2_pwr_float[p0]>pulmax_float)pulmax_float=timf2_pwr_float[p0];
        ifirst++;
        k=p0<<3;
        if(timf2_findmax == 0)
          {
          if(timf2_oscilloscope_powermax_float < timf2_pwr_float[p0])
            {
            timf2_oscilloscope_powermax_float=timf2_pwr_float[p0];
            timf2_oscilloscope_maxpoint=p0;
            }
          if(t1<fabs(timf2_float[k  ]))t1=fabs(timf2_float[k  ]);
          if(t1<fabs(timf2_float[k+1]))t1=fabs(timf2_float[k+1]);
          if(t1<fabs(timf2_float[k+2]))t1=fabs(timf2_float[k+2]);
          if(t1<fabs(timf2_float[k+3]))t1=fabs(timf2_float[k+3]);
          }
        cleared_points++;
        timf2_pwr_float[p0]=0;
        timf2_float[k  ]=0;    
        timf2_float[k+1]=0;    
        timf2_float[k+2]=0;    
        timf2_float[k+3]=0;    
        }
      else
        {
        if(ifirst != 0)
          {
          ifirst=0;
          t1=pulmax_float/totnoise;
          pulmax_float=0;
          if(t1 > 4)
            {
// Set a limit at 40 dB.            
            if(t1 > 10000)t1=10000;
            t1=sqrt(t1)/100;
// Clear some points before the first cleared.
            pa=pk;
            i=stupid_clr1*t1+0.5;
            for(j=0; j<i; j++)
              {
              pa=(pa+timf2pow_mask)&timf2pow_mask;
              k=pa<<3;
              timf2_pwr_float[pa]=0;
              timf2_float[k  ]=0;    
              timf2_float[k+1]=0;    
              timf2_float[k+2]=0;    
              timf2_float[k+3]=0;    
              cleared_points++;
              }
// Clear some points after the last cleared.
            pa=p0;
            i=stupid_clr2*t1+0.5;
            for(j=0; j<i; j++)
              {
              k=pa<<3;
              timf2_pwr_float[pa]=0;
              timf2_float[k  ]=0;    
              timf2_float[k+1]=0;    
              timf2_float[k+2]=0;    
              timf2_float[k+3]=0;    
              pa=(pa+1)&timf2pow_mask;
              cleared_points++;
              }
            }  
          }
        }
      }
    }
  else
    {
    while(p0!=blnk_pend)
      {
      p0=(p0+1)&timf2pow_mask;
      if(timf2_pwr_int[p0]>nfl)
        {
        if(ifirst==0)pk=p0;
        if(timf2_pwr_int[p0]>pulmax)pulmax=timf2_pwr_int[p0];
        ifirst++;
        k=p0<<3;
        if(timf2_findmax == 0)
          {
          if(timf2_oscilloscope_powermax_uint < timf2_pwr_int[p0])
            {
            timf2_oscilloscope_powermax_uint=timf2_pwr_int[p0];
            timf2_oscilloscope_maxpoint=p0;
            }
          if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
          if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
          if(i<abs(timf2_shi[k+2]))i=abs(timf2_shi[k+2]);
          if(i<abs(timf2_shi[k+3]))i=abs(timf2_shi[k+3]);
          }          
        cleared_points++;
        timf2_pwr_int[p0]=0;
        timf2_shi[k  ]=0;    
        timf2_shi[k+1]=0;    
        timf2_shi[k+2]=0;    
        timf2_shi[k+3]=0;    
        }
      else
        {
        if(ifirst != 0)
          {
          ifirst=0;
          t1=pulmax/totnoise;
          pulmax=0;
          if(t1 > 4)
            {
// Set a limit at 40 dB.            
            if(t1 > 10000)t1=10000;
            t1=sqrt(t1)/100;
// Clear some points before the first cleared.
            pa=pk;
            i=stupid_clr1*t1+0.5;
            for(j=0; j<i; j++)
              {
              pa=(pa+timf2pow_mask)&timf2pow_mask;
              k=pa<<3;
              timf2_pwr_int[pa]=0;
              timf2_shi[k  ]=0;    
              timf2_shi[k+1]=0;    
              timf2_shi[k+2]=0;    
              timf2_shi[k+3]=0;    
              cleared_points++;
              }
// Clear some points after the last cleared.
            pa=p0;
            i=stupid_clr2*t1+0.5;
            for(j=0; j<i; j++)
              {
              k=pa<<3;
              timf2_pwr_int[pa]=0;
              timf2_shi[k  ]=0;    
              timf2_shi[k+1]=0;    
              timf2_shi[k+2]=0;    
              timf2_shi[k+3]=0;    
              pa=(pa+1)&timf2pow_mask;
              cleared_points++;
              }
            }
          }
        }
      }
    }
  }
if(cleared_points > 0)
  {
  if(timf2_findmax == 0)
    {
    timf2_findmax =1;
    if(swfloat)
      {
      if(timf2_oscilloscope_maxval_float<t1)
        {
        timf2_oscilloscope_maxval_float=t1;
        }
      }  
    else
      {
      if(timf2_oscilloscope_maxval_uint<(unsigned int)(i))
        {
        timf2_oscilloscope_maxval_uint=i;
        }
      }  
    }
  }   



 
skip_stupid:;
// In case we did not remove any interference, check every fourth
// point for a maximum value.
// Note that pointers are multiples of four.
if(timf2_findmax == 0)
  {
  p0=blnk_pbeg;
  if(swfloat)
    {
    pwrlim_float=timf2_oscilloscope_powermax_float/3;
    while(p0 != blnk_pend )
      {
      p0=(p0+4)&timf2pow_mask;
      if(timf2_pwr_float[p0] > pwrlim_float)
        {
        if(sw_onechan)
          {
          k=p0<<2;
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k  ]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k  ]);
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+1]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k+1]);
          }
        else
          {
          k=p0<<3;
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k  ]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k  ]);
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+1]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k+1]);
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+2]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k+2]);
          if(timf2_oscilloscope_maxval_float<fabs(timf2_float[k+3]))
                            timf2_oscilloscope_maxval_float=fabs(timf2_float[k+3]);
          }
        if(timf2_oscilloscope_powermax_float < timf2_pwr_float[p0])
          {
          timf2_oscilloscope_powermax_float=timf2_pwr_float[p0];
          pwrlim_float=timf2_oscilloscope_powermax_float/3;
          timf2_oscilloscope_maxpoint=p0;
          }
        }
      }
    }
  else
    {
    pwrlim_uint=timf2_oscilloscope_powermax_uint/3;
    i=timf2_oscilloscope_maxval_uint;
    while(p0 != blnk_pend )
      {
      p0=(p0+4)&timf2pow_mask;
      if(timf2_pwr_int[p0] > pwrlim_uint)
        {
        if(sw_onechan)
          {
          k=p0<<2;
          if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
          if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
          }
        else
          {
          k=p0<<3;
          if(i<abs(timf2_shi[k  ]))i=abs(timf2_shi[k  ]);
          if(i<abs(timf2_shi[k+1]))i=abs(timf2_shi[k+1]);
          if(i<abs(timf2_shi[k+2]))i=abs(timf2_shi[k+2]);
          if(i<abs(timf2_shi[k+3]))i=abs(timf2_shi[k+3]);
          }
        if(timf2_oscilloscope_powermax_uint < timf2_pwr_int[p0])
          {
          timf2_oscilloscope_powermax_uint=timf2_pwr_int[p0];
          pwrlim_uint=timf2_oscilloscope_powermax_uint/3;
          timf2_oscilloscope_maxpoint=p0;
          }
        }
      }
    if(timf2_oscilloscope_maxval_uint<(unsigned int)(i))
      {
      timf2_oscilloscope_maxval_uint=i;
      }
    }
  }    
if(hg.timf2_oscilloscope != 0)
  {
  timf2_oscilloscope_counter++;
  if(timf2_oscilloscope_counter == 1 && timf2_show_pointer >=0 )
    {
    sc[SC_TIMF2_OSCILLOSCOPE]++;
    }
  if(timf2_oscilloscope_counter >= timf2_oscilloscope_interval
                              /(hg.timf2_oscilloscope*hg.timf2_oscilloscope))
    {
    i=0;
    if(swfloat)
      {
      if(timf2_oscilloscope_powermax_float > 0)
        {
        i=timf2_oscilloscope_maxpoint;
        }
      }  
    else
      {  
      if(timf2_oscilloscope_powermax_uint > 0)
        {
        i=timf2_oscilloscope_maxpoint;
        }
      }
    if(i != timf2_show_pointer)
      {
      if(i != -timf2_show_pointer)
        {
        timf2_show_pointer=i;
        }
      }
    else
      { 
      timf2_show_pointer =-i;
      }
    timf2_oscilloscope_powermax_uint=0;
    timf2_oscilloscope_counter=0;
    timf2_oscilloscope_maxval_uint=0;
    timf2_oscilloscope_powermax_float=0;
    timf2_oscilloscope_maxval_float=0;
    }        
  }
if(hg.clever_bln_mode != 0)
  {
  timf2p_fit=(pf-16+timf2pow_mask)&(timf2pow_mask&0xfffffffc);
  }
else
  {
  timf2p_fit=blnk_pend;
  }
timf2_pn2=mm*blnk_pend;
// Update timf2_noise_floor and timf2_despiked_pwr[]
// This is statistics and we do not have to use all the points.
// some time can be saved without significant loss of information.
// collect every 4th point. 
p0=blnk_pbeg;
m=((timf2p_fit-blnk_pbeg+1+timf2pow_mask)&timf2pow_mask);
timf2_fitted_pulses+=fitted_pulses;
timf2_cleared_points+=cleared_points;
timf2_blanker_points+=m;
if(timf2_blanker_points == 0)
  {
  return;
  }
// In case we cleared many points the number of points used for
// calculating average noise will be incorrect.
// Change k so it corresponds to the number of untouched points.
// but do not allow division by small numbers.
k=m-cleared_points-fitted_pulses;
if( k < m/25 )k=m/25;
k=(k+2)/4;
if(k<1)k=1;
t1=0;
if(sw_onechan)
  {
  if(swfloat)
    {
    while(p0 != timf2p_fit)
      {
      p0=(p0+4)&timf2pow_mask;
      t1+=timf2_pwr_float[p0];
      }
    }
  else
    {
    while(p0 != timf2p_fit)
      {
      p0=(p0+4)&timf2pow_mask;
      t1+=timf2_pwr_int[p0];
      }
    }
  t1/=k;
  if(t1<10)t1=10;
  timf2_despiked_pwrinc[0]+=t1;
  }
else
  {
  t2=0;
  if(swfloat)
    {
    while(p0 != timf2p_fit)
      {
      p0=(p0+4)&timf2pow_mask;
      pa=p0<<3;
      t3=timf2_float[pa  ];
      t4=timf2_float[pa+1];
      t1+=t3*t3+t4*t4;
      t3=timf2_float[pa+2];
      t4=timf2_float[pa+3];
      t2+=t3*t3+t4*t4;
      }
    }
  else
    {
    while(p0 != timf2p_fit)
      {
      p0=(p0+4)&timf2pow_mask;
      pa=p0<<3;
      t3=(float)(timf2_shi[pa  ]);
      t4=(float)(timf2_shi[pa+1]);
      t1+=t3*t3+t4*t4;
      t3=(float)(timf2_shi[pa+2]);
      t4=(float)(timf2_shi[pa+3]);
      t2+=t3*t3+t4*t4;
      }
    }
  t1/=k;
  t2/=k;
  if(t1<10)t1=10;
  if(t2<10)t2=10;
  timf2_despiked_pwrinc[0]+=t1;
  timf2_despiked_pwrinc[1]+=t2;
  t1+=t2;
  }
blanker_info_update_counter++;
if(blanker_info_update_counter>=blanker_info_update_interval)
  { 
  if(fft1_lowlevel_fraction < 0.1)
    {
    blanker_info_update_counter--;
    lir_pixwrite(hg.xleft+5*text_width,hg.ybottom-text_height-1,
                                         "SELLIM ERROR");
    }
  else
    {
  
    timf2_despiked_pwr[0]=timf2_despiked_pwrinc[0]/
                        (blanker_info_update_interval*fft1_lowlevel_fraction);
    timf2_despiked_pwr[1]=timf2_despiked_pwrinc[1]/
                        (blanker_info_update_interval*fft1_lowlevel_fraction);
    clever_blanker_rate=100.*(float)timf2_fitted_pulses/timf2_blanker_points;
    if(clever_blanker_rate > 99)clever_blanker_rate = 99;
    stupid_blanker_rate=100.*(float)timf2_cleared_points/timf2_blanker_points;
    if(stupid_blanker_rate > 99)stupid_blanker_rate = 99;
    timf2_noise_floor=(timf2_despiked_pwr[0]+timf2_despiked_pwr[1])/
                                                     ui.rx_rf_channels;
    if( hg.stupid_bln_mode == 1)
      {
      if(stupid_blanker_rate > 20)
        {
        if(timf2_noise_floor<30)timf2_noise_floor=30; 
        t1=0.01*pow(stupid_blanker_rate-20.0,2.);
        if(t1>10)t1=10;
        timf2_noise_floor*=1+t1;
        }
      else
        {  
        timf2_noise_floor=((timf2_noise_floor_avgnum-1)*timf2_noise_floor+t1)/
                                                   timf2_noise_floor_avgnum;
        }
      hg.stupid_bln_limit=timf2_noise_floor*hg.stupid_bln_factor;
      }
    if( hg.clever_bln_mode == 1)
      {
      hg.clever_bln_limit=timf2_noise_floor*hg.clever_bln_factor;
      }
    sc[SC_BLANKER_INFO]++;
    awake_screen();
    blanker_info_update_counter=0;
// Set start value to 1 to avoid problems with log function in hires_graph.
    timf2_despiked_pwrinc[0]=1;
    timf2_despiked_pwrinc[1]=1;
    timf2_fitted_pulses=0;
    timf2_cleared_points=0;
    timf2_blanker_points=0;
    }
  }
}
