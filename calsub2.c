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
#include <string.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "llsqdef.h"
#include "caldef.h"
#include "keyboard_def.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define IQ_FIT_MINFREQ 500
#define CORR_MINLEVEL 0.001
#define CORR_MAXLEVEL 20.
#define YSH 0.29
#define ERF_START -2.2
#define ERF_END 2.8

#define SYMFIT_ERROR -1
#define SYMFIT_ACCEPT -2
#define SYMFIT_CHANGE_LIMITS -3

void draw_fitline(void)
{
int i;
i=screen_width*(-1+4*(float)(cal_midlim)/fft1_size);
lir_line(i,screen_height*(cal_yzer+YSH),i,screen_height*(cal_yzer-YSH),2);
}


void make_symfit(int kk, int even, float *buf, float offset)
{
int i, k, m, mm;
float t1, t2, t3, fft1_hsize;
mm=twice_rxchan;
fft1_hsize=0.5*(fft1_size-1);
k=0;
for(i=cal_lowedge; i<cal_midlim; i++)
  {
  t1=cal_buf5[i];
  t2=1-i/fft1_hsize;
  if(kk == 1)llsq_derivatives[k]=t1;
  if(even == 1)
    {
    t2=t2*t2;
    t3=t1*t2;
    }
  else
    {
    t3=t1*t2;
    t2=t2*t2;
    }
  for(m=kk; m<llsq_npar; m++)
    {
    llsq_derivatives[m*llsq_neq+k]=t3;
    t3*=t2;
    }    
  llsq_errors[2*k]=t1*(buf[mm*i  ]-offset);
  if(ui.rx_rf_channels == 2)
    {
    llsq_errors[2*k+1]=t1*(buf[mm*i+2]-offset);
    }
  else
    {
    llsq_errors[2*k+1]=0;
    }
  k++;
  }
if(llsq2() != 0)
  {
  lirerr(1050);
  return;
  }
// Store the fitted curve in cal_buf
for(i=0; i<=fft1_size/2; i++)
  {
  t2=1-i/fft1_hsize;
  cal_buf[mm*i]=offset+kk*llsq_steps[0];
  if(ui.rx_rf_channels == 2)cal_buf[mm*i+2]=offset+kk*llsq_steps[1];
  if(even == 1)
    {
    t2=t2*t2;
    t3=t2;
    }
  else  
    {
    t3=t2;
    t2=t2*t2;
    }
  for(m=kk; m<llsq_npar; m++)
    {
    cal_buf[mm*i  ]+=llsq_steps[2*m]*t3;
    if(ui.rx_rf_channels == 2)
      {
      cal_buf[mm*i+2]+=llsq_steps[2*m+1]*t3;
      }
    t3*=t2;
    }
  }
}

int symmetry_fit_decide(float *buf, float offset, char *txt)  
{
char s[80];
float t2;
int i,j,k,mm;
float center_region_ia;
int center_region_pix;
center_region_ia=0.4;
symfit_begin:;
cal_type=CAL_TYPE_FIX_CENTER_SYMFIT;
cal_initscreen();
mm=twice_rxchan;
lir_text(0,1,"Curve fitting to response of direct conversion receiver.");
lir_text(0,2,txt);
for(j=0; j<ui.rx_rf_channels; j++)
  {
  sprintf(s,"Ch%d",j);
  lir_text(0,j+3,s);
  for(k=0; k<llsq_npar; k++)
    {
    if(llsq_steps[j+2*k] < 1000)
      {
      sprintf(s,"%.4f",llsq_steps[j+2*k]);
      }                    
    else
      {
      sprintf(s,"%.0f",llsq_steps[j+2*k]);
      }                    
    lir_text(4+11*k,j+3,s);
    }
  }
lir_text(2,6, " +- => Change Y-scale");
lir_text(2,7, "  0 => Use raw data without curve fit");
lir_text(2,8, "1-9 => Fit with new number of terms"); 
lir_text(2,9, "  C => Enter a larger number than 9 and fit"); 
lir_text(50,6,"  A => Use curve fitted with current no of terms"); 
lir_text(50,8,"  B => Back, change freq limits");
lir_text(50,9,"L,R => Border left, right");
settextcolor(14);
lir_text(0,10,"Raw data.");
settextcolor(12);
lir_text(16,10,"Difference raw to fitted.");
settextcolor(7);
redraw:;
center_region_pix=screen_width*(-1+4*center_region_ia);
lir_line(center_region_pix,screen_height*(cal_yzer+YSH),
        center_region_pix,screen_height*(cal_yzer-YSH),1);
if(kill_all_flag) return SYMFIT_ERROR;
draw_fitline();
if(kill_all_flag) return SYMFIT_ERROR;
sprintf(s,"Y-gain = %f     No of terms = %d    ",cal_ygain,llsq_npar);
lir_text(0,5,s);  
lir_hline(0,screen_height*cal_yzer,screen_last_xpixel,1);
if(kill_all_flag) return SYMFIT_ERROR;
lir_hline(0,screen_height*(cal_yzer+YSH),screen_last_xpixel,1);
if(kill_all_flag) return SYMFIT_ERROR;
for(i=0; i<screen_width; i++)
  {
  k=i*fft1_size/(4*screen_width)+fft1_size/4;
  for(j=0; j<mm; j+=2)
    {
    lir_setpixel(i, cal_graph[screen_width*(2*j  )+i], 0);  
    t2=0.1*cal_ygain*(buf[mm*k+j]-offset);
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 0)t2-=YSH;
    cal_graph[screen_width*(2*j  )+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, cal_graph[screen_width*(2*j  )+i], 14);  
    lir_setpixel(i, cal_graph[screen_width*(2*j+1)+i], 0);  
    t2=0.1*cal_ygain*(cal_buf[mm*k+j]-buf[mm*k+j]);
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 0)t2-=YSH;
    cal_graph[screen_width*(2*j+1)+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, cal_graph[screen_width*(2*j+1)+i], 12);  
    }
  }
await_processed_keyboard();
if(kill_all_flag) return SYMFIT_ERROR;
if(lir_inkey == 'X')return SYMFIT_ERROR;
if(lir_inkey == F1_KEY || lir_inkey == '!')
  {
  help_message(300);
  if(kill_all_flag) return SYMFIT_ERROR;
  goto symfit_begin;
  }
if(lir_inkey == 'C')
  {
  sprintf(s,"Enter number of terms (10 to %d)  =>",LLSQ_MAXPAR);
  lir_text(10,12,s);
  i=lir_get_integer(11+strlen(s),12,3,10,LLSQ_MAXPAR);
  clear_lines(12,12);
  return i;
  }  
if(lir_inkey == 'B')return SYMFIT_CHANGE_LIMITS;
if(lir_inkey >= '0' && lir_inkey <= '9')return lir_inkey-'0';
if(lir_inkey == 'A')
  {
  for(i=center_region_ia*fft1_size; i<=fft1_size/2; i++)
    {
    for(j=0; j<mm; j+=2)
      {
      buf[mm*i+j]=cal_buf[mm*i+j];
      }
    }
  return SYMFIT_ACCEPT;  
  }
if(lir_inkey == '+')cal_ygain*=2;
if(lir_inkey == '-')cal_ygain/=2;
if(lir_inkey == 'L')center_region_ia-=0.003;
if(center_region_ia < 0.3)center_region_ia=0.3;
if(lir_inkey == 'R')center_region_ia+=0.0025;
if(center_region_ia > 0.4975)center_region_ia=0.4975;
lir_line(center_region_pix,screen_height*(cal_yzer+YSH),
        center_region_pix,screen_height*(cal_yzer-YSH),0);
if(kill_all_flag) return SYMFIT_ERROR;
goto redraw;
}

void make_cal_fft1_filtercorr(void)
{
int i;
// We do calibration in fft2_size points (before Linrad-02.28 it
// was done in the much larger size fft1_size)
// Start by reducing the sizes of fft1_desired and fft1_filtercorr.

resize_fft1_desired(fft1_size, fft1_desired, fft2_size, cal_fft1_desired);
for(i=0; i<twice_rxchan*fft1_size; i++)cal_fft1_filtercorr[i]=fft1_filtercorr[i];
convert_filtercorr_fd_to_td(fft1_n, fft1_size, cal_fft1_filtercorr);
resize_filtercorr_td_to_fd(TRUE, fft1_size, cal_fft1_filtercorr, 
                                      fft2_n, fft2_size, cal_fft1_filtercorr);
}

void  compute_pulse(void)
{
int i, ka, kb, siz128;
siz128=fft2_size/128;
ka=fft2_size/2-siz128;
kb=fft2_size/2+siz128;
if( (ui.rx_input_mode&IQ_DATA) != 0)
  {
  for(i=0; i<siz128; i++)
    {
    cal_tmp[2*i  ]=0;
    cal_tmp[2*i+1]=0;
    }
  for(i=ka; i<kb; i++)
    {
    cal_tmp[2*i  ]=0;
    cal_tmp[2*i+1]=0;
    }
  for(i=fft2_size-siz128; i<fft2_size; i++)
    {
    cal_tmp[2*i  ]=0;
    cal_tmp[2*i+1]=0;
    }
  }      
fftback(fft2_size, fft2_n, cal_tmp, fft2_tab, fft2_permute,FALSE);
}

#define PULPTS 8
int cal_update_ram(void)
{
int lowp, midp, higp, siz128;
int cal_hw_lowp;
int cal_hw_higp;
char s[80];
int mm, falloff_length, step;
int ia, ib;
int i, j, k, m, ja, jb, ka, kb;
int refine_cnt;
float t1,t2,t3;
float r1;
float ygain;
float z[MAX_ADCHAN];
float pul[4*PULPTS];
int desired_shape;
char *shape_texts[2]={"erfc(x)","Parabolic"};
desired_shape=0;
siz128=fft2_size/128;
step=10;
ygain=1;
mm=twice_rxchan;
//clear_screen();
settextcolor(14);
// ******************************************************
// Valid spectra:
// cal_buf4 = accumulated power and phase (normalised in fft2_size points)
// The spectrum in cal_buf4 is what we have while fft1_desired contains
// what we have asked for.
// To get the desired spectrum we have to add one more filter
// to our filter chain.
// The new filter should be fft1_desired/cal_buf4.
// ******************************************************
make_cal_fft1_filtercorr();
// ******************************************************
// The data in buf4 is now contracted to fft2 points
for(refine_cnt=0; refine_cnt<3; refine_cnt++)
  {
  if(ui.rx_rf_channels == 32)
    {
// If we have two channels, adjust the phase so pulses
// have the same phase in both channels.  
    for(i=0; i<fft2_size; i++)
      {
      t1=cal_buf4[mm*i  ];
      t2=cal_buf4[mm*i+1];
      cal_tmp[2*i  ]=cos(t2)*t1;
      cal_tmp[2*i+1]=sin(t2)*t1;
      }
    compute_pulse();  
    k=0;
    t1=0;
    t2=0;
    for(i=fft2_size-PULPTS; i<fft2_size; i++)
      {
      pul[2*k  ]=cal_tmp[2*i  ];
      pul[2*k+1]=cal_tmp[2*i+1];
      k++;
      }
    for(i=0; i<PULPTS; i++)
      {
      pul[2*k  ]=cal_tmp[2*i  ];
      pul[2*k+1]=cal_tmp[2*i+1];
      k++;
      }
    for(i=0; i<fft2_size; i++)
      {
      t1=cal_buf4[mm*i+2];
      t2=cal_buf4[mm*i+3];
      cal_tmp[2*i  ]=cos(t2)*t1;
      cal_tmp[2*i+1]=sin(t2)*t1;
      }
    compute_pulse();  
    k=0;
    t1=0;
    t2=0;
    for(i=fft2_size-PULPTS; i<fft2_size; i++)
      {
      t3=cal_tmp[2*i]*cal_tmp[2*i]+cal_tmp[2*i+1]*cal_tmp[2*i+1]+
                                     pul[2*k]*pul[2*k]+pul[2*k+1]*pul[2*k+1];
      r1=atan2(cal_tmp[2*i+1],cal_tmp[2*i])-atan2(pul[2*k+1],pul[2*k]);
      if(r1 >  PI_L)r1-=2*PI_L;
      if(r1 < -PI_L)r1+=2*PI_L;
      t1+=t3*r1;
      t2+=t3;
      k++;
      } 
    for(i=0; i<PULPTS; i++)
      {
      t3=cal_tmp[2*i]*cal_tmp[2*i]+cal_tmp[2*i+1]*cal_tmp[2*i+1]+
                                     pul[2*k]*pul[2*k]+pul[2*k+1]*pul[2*k+1];
      r1=atan2(cal_tmp[2*i+1],cal_tmp[2*i])-atan2(pul[2*k+1],pul[2*k]);
      if(r1 >  PI_L)r1-=2*PI_L;
      if(r1 < -PI_L)r1+=2*PI_L;
      t1+=t3*r1;
      t2+=t3;
      k++;
      }
    t3=t1/t2;
    if(t3 > PI_L)t3-=2*PI_L;
    if(t3 < -PI_L)t3+=2*PI_L;
    t3/=2;
// t3 is now the phase difference between channels. Correct for it.  
    for(i=0; i<fft2_size; i++)
      {
      cal_buf4[mm*i+1]+=t3;
      cal_buf4[mm*i+3]-=t3;
      }
    }
  }
for(i=0; i<2*MAX_ADCHAN*screen_width; i++)cal_graph[i]=0;
for(j=0; j<mm; j++)z[j]=0;
for(i=0;i<mm*fft2_size; i++)cal_buf2[i]=0;
// Since there may be serious errors in the spectrum of the average 
// pulse at the spectrum ends, start by locating the passband center.
// for this purpose, skip fft2_size/128 points at each end as well
// as around the center if we run in I/Q mode.
if(  (ui.rx_input_mode&IQ_DATA) == 0)
  {
  for(i=siz128; i<fft2_size-siz128; i++)
    {
    if(cal_fft1_desired[i]>0)
      {
      for(j=0; j<mm; j+=2)  
        {
        t1=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
        if(t1 > 0 )
          {
          t1=cal_buf4[mm*i+j]/sqrt(t1);
          z[j]+=t1*i;
          z[j+1]+=t1;
          cal_buf2[mm*i+j]=t1;
          }
        }
      }
    }
  }    
else
  {
  for(i=siz128; i<fft2_size/2-siz128; i++)
    {
    if(cal_fft1_desired[i]>0)
      {
      for(j=0; j<mm; j+=2)  
        {
        t1=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+
           pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
        if(t1 > 0 )
          {
          t1=cal_buf4[mm*i+j]/sqrt(t1);
          z[j]+=t1*i;
          z[j+1]+=t1;
          cal_buf2[mm*i+j]=t1;
          }
        }
      }
    }  
  for(i=fft2_size/2+siz128; i<fft2_size-siz128; i++)
    {
    if(cal_fft1_desired[i]>0)
      {
      for(j=0; j<mm; j+=2)  
        {
        t1=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
        if(t1 > 0 )
          {
          t1=cal_buf4[mm*i+j]/sqrt(t1);
          z[j]+=t1*i;
          z[j+1]+=t1;
          cal_buf2[mm*i+j]=t1;
          }
        }
      }
    }  
  }
midp=0;
for(j=0; j<mm; j+=2)
  {
  if(z[j+1]==0)
    {
    lirerr(1162);
    return 0;
    }
  midp+=z[j]/z[j+1];
  }
midp/=mm>>1;
ia=midp-fft2_size/16;
ib=midp+fft2_size/16;
// Locate the maximum near the midpoint.
for(j=0; j<mm; j+=2)
  {
  z[j]=0;
  if(  (ui.rx_input_mode&IQ_DATA) == 0)
    {
    for(i=ia; i<ib; i++)
      {
      if(z[j]<cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    }
  else
    {
    for(i=ia; i<fft2_size/2-siz128; i++)
      {
      if(z[j]<cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    for(i=fft2_size/2+siz128; i<ib; i++)
      {
      if(z[j]<cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    }
  z[j]*=.01;  
  z[j+1]=0;
  }
// Locate where the level is -20 dB at the sides relative to the level
// at the center.     
lowp=0;
higp=fft2_size-1;
k=0;
while( lowp < midp && k == 0)
  {
  k=1;
  for(j=0; j<mm; j+=2)
    {
    if( z[j]>cal_buf2[mm*lowp+j]) k=0;
    }
  lowp++;
  }
m=0;
while(higp > midp && m==0)
  {
  m=1;
  for(j=0; j<mm; j+=2)
    {
    if( z[j]>cal_buf2[mm*higp+j])m=0;
    }
  higp--;
  }
// Get a minimum level for filtercorr power to not divide by zero  
for(j=0; j<mm; j+=2)
  {
  k=higp-lowp-1;
  if(  (ui.rx_input_mode&IQ_DATA) == 0)
    {
    for(i=lowp+1; i<higp; i++)
      {
      z[j+1]+=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+
                                  pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
      }
    }
  else
    {
    for(i=lowp+1; i<fft2_size/2-siz128; i++)
      {
      z[j+1]+=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+
                                  pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
      }
    for(i=fft2_size/2+siz128; i<higp; i++)
      {
      z[j+1]+=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+
                                  pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
      }
    k-=2*siz128;
    }  
// Set level at -30dB  
  z[j+1]=0.001*z[j+1]/k;
  }
// Store the hardware spectrum in cal_buf2 as amplitude and phase
// Get it as the shape before the correction in filcorr was applied i.e.
// as cal_buf4/fft1_filtercorr
for(i=0; i<fft2_size; i++)
  {
  for(j=0; j<mm; j+=2)  
    {
    t1=pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+pow(cal_fft1_filtercorr[mm*i+j+1],2.0);
    if(t1 > z[j+1] && cal_fft1_desired[i] > 0)
      {
      cal_buf2[mm*i+j]=cal_buf4[mm*i+j]/t1;
      cal_buf2[mm*i+j+1]=cal_buf4[mm*i+j+1]-atan2(cal_fft1_filtercorr[mm*i+j+1],
                                          cal_fft1_filtercorr[mm*i+j  ]);
      }
    else
      {
      cal_buf2[mm*i+j  ]=0;
      cal_buf2[mm*i+j+1]=0;
      }
    }
  }  
for(j=0; j<mm; j+=2)  
  {
  z[j]=0;
  if(  (ui.rx_input_mode&IQ_DATA) == 0)
    {
    for(i=lowp+1; i<fft2_size; i++)
      {
      if(z[j] < cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    }  
  else
    {
    for(i=lowp+1; i<fft2_size/2-siz128; i++)
      {
      if(z[j] < cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    for(i=fft2_size/2+siz128; i<higp; i++)
      {
      if(z[j] < cal_buf2[mm*i+j])z[j]=cal_buf2[mm*i+j];
      }
    }  
  }  
for(i=0; i<fft2_size; i++)
  {
  for(j=0; j<mm; j+=2)  
    {
    cal_buf2[mm*i+j]/=z[j];
    }
  }  
// ******************************************************
// Valid spectra:
// cal_buf4 = accumulated pulse spectrum, power and phase (normalised)
// cal_buf2 = hardware spectrum, power and phase (normalised)
// ******************************************************
// Locate the MIN_GAIN points at the low and high frequency sides of cal_buf2
#define MIN_GAIN 0.1     //10dB
cal_hw_lowp=lowp;
cal_hw_higp=higp;
k=0;
while(k != ui.rx_rf_channels && cal_hw_lowp < cal_hw_higp)
  {
  k=0;
  cal_hw_lowp++;
  for(j=0; j<mm; j+=2)
    {
    if(cal_buf2[mm*cal_hw_lowp+j] > MIN_GAIN)k++;
    }
  }
k=0;  
while(k != ui.rx_rf_channels && cal_hw_lowp<=cal_hw_higp)
  {
  k=0;
  cal_hw_higp--;
  for(j=0; j<mm; j+=2)
    {
    if(cal_buf2[mm*cal_hw_higp+j] > MIN_GAIN)k++;
    }
  }      
screen:;
cal_type=CAL_TYPE_SET_FILTERSHAPE;
cal_initscreen();
t1=(0.001*fft1_size)/fft2_size;
sprintf(s,"%.1f dB points:   %fkHz     %fkHz",20*log10(MIN_GAIN),
       t1*fft1_hz_per_point*cal_hw_lowp,t1*fft1_hz_per_point*cal_hw_higp);
lir_text(0,1,s);
if( (fft1_calibrate_flag&CALAMP) != CALAMP)
  {
// This is the first time.
// Place the 75% point of the corrected spectrum at the MIN_GAIN point of
// the hardware. (75% amplitude = -4.4dB)
// Make the slope a parabola, start and stop equally many points away
// from the 75% point.
// Start with a parabola that ends halfway to the farthest end point.
// Store the desired filter with parabolic fall offs in cal_buf5.
  falloff_length=cal_hw_lowp-1;   
  if(fft2_size-cal_hw_higp-2 > falloff_length)
                    falloff_length=fft2_size-cal_hw_higp-2;
  if(desired_shape ==1)
    {
    lowp=cal_hw_lowp;
    higp=cal_hw_higp;
    }
  else
    {
    if(fft2_size/(falloff_length+1) > 20)
      {
      falloff_length=fft2_size/20;
      if(cal_hw_lowp <= falloff_length)cal_hw_lowp=falloff_length+1;   
      if(cal_hw_higp > fft2_size-falloff_length-2)
                                    cal_hw_higp=fft2_size-falloff_length-2;
      }
    t1=(ERF_END-ERF_START)/(ERF_END+0.5);
    i=falloff_length;
    falloff_length*=t1;
    i=falloff_length-i;
    lowp=cal_hw_lowp+i;
    higp=cal_hw_higp-i;
    }   
  }
else
  {
  i=0;
  while(cal_fft1_desired[i] == 0 && i<fft2_size)i++;
  if(i==fft2_size)
    {
    lirerr(1054);
    return 0;
    }
  k=fft2_size-1;
  while(cal_fft1_desired[k] == 0 && k>=i)k--;
  j=i;
  while(cal_fft1_desired[j] < 0.99999 && j<fft2_size)j++;
  falloff_length=(j-i)/2;
  if(falloff_length >= (k-i)/4)falloff_length=(k-i)/4-1;
  lowp=i+falloff_length;
  higp=k-falloff_length;
  }  
get_ideal_ampl:;
if(falloff_length >=lowp)
  {
  i=1+(falloff_length-lowp)/2;
  lowp+=i;
  falloff_length-=i;
  }
if(falloff_length >=fft2_size-1-higp)
  {
  i=1+(falloff_length-(fft2_size-higp-1))/2;
  higp-=i;
  falloff_length-=i;
  }  
for(i=0; i<lowp; i++)cal_buf5[i]=0;
for(i=higp; i<fft2_size; i++)cal_buf5[i]=0;
for(i=lowp+1; i<higp; i++)cal_buf5[i]=1;
if(desired_shape ==1)
  {
  t1=sqrt(.25)/falloff_length;
  t2=1;
  t3=t1;
  t2=1-t3*t3;
  t3+=t1;
  ja=lowp+falloff_length;
  jb=higp-falloff_length;
  while(t2 > 0)
    {
    cal_buf5[ja]=t2;
    cal_buf5[jb]=t2;
    t2=1-t3*t3;
    t3+=t1;
    ja--;
    jb++;
    }
  }
else
  {  
adjust:;
  ja=lowp+1;
  jb=higp-1;
  if(jb-ja < falloff_length/2)
    {
    lirerr(9845621);
    return 0;
    }
  t1=(ERF_END-ERF_START)/falloff_length;
  t2=ERF_START;
  while(t2 < ERF_END)
    {
    t3=0.5*erfc(t2);
    cal_buf5[ja]=t3;
    cal_buf5[jb]=t3;
    t2+=t1;
    ja--;
    jb++;
    if(ja < 0)
      {
      lowp++;
      goto adjust;
      }
    if(jb >= fft2_size)
      {
      higp--;
      goto adjust;
      }
    }
  }  
// ******************************************************
// Valid spectra:
// cal_buf4 = accumulated pulse spectrum, power and phase (normalised)
// cal_buf2 = hardware spectrum, power and phase (normalised)
// cal_buf5 = desired spectrum (amplitude only)
// ******************************************************
// Divide cal_buf5 by sqrt(cal_buf2) to get the total correction function we need 
// to get the desired frequency response.
for(i=0; i<fft2_size; i++)
  {
  for(j=0; j<mm; j+=2)
    {
    if(cal_buf2[mm*i+j]>0)
      {
      cal_buf3[mm*i+j]=cal_buf5[i]/sqrt(cal_buf2[mm*i+j]);
      }
    else
      {
      cal_buf3[mm*i+j]=0;
      }
    }
  }
// Find max of the correction function from both sides.
ja=0;
jb=fft2_size-1;
ka=siz128;
kb=siz128;
for(j=0; j<mm; j++)z[j]=0;      
for(i=0; i<fft2_size; i++)
  {
  for(j=0; j<mm; j+=2)
    {
    if(z[j  ]<cal_buf3[mm*ja+j])
      {
      z[j  ]=cal_buf3[mm*ja+j];
      ka=fft2_size/128;
      }
    if(z[j+1]<cal_buf3[mm*jb+j])
      {
      z[j+1]=cal_buf3[mm*jb+j];
      kb=fft2_size/128;
      }
    if(z[j  ]==0)ka=fft2_size/128;
    if(z[j+1]==0)kb=fft2_size/128;
    }
  if(ka>0)
    {
    ja++;
    ka--;
    } 
  if(kb>0)
    {
    jb--;
    kb--;
    }
  }
for(j=0; j<ui.rx_rf_channels; j++)
  {
  sprintf(s,"Max correction Ch %d    low=%.2fdB   high=%.2fdB     ",
                        j,20*log10(z[2*j]), 20*log10(z[2*j+1]));
  lir_text(0,2+j,s);
  }
clear_lines(5,13);  
lir_text(0, 5,"| inc | dec |      Item     |");
lir_text(0, 6,"|  A  |  B  |  Flat region  |");
lir_text(0, 7,"|  C  |  D  |  Center freq  |");
lir_text(0, 8,"|  E  |  F  |  Steepness    |");
sprintf(s,    "|  I  |  K  |  Step  %4d   |",step);
lir_text(0, 9,s);
lir_text(0,10,"|  +  |  -  |   Y-Gain      |");
lir_text(5,13,"Y=>Accept   X=Skip");                                            
settextcolor(15);
lir_text(40,5,"DESIRED AMPLITUDE");
settextcolor(14);
lir_text(40,6,"CORR. AMPLITUDE");
settextcolor(10);
lir_text(40,7,"CORR. PHASE");
settextcolor(7);
sprintf(s,"|  M  |  Shape  %s",shape_texts[desired_shape]);
lir_text(40,9,s);
sprintf(s,"|     |");
lir_text(40,8,s);
lir_text(40,10,s);
for(i=0; i<screen_width; i++)
  {
  k=i*fft2_size/screen_last_xpixel;
  if(k>fft2_size-1)k=fft2_size-1;
//k=fft2_size/2-screen_width/2+i;
  for(j=0; j<mm; j+=2)
    {
    lir_setpixel(i, cal_graph[screen_width*(2*j  )+i], 0);  
    t2=0.1*ygain*cal_buf3[mm*k+j];
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 0)t2-=YSH;
    cal_graph[screen_width*(2*j  )+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, cal_graph[screen_width*(2*j  )+i], 14);  
    lir_setpixel(i, cal_graph[screen_width*(2*j+1)+i], 0);  
    t2=0.1*ygain*cal_buf5[k];
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 0)t2-=YSH;
    cal_graph[screen_width*(2*j+1)+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, cal_graph[screen_width*(2*j+1)+i], 15);  
    lir_setpixel(i, cal_graph[screen_width*(2*j+2)+i], 0);  
    t2=0.03*ygain*cal_buf2[mm*k+j+1];
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 0)t2-=YSH;
    cal_graph[screen_width*(2*j+2)+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, cal_graph[screen_width*(2*j+2)+i], 10);  
    }
  if(k==fft2_size-1)goto grx;
  }
grx:;
if(ui.rx_rf_channels >1)
  {
  if(z[0] > z[2])
    {
    t1=z[2]/z[0];
    }
  else  
    {
    t1=z[0]/z[2];
    }
  sprintf(s,"Channel amplitudes  %f  %f   Diff %f dB",
                                  z[0],z[2],-10*log10(t1));
  if(t1 < 0.5)
    {
    clear_lines(14,15);
    lir_text(0,14,"Channel unbalance too large.");
    lir_text(0,15,"Clear old data or adjust hardware!"); 
    }
  }
await_processed_keyboard();
if(kill_all_flag) return 0;
switch (lir_inkey)
  {
  case 'X':
  return 0;
  
  case '+':
  ygain*=2;
  break;
  
  case '-':
  ygain/=2;
  break;
  
  case 'A':
  lowp-=step;
  if(lowp < 2)lowp=2;
  higp+=step;
  if(higp > fft2_size-3)higp=fft2_size-3;
  break;
  
  case 'B':
  lowp+=step;
  higp-=step;
  if(lowp>higp-2*falloff_length)
    {
    higp=(lowp+higp)/2;
    lowp=higp-falloff_length;
    higp+=falloff_length;
    } 
  break;
  
  case 'C':
  lowp+=step;
  higp+=step;
  if(higp > fft2_size-3)higp=fft2_size-3;
  break;
  
  case 'D':
  lowp-=step;
  higp-=step;
  if(lowp<2)lowp=2;
  break;
  
  case 'E':
  falloff_length-=step;
  if(falloff_length<1)falloff_length=1;
  break;
  
  case 'F':
  falloff_length+=step;
  if(falloff_length>fft2_size/2.5)falloff_length=fft2_size/2.5;
  break;

  case 'I':
  step++;
  if(step > siz128)step=siz128;
  break;
  
  case 'K':
  step--;
  if(step < 1)step=1;
  break;

  case 'M':
  desired_shape^=1;
  break;

  case F1_KEY:
  case '!':
  help_message(307);
  goto screen;

  case 'Y':
  goto update_ram; 
  }
goto get_ideal_ampl;
update_ram:;  
// ******************************************************
// Valid spectra:
// cal_buf4 = accumulated pulse spectrum, power and phase (normalised)
// cal_buf2 = hardware spectrum, power and phase (normalised)
// cal_buf5 = desired spectrum (amplitude only)
// ******************************************************
// Renormalise cal_buf4 so the average becomes 1 within the hw limits
for(j=0; j<mm; j+=2)z[j]=0;
for(i=cal_hw_lowp+1; i<cal_hw_higp; i++)
  {
  for(j=0; j<mm; j+=2)  
    {
    z[j]+=cal_buf4[mm*i+j  ];
    }
  }
for(j=0; j<mm; j+=2)z[j]/=(cal_hw_higp-cal_hw_lowp-1);
for(i=0; i<fft2_size; i++)
  {
  for(j=0; j<mm; j+=2)  
    {
    cal_buf4[mm*i+j  ]/=z[j];
    }
  }
// We get the new filter function as cal_fft1_desired/cal_buf4
// Get the new correction by multiplication onto the old.
fft1_calibrate_flag|=CALAMP;
for(i=0; i<fft2_size; i++)
  {
  cal_fft1_desired[i]=cal_buf5[i];
    {
    for(j=0; j<mm; j+=2)  
      {
      if(cal_buf4[mm*i+j]>0)
        {
        t1=sqrt(pow(cal_fft1_filtercorr[mm*i+j  ],2.0)+
                pow(cal_fft1_filtercorr[mm*i+j+1],2.0));
        t2=atan2(cal_fft1_filtercorr[mm*i+j+1],
                cal_fft1_filtercorr[mm*i+j  ]);
        t1*=cal_fft1_desired[i]/sqrt(cal_buf4[mm*i+j]);
        t2-=cal_buf4[mm*i+j+1];
        cal_fft1_filtercorr[mm*i+j  ]=t1;
        cal_fft1_filtercorr[mm*i+j+1]=t2;
        }
      else
        {
        cal_fft1_filtercorr[mm*i+j  ]=0;
        cal_fft1_filtercorr[mm*i+j+1]=0;
        }        
      }  
    }
  }
for(i=0; i<fft2_size; i++)
  {
  if(cal_fft1_desired[i] > 0)
    {
    for(j=0; j<mm; j+=2)  
      {
      t1=cal_fft1_filtercorr[mm*i+j  ];
      t2=cal_fft1_filtercorr[mm*i+j+1];
      cal_fft1_filtercorr[mm*i+j  ]=cos(t2)*t1;
      cal_fft1_filtercorr[mm*i+j+1]=sin(t2)*t1;
      }
    }
  else
    {
    for(j=0; j<mm; j+=2)  
      {
      cal_fft1_filtercorr[mm*i+j  ]=0;
      cal_fft1_filtercorr[mm*i+j+1]=0;
      }
    }  
  }
for(i=0; i<mm*fft2_size; i++)cal_tmp[i]=cal_fft1_filtercorr[i];
convert_filtercorr_fd_to_td(fft2_n, fft2_size, cal_tmp);
resize_filtercorr_td_to_fd(TRUE, fft2_size, cal_tmp, 
                   fft1_n, fft1_size, fft1_filtercorr);
resize_fft1_desired(fft2_size, cal_fft1_desired, fft1_size, fft1_desired);
i=0;
while(fft1_desired[i] == 0)
  {
  for(j=0; j<mm; j++)
    {
    fft1_filtercorr[mm*i+j]=0;
    }
  i++;
  if(i==fft1_size)lirerr(936183);
  }
i=fft1_size-1;  
while(fft1_desired[i] == 0)
  {
  for(j=0; j<mm; j++)
    {
    fft1_filtercorr[mm*i+j]=0;
    }
  i--;
  }
settextcolor(7);
clear_screen();
return 1;
}

void show_missing_cal_info(void)
{
clear_screen();
lir_text(5,5,"There is no calibration function.");
lir_text(5,7,"Run B first for amplitude and phase calibration");
lir_text(8,9,press_any_key);
await_processed_keyboard();
}


int remove_iq_notch(void)
{
char s[80];
int i, j, k, m, n, mm;
float t1,t2,t3;
int old_end, cal_lowedge_min;
float xgain, xmid, ampmax;
// In case the native size of the calibration data is smaller
// than fft1_size, change fft1_size and remember old values in fft2_size.
if(cal_fft1_size == 0)
  {
  show_missing_cal_info();
  return 1;
  }
fft2_size=fft1_size;
fft2_n=fft1_n;
if(cal_fft1_size < fft1_size)
  {
  fft1_size=cal_fft1_size;
  fft1_n=cal_fft1_n;
  init_fft1_filtercorr();
  fft1_hz_per_point*=(float)(fft2_size)/fft1_size;
  }
mm=twice_rxchan;
old_end=0;
llsq_derivatives=malloc(LLSQ_MAXPAR*fft1_size/2*sizeof(float));
if(llsq_derivatives == NULL)
  {
  lirerr(1048);
  return 0;
  }
llsq_errors=malloc(fft1_size*sizeof(float));
if(llsq_errors == NULL)
  {
  lirerr(1048);
err_ex1:;
  free(llsq_derivatives);
  return 0;
  }
xgain=(float)(screen_width)/fft1_size;
xmid=fft1_size/2;
// Do not fit curves closer than IQ_FIT_MINFREQ Hz from the center.
// We assume that the phase and amplitude is entirely unaffected
// by the AC coupling at this relatively high frequency.
// The user may change this limit if he so wishes.
cal_midlim=fft1_size/2-1-IQ_FIT_MINFREQ/fft1_hz_per_point;
// Convert fft1_filtercorr to amplitude and phase.
// Split it into a symmetric and an antisymmetric part.
for(i=0; i<fft1_size/2; i++)
  {
  k=fft1_size-1-i;
  t1=0;
  for(j=0; j<mm; j+=2)
    {
    t2=sqrt(pow(fft1_filtercorr[mm*i+j  ],2.0)+
            pow(fft1_filtercorr[mm*i+j+1],2.0));
    t3=sqrt(pow(fft1_filtercorr[mm*k+j  ],2.0)+
            pow(fft1_filtercorr[mm*k+j+1],2.0));
    if(t2 > 0 && t3 > 0)
      {
      cal_buf6[mm*i+j  ]=t2/t3;
      cal_buf3[mm*i+j  ]=sqrt(t2*t3);
      if( cal_buf3[mm*i+j  ] > t1)t1=cal_buf3[mm*i+j  ];
      t2=atan2(fft1_filtercorr[mm*i+j+1],fft1_filtercorr[mm*i+j  ]);
      t3=atan2(fft1_filtercorr[mm*k+j+1],fft1_filtercorr[mm*k+j  ]);
      cal_buf6[mm*i+j+1]=(t2-t3)/2;
      cal_buf3[mm*i+j+1]=(t2+t3)/2;
      }
    else  
      {
      if(t3 > 0)
        {
        cal_buf6[mm*i+j  ]=t2/t3;
        cal_buf3[mm*i+j  ]=sqrt(t3);
        t3=atan2(fft1_filtercorr[mm*k+j+1],fft1_filtercorr[mm*k+j  ]);
        cal_buf6[mm*i+j+1]=-t3/2;
        cal_buf3[mm*i+j+1]=t3/2;
        }
      else
        {
        if(t2 > 0)
          {
          t3=t2*0.00001;
          cal_buf6[mm*i+j  ]=t2/t3;
          cal_buf3[mm*i+j  ]=sqrt(t2*t3);
          t2=atan2(fft1_filtercorr[mm*i+j+1],fft1_filtercorr[mm*i+j  ]);
          cal_buf6[mm*i+j+1]=t2/2;
          cal_buf3[mm*i+j+1]=t2/2;
          }
        else
          {
          cal_buf6[mm*i+j  ]=0;
          cal_buf6[mm*i+j+1]=0;
          cal_buf3[mm*i+j  ]=0;
          cal_buf3[mm*i+j+1]=0;
          }
        }
      }
    }    
  if(t1 > 0)
    {
    cal_buf5[i]=1/t1;
    }
  else  
    {
    cal_buf5[i]=0;
    }
  }
// cal_buf5 is for use as weight factor in curve fitting.
// We may have incorrect data outside the passband so
// fitting is limited by cal_lowedge
cal_lowedge=0;
while(fft1_desired[cal_lowedge] != 1 && 
                            cal_lowedge < fft1_size/2)cal_lowedge++;
if(cal_lowedge < fft1_size/4)cal_lowedge=fft1_size/4;  
cal_lowedge_min=cal_lowedge;
restart:;
cal_type=CAL_TYPE_FIX_CENTER_SETUP;
cal_initscreen();
lir_text(5,5,"Remove center discontinuity");
i=fft1_hz_per_point*(fft1_size/2-cal_midlim-1);
sprintf(s,"%d Hz is excluded from polynomial fit on both",i);
lir_text(5,6,s);
lir_text(5,7,"sides of the center as indicated by the green line.");
i=fft1_hz_per_point*(fft1_size/2-cal_lowedge-1);
sprintf(s,"%d Hz away from the center is included in the polynomial fit",i);
lir_text(5,8,s);
lir_text(5,10,"L=Change center limit");
lir_text(5,11,"R=Change range to fit");
lir_text(5,13,"C=Continue with current value");
draw_fitline();
to_upper_await_keyboard();
if(kill_all_flag)
  {
err_ex2:;  
  free(llsq_errors);
  goto err_ex1;
  }
switch (lir_inkey)
  {
  case 'X':
  goto err_ex2;
  
  case 'L':
  lir_text(5,15,"New limit (Hz):");
  i=lir_get_integer(22,15,6,0,99999);
  if(kill_all_flag)goto err_ex2;
  if(i != 0)
    {
    cal_midlim=fft1_size/2-1-i/fft1_hz_per_point;
    }
  if(cal_midlim < 0.3*fft1_size)
    {
    cal_midlim=0.31*fft1_size;
    }
  goto restart;
     
  case 'R':
  lir_text(5,15,"New limit (Hz):");
  i=lir_get_integer(22,15,7,0,999999);
  if(kill_all_flag)goto err_ex2;
  if(i != 0)
    {
    cal_lowedge=fft1_size/2-1-i/fft1_hz_per_point;
    }
  if(cal_lowedge < cal_lowedge_min)cal_lowedge=cal_lowedge_min;
  if(cal_midlim-cal_lowedge < fft1_size/32)
    {
    cal_lowedge = 1+fft1_size/32-cal_midlim;
    }
  goto restart;
     
  case F1_KEY:
  case '!':
  help_message(308);
  goto restart;

  case 'C':
  break;
  
  default:
  goto restart;
  }
  
if(kill_all_flag)goto err_ex2;
llsq_neq=cal_midlim-cal_lowedge;
if(llsq_neq < fft1_size/32)
  {
  lirerr(1137);
  goto err_ex2;
  }
ampmax=0;
for(i=cal_lowedge; i<cal_midlim; i++)
  {
  if(ampmax<cal_buf5[i])ampmax=cal_buf5[i];
  }
for(i=0; i<fft1_size/2; i++)
  {
  for(j=0; j<mm; j+=2)
    {
    cal_buf3[mm*i+j  ]*=ampmax;
    }
  }
// ***********************************************************
// The antisymmetric power component has to be 1.0 at (fft1_size-1)/2
// Fit it to y=c1*x + c2*x*x + c3*x*x*x ........
llsq_npar=3;
cal_ygain=1;
fit_asym_power:;
make_symfit(0, 0, cal_buf6, 1.0);
llsq_npar=symmetry_fit_decide(cal_buf6, 1., "Antisymmetric power (RF)");  
if(llsq_npar == SYMFIT_CHANGE_LIMITS)goto restart;
if(llsq_npar == -1)goto err_ex2;
if(llsq_npar > 0)goto fit_asym_power;  
// ***********************************************************
// The antisymmetric phase may have a dicontinuity at (fft1_size-1)/2.
// Fit it to y=c1 + c2*x + c3*x*x ........
llsq_npar=5;
fit_asym_phase:;
make_symfit(1,0,&cal_buf6[1],0.);
llsq_npar=symmetry_fit_decide(&cal_buf6[1], 0.,"Antisymmetric phase (AF)");  
if(llsq_npar == SYMFIT_CHANGE_LIMITS)goto restart;
if(llsq_npar == -1)goto err_ex2;
if(llsq_npar > 0)goto fit_asym_phase;  
// Make the antisymmetric phase cross zero at (fft1_size-1)/2 in
// case the user decides to use the fitted values.
for(i=0; i<=fft1_size/2; i++)
  {
  for(j=0;j<ui.rx_rf_channels;j++)
    {
    cal_buf6[mm*i+2*j+1]-=llsq_steps[j];
    }
  }  
// ***********************************************************
// The symmetric power part must be a symmetric function (even).
// Fit it to y=c1 + c2*x*x + c3*x*x*x*x ........
llsq_npar=6;
fit_sym_power:;
make_symfit(1,1,cal_buf3,0.);
llsq_npar=symmetry_fit_decide(cal_buf3, 0.,"Symmetric power (AF)");  
if(llsq_npar == SYMFIT_CHANGE_LIMITS)goto restart;
if(llsq_npar == -1)goto err_ex2;
if(llsq_npar > 0)goto fit_sym_power;  
// ***********************************************************
// The symmetric phase part must be a symmetric function (even).
// Fit it to y=c1 + c2*x*x + c3*x*x*x*x ........
llsq_npar=3;
fit_sym_phase:;
make_symfit(1,1,&cal_buf3[1],0.);
llsq_npar=symmetry_fit_decide(&cal_buf3[1], 0.,"Symmetric phase (RF)");  
if(llsq_npar == SYMFIT_CHANGE_LIMITS)goto restart;
if(llsq_npar == -1)goto err_ex2;
if(llsq_npar > 0)goto fit_sym_phase;  
// Combine the symmetric and antisymmetric functions to give back
// the total filter response.
for(i=1; i<fft1_size/2; i++)
  {
  k=fft1_size-1-i;
  for(j=0; j<mm; j+=2)
    {
    t1=cal_buf3[mm*i+j];
    t2=sqrt(cal_buf6[mm*i+j]);
    if(t1>0 && t2>0)
      {
      cal_buf3[mm*k+j]=t1/t2;
      cal_buf3[mm*i+j]=t1*t2;
      t1=cal_buf3[mm*i+j+1]-cal_buf6[mm*i+j+1];
      t2=cal_buf3[mm*i+j+1]+cal_buf6[mm*i+j+1];
      cal_buf3[mm*k+j+1]=t1;    
      cal_buf3[mm*i+j+1]=t2;
      }
    else
      {
      cal_buf3[mm*k+j]=0;
      cal_buf3[mm*i+j]=0;
      cal_buf3[mm*k+j+1]=0;    
      cal_buf3[mm*i+j+1]=0;
      }
    }
  }  
cal_ygain=1;
cal_type=CAL_TYPE_FIX_CENTER_SAVE;
cal_initscreen();
lir_text(4,2,"+/- => Y-scale");
lir_text(4,3,"E,C => Expand,Contract x-scale");
lir_text(4,4,"R,L => Right/Left x-scale");
lir_text(4,5,"X  => Exit without saving");
lir_text(4,6,"S  => Update RAM and save to disk");
show:;
for(j=0; j<mm; j++)
  {
  m=screen_width*j;
  n=screen_width*mm;
  for(i=1; i<=old_end; i++)
    {
    lir_line(cal_graph[n+i-1], cal_graph[m+i-1],
            cal_graph[n+i  ], cal_graph[m+i  ],0);
    if(kill_all_flag)goto err_ex2;
    }
  }
old_end=-1;  
t1=0;
i=-1;
show1:
t1+=xgain;
if(t1<i+1)goto show1;
i=t1;
if(i>screen_last_xpixel)goto showx;
k=(t1-screen_width/2)/xgain+xmid;
while(k<0)k+=fft1_size;
while(k>=fft1_size)k-=fft1_size;
old_end++;
cal_graph[screen_width*mm+old_end]=i;
for(j=0; j<mm; j+=2)
  {
  t2=0.1*cal_ygain*cal_buf3[mm*k+j  ];
  if(t2 <-cal_ymax)t2=-cal_ymax;
  if(t2 >cal_ymax)t2=cal_ymax;
  if(j > 0)t2-=YSH;
  cal_graph[screen_width*j+old_end]=screen_height*(cal_yzer-t2);
  t2=0.03*cal_ygain*cal_buf3[mm*k+j+1];
  if(t2 <-cal_ymax)t2=-cal_ymax;
  if(t2 >cal_ymax)t2=cal_ymax;
  if(j > 0)t2-=YSH;
  cal_graph[screen_width*(j+1)+old_end]=screen_height*(cal_yzer-t2);
  if(old_end>0)
    {
    lir_line(cal_graph[screen_width*mm+old_end-1],
            cal_graph[screen_width*j+old_end-1],
          i,cal_graph[screen_width*j+old_end],13);
    if(kill_all_flag) goto err_ex2;
    lir_line(cal_graph[screen_width*mm+old_end-1],
         cal_graph[screen_width*(j+1)+old_end-1],
       i,cal_graph[screen_width*(j+1)+old_end],10);
    if(kill_all_flag) goto err_ex2;
    }
  }
goto show1;
showx:;
await_processed_keyboard();
if(kill_all_flag) goto err_ex2;
switch (lir_inkey)
  {
  case 'S':
  for(i=0; i<fft1_size; i++)
    {
    for(j=0; j<mm; j+=2)
      {
      t1=cal_buf3[mm*i+j  ]/ampmax;
      t2=cal_buf3[mm*i+j+1];
      fft1_filtercorr[mm*i+j  ]=t1*cos(t2);
      fft1_filtercorr[mm*i+j+1]=t1*sin(t2);
      }
    }  
  write_filcorr(0);
  free(llsq_derivatives);
  free(llsq_errors);
// Restore filtercorr and fft1 desired.
  if(fft2_size != fft1_size)
    {
    fft1_hz_per_point/=(float)(fft2_size)/fft1_size;
    fft1_size=fft2_size;
    fft1_n=fft2_n;
    init_fft1_filtercorr();
    }
  return 1;

  case 'X':
  goto err_ex2;
    
  case '+':
  cal_ygain*=2;
  break;
  
  case '-':
  cal_ygain/=2;
  break;

  case 'C':
  xgain/=2;
  break;

  case 'E':
  xgain*=2;
  break;

  case 'R':
  xmid-=0.25*screen_width/xgain;
  break;

  case 'L':
  xmid+=0.25*screen_width/xgain;
  break;


  case F1_KEY:
  case '!':
  help_message(309);
  break;
  }
goto show;
}

