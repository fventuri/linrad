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
#if(OSNUM == OSNUM_LINUX)
#include <pthread.h>
#endif
#include "uidef.h"
#include "fft1def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "seldef.h"
#include "screendef.h"
#include "options.h"
#include "thrdef.h"

void detect_fm(void);
float mix2_phase=0;      

void do_mix2(void)
{
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_MIX2);
#endif
resume:;
thread_status_flag[THREAD_MIX2]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
         thread_command_flag[THREAD_MIX2]==THRFLAG_ACTIVE)
  {
// *******************************************************
  thread_status_flag[THREAD_MIX2]=THRFLAG_SEM_WAIT;
  lir_sched_yield();
  lir_await_event(EVENT_MIX2);
  thread_status_flag[THREAD_MIX2]=THRFLAG_ACTIVE;
  lir_sched_yield();
  while(!kill_all_flag &&
         thread_command_flag[THREAD_MIX2]==THRFLAG_ACTIVE &&
         ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
        ((baseb_pa-baseb_py+baseband_size)&baseband_mask) < baseband_size-4*(int)mix2.size)

    {
    fft3_mix2();
    }
  }
if(thread_command_flag[THREAD_MIX2] == THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_MIX2]=THRFLAG_IDLE;
  while(!kill_all_flag && thread_command_flag[THREAD_MIX2] == THRFLAG_IDLE)
    {
    lir_sleep(2000);
    }
  if(thread_command_flag[THREAD_MIX2] == THRFLAG_ACTIVE)goto resume;
  }
thread_status_flag[THREAD_MIX2]=THRFLAG_RETURNED;
while(!kill_all_flag)
  {
  if(thread_command_flag[THREAD_MIX2] == THRFLAG_KILL)return;
  lir_sleep(1000);
  }
}

void fft3_mix2(void)
{
int i, j, k, ia, ib, ic, id, ix, iy, pb, p0, sizhalf;
int last_point;
float latest_hold_max1;
int latest_hold_pos1;
float latest_hold_max2;
int latest_hold_pos2;
float ampfac,cg_mid;
float big;
int nn, mm;
int pa, resamp;
#define NOISE_FILTERS 2
#define NOISE_POINTS 5
float noise[2*NOISE_FILTERS];
float t1,t2,t3,t4,t5;
float r1,r2,c1,c2,c3;
float noise_floor, a2, b2, re, im;
float noi2, sina, a2s, b2s;
float sellim_correction, fac;
double dr3, dt1, dt2, dt3, dt4, dr4;
resamp=fft3_size/mix2.size;
noise_floor=0;
sizhalf=mix2.size/2;
if(sw_onechan)
  {
  nn=2*(mix2.size-1);
  if(genparm[CW_DECODE_ENABLE] != 0)
    {
// Compute the power within several narrow filters near the passband.
// Get the average while excluding large values.
// This way occasional signals outside our filter will not
// be included in the noise floor.
    mm=bg_flatpoints+2*bg_curvpoints+NOISE_SEP;
    t3=BIGFLOAT;
    for(k=0; k<NOISE_FILTERS; k++)
      {
      t1=0;
      t2=0;
      for(i=0; i<NOISE_POINTS; i++)
        {
        j=mm+i+k*(NOISE_POINTS+NOISE_SEP);
        t1+=fft3_slowsum[fft3_size/2-bg_first_xpoint+j];
        t2+=fft3_slowsum[fft3_size/2-bg_first_xpoint-j];
        }
      noise[2*k]=t1;
      noise[2*k+1]=t2;
      if(t1<t3)t3=t1;
      if(t2<t3)t3=t2;
      }
    k=0;
    t1=0;
    t3*=1.5;
    for(i=0; i<2*NOISE_FILTERS; i++)
      {
      if(noise[i] < t3)
        {
        t1+=noise[i];
        k++;
        }
      }
    noise_floor=t1/(k*NOISE_POINTS);
    }
  if(bg.mixer_mode == 1)
    {
// Filter and decimate in the frequency domain.    
// Select mix2.size points centered at fft3_size/2, and move them 
// to mi2_tmp while multiplying with the baseband filter function.
    p0=fft3_px+fft3_size;
    k=fft3_size/2;
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[2*i  ]=fft3[p0+2*i  ]*bg_filterfunc[k+i];    
      mi2_tmp[2*i+1]=fft3[p0+2*i+1]*bg_filterfunc[k+i];    
      }
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[nn-2*i  ]=fft3[p0-2*i-2]*bg_filterfunc[k-i-1];    
      mi2_tmp[nn-2*i+1]=fft3[p0-2*i-1]*bg_filterfunc[k-i-1];    
      }
    fftback(mix2.size, mix2.n, mi2_tmp, mix2.table, mix2.permute, 
                                                       yieldflag_ndsp_mix2);
// Store baseb_raw as the time function we obtain from the back transformation.
    if(genparm[THIRD_FFT_SINPOW] == 2)
      {
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*baseb_pa+2*i  ]+=mi2_tmp[2*i];     
        baseb_raw[2*baseb_pa+2*i+1]+=mi2_tmp[2*i+1];     
        }
      p0=(baseb_pa+sizhalf)&baseband_mask;
      for(i=0; i<(int)mix2.size; i++)
        {
        baseb_raw[2*p0+i]=mi2_tmp[mix2.size+i];     
        }
      }
    else
      {
      p0=baseb_pa;
      k=mix2.interleave_points-2*(mix2.crossover_points/2);
      j=k/2+mix2.crossover_points;
      ia=2*mix2.crossover_points;
      for(i=0; i<ia; i+=2)
        {
        baseb_raw[2*p0  ]=baseb_raw[2*p0  ]*mix2.cos2win[i>>1]+mi2_tmp[i+k  ]*mix2.sin2win[i>>1];
        baseb_raw[2*p0+1]=baseb_raw[2*p0+1]*mix2.cos2win[i>>1]+mi2_tmp[i+k+1]*mix2.sin2win[i>>1];
        p0=(p0+1)&baseband_mask;
        } 
      ib=mix2.new_points+2+2*(mix2.crossover_points/2);
      for(i=ia; i<ib; i+=2)
        {
        baseb_raw[2*p0  ]=mi2_tmp[i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=mi2_tmp[i+k+1]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        j++;
        } 
      j--;  
      ic=2*mix2.new_points;  
      for(i=ib; i<ic; i+=2)
        {
        j--;
        baseb_raw[2*p0  ]=mi2_tmp[i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=mi2_tmp[i+k+1]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        } 
      id=2*(mix2.crossover_points+mix2.new_points);
      for(i=ic; i<id; i+=2)
        {
        baseb_raw[2*p0  ]=mi2_tmp[i+k  ];
        baseb_raw[2*p0+1]=mi2_tmp[i+k+1];
        p0=(p0+1)&baseband_mask;
        } 
      }    
    }
  if(bg.mixer_mode == 2)
    {
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebraw_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebraw_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebraw_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
    p0=baseb_pa;
    for(k=1; k<=(int)mix2.new_points; k++)
      {
      pa=(timf3_py+2*(1-basebraw_fir_pts+fft3_size-fft3_new_points+k*resamp)+
                                                    timf3_size)&timf3_mask;
      mm=pa;
      t1=timf3_float[pa  ]*basebraw_fir[basebraw_fir_pts/2];
      t2=timf3_float[pa+1]*basebraw_fir[basebraw_fir_pts/2];
      for(i=basebraw_fir_pts/2-1; i>=0; i--)
        {
        pa=(pa+2)&timf3_mask;
        mm=(mm-2+timf3_size)&timf3_mask;
        t1+=(timf3_float[pa  ]+timf3_float[mm  ])*basebraw_fir[i];
        t2+=(timf3_float[pa+1]+timf3_float[mm+1])*basebraw_fir[i];
        }
      baseb_raw[2*p0  ]=t1;
      baseb_raw[2*p0+1]=t2;
      p0=(p0+1)&baseband_mask;
      }
    }
  if(fm_pilot_size == 0)
    {    
// Select mix2.size points centered at fft3_size/2, and move them 
// to carr while multiplying with a filter function that is 
// bg.coh_factor times narrower than the baseband filter function
    k=fft3_size/2;
    for(i=0; i<sizhalf; i++)
      {
      carr_tmp[2*i  ]=fft3[fft3_px+fft3_size+2*i  ]*bg_carrfilter[k+i];    
      carr_tmp[2*i+1]=fft3[fft3_px+fft3_size+2*i+1]*bg_carrfilter[k+i];    
      }
    for(i=0; i<sizhalf; i++)
      {
      carr_tmp[nn-2*i  ]=fft3[fft3_px+fft3_size-2*i-2]*bg_carrfilter[k-i-1];    
      carr_tmp[nn-2*i+1]=fft3[fft3_px+fft3_size-2*i-1]*bg_carrfilter[k-i-1];    
      }
    }
  }  
else
  {
// ********************************************************************

// *********************************************************************
// !!!!!!!!!!!!!! There are two rx channels. !!!!!!!!!!!!!!!!!!
  nn=4*(mix2.size-1);
  if(fft1_correlation_flag != 0)goto good_poldata;
// ****************************************************************** 
  if( genparm[CW_DECODE_ENABLE] != 0)
    {
// Compute the power within several narrow filters near the passband.
// Get the average while excluding large values.
// This way occasional signals outside our filter will not
// be included in the noise floor.
    mm=bg_flatpoints+2*bg_curvpoints+NOISE_SEP;
    t3=BIGFLOAT;
    for(k=0; k<NOISE_FILTERS; k++)
      {
      t1=0;
      t2=0;
      for(i=0; i<NOISE_POINTS; i++)
        {
        j=mm+i+k*(NOISE_POINTS+NOISE_SEP);
        t1+=fft3_slowsum[2*(fft3_size/2-bg_first_xpoint+j)];
        t2+=fft3_slowsum[2*(fft3_size/2-bg_first_xpoint-j)];
        }
      noise[2*k]=t1;
      noise[2*k+1]=t2;
      if(t1<t3)t3=t1;
      if(t2<t3)t3=t2;
      }
    k=0;
    t1=0;
    t3*=1.5;
    for(i=0; i<2*NOISE_FILTERS; i++)
      {
      if(noise[i] < t3)
        {
        t1+=noise[i];
        k++;
        }
      }
    noise_floor=t1/(k*NOISE_POINTS);
    }
// END  if( genparm[CW_DECODE_ENABLE] != 0)
// *************************************************************


// Select mix2.size points centered at fft3_size/2, and move them 
// to mi2_tmp while multiplying with the baseband filter function.
// Transform the polarization at the same time.
// We fill mi2_tmp even when the filter and decimation will be
// made in the time domain with a FIR filter in order to have
// the polarization computed here by use of the fft3 spectrum.  
  a2=0;
  b2=0;
  re=0;
  im=0;
  p0=fft3_px+2*fft3_size;
  k=fft3_size/2;
  for(i=0; i<sizhalf; i++)
    {
    if(bg_filterfunc[k+i] == 0)
      {
      mi2_tmp[4*i  ]=0;
      mi2_tmp[4*i+1]=0;
      mi2_tmp[4*i+2]=0;
      mi2_tmp[4*i+3]=0;
      }
    else
      {  
      t1=fft3[p0+4*i  ];
      t2=fft3[p0+4*i+1];
      t3=fft3[p0+4*i+2];
      t4=fft3[p0+4*i+3];
      mi2_tmp[4*i  ]=(pg.c1*t1+pg.c2*t3+pg.c3*t4)*bg_filterfunc[k+i];
      mi2_tmp[4*i+1]=(pg.c1*t2+pg.c2*t4-pg.c3*t3)*bg_filterfunc[k+i];
      mi2_tmp[4*i+2]=(pg.c1*t3-pg.c2*t1+pg.c3*t2)*bg_filterfunc[k+i];
      mi2_tmp[4*i+3]=(pg.c1*t4-pg.c2*t2-pg.c3*t1)*bg_filterfunc[k+i];
      r1=P4SCALE*(fft3_slowsum[fft3_size-2*bg_first_xpoint+2*i]-noise_floor);
      if(r1 > 0  && pg.adapt == 0)
        {
        a2+=r1*(mi2_tmp[4*i  ]*mi2_tmp[4*i  ]+
                mi2_tmp[4*i+1]*mi2_tmp[4*i+1]);
        b2+=r1*(mi2_tmp[4*i+2]*mi2_tmp[4*i+2]+
                mi2_tmp[4*i+3]*mi2_tmp[4*i+3]);
        re+=r1*(mi2_tmp[4*i  ]*mi2_tmp[4*i+2]+
                mi2_tmp[4*i+1]*mi2_tmp[4*i+3]);
        im+=r1*(-mi2_tmp[4*i  ]*mi2_tmp[4*i+3]+
                mi2_tmp[4*i+1]*mi2_tmp[4*i+2]);
        }
      }
    }
  for(i=0; i<sizhalf; i++)
    {
    if(bg_filterfunc[k-i-1] == 0)
      {
      mi2_tmp[nn-4*i  ]=0;
      mi2_tmp[nn-4*i+1]=0;
      mi2_tmp[nn-4*i+2]=0;
      mi2_tmp[nn-4*i+3]=0;
      }
    else
      {  
      t1=fft3[p0-4*i-4];
      t2=fft3[p0-4*i-3];
      t3=fft3[p0-4*i-2];
      t4=fft3[p0-4*i-1];
      mi2_tmp[nn-4*i  ]=(pg.c1*t1+pg.c2*t3+pg.c3*t4)*bg_filterfunc[k-i-1];
      mi2_tmp[nn-4*i+1]=(pg.c1*t2+pg.c2*t4-pg.c3*t3)*bg_filterfunc[k-i-1];
      mi2_tmp[nn-4*i+2]=(pg.c1*t3-pg.c2*t1+pg.c3*t2)*bg_filterfunc[k-i-1];
      mi2_tmp[nn-4*i+3]=(pg.c1*t4-pg.c2*t2-pg.c3*t1)*bg_filterfunc[k-i-1];
      r1=P4SCALE*(fft3_slowsum[fft3_size-2*bg_first_xpoint-2*i]-noise_floor);
      if(r1 > 0  && pg.adapt == 0)
        {
        a2+=r1*(mi2_tmp[nn-4*i  ]*mi2_tmp[nn-4*i  ]+
                mi2_tmp[nn-4*i+1]*mi2_tmp[nn-4*i+1]);
        b2+=r1*(mi2_tmp[nn-4*i+2]*mi2_tmp[nn-4*i+2]+
                mi2_tmp[nn-4*i+3]*mi2_tmp[nn-4*i+3]);
        re+=r1*(mi2_tmp[nn-4*i  ]*mi2_tmp[nn-4*i+2]+
                mi2_tmp[nn-4*i+1]*mi2_tmp[nn-4*i+3]);
        im+=r1*(-mi2_tmp[nn-4*i  ]*mi2_tmp[nn-4*i+3]+
                 mi2_tmp[nn-4*i+1]*mi2_tmp[nn-4*i+2]);
        }
      }
    }
// In case the polarization is already correct, a2 is large because
// it is S+N while the other numbers are small, containing noise only.
// Now, to follow the polarization correctly for really weak
// signals do not just use the data as they come here.
// Store the result in poleval_data and adjust pol first when it
// is quite certain that a new set of coefficients is better. 
  if(pg.adapt != 0)goto good_poldata;
  poleval_data[poleval_pointer].a2=a2;
  poleval_data[poleval_pointer].b2=b2;
  poleval_data[poleval_pointer].re=re;
  poleval_data[poleval_pointer].im=im;
  for(i=0; i<poleval_pointer; i++)
    {
    a2+=poleval_data[i].a2;
    b2+=poleval_data[i].b2;
    re+=poleval_data[i].re;
    im+=poleval_data[i].im;
    }
  poleval_pointer++;  
  if(poleval_pointer<POLEVAL_SIZE/5)goto good_poldata;
// Check afcsub.c or blank1 .c for an explanation how pol 
// coefficients are calculated.
// If we shall change pol coefficients now, we must be sure that
// the phase angle between a and b is accurate.
// Step through the data and sum the square of the phase deviation.
// Note that the phase is two dimensional. 
// The angle between polarization planes and the phase angle 
// between voltages. 
  t1=a2+b2;
  if(t1 == 0)goto bad_poldata;
  a2/=t1;
  b2/=t1;
  re/=t1;
  im/=t1;
  r1=0;
  r2=0;
  for(i=0; i<poleval_pointer; i++)
    {
    t1=poleval_data[i].a2+poleval_data[i].b2;
    t2=re*t1;
    t3=im*t1;
    r1+=poleval_data[i].re*poleval_data[i].re+
        poleval_data[i].im*poleval_data[i].im;
    r2+=-(t2-poleval_data[i].re)*(t2-poleval_data[i].re)+
        (t3-poleval_data[i].im)*(t3-poleval_data[i].im);
    }
  r2/=r1;
  if(r2 >0.2)goto bad_poldata;
  t2=re*re+im*im;
  noi2=a2*b2-t2;
  if(noi2 > 0.12)goto bad_poldata;
  a2s=a2-noi2;
  b2s=b2-noi2;
  if(b2s <=0.0001 || t2 == 0)
    {
    poleval_pointer=0;
    if(b2 > a2)
      {
      t1=atan2(pg.c2,pg.c3);
      t2=pg.c1;
      pg.c1=sqrt(pg.c2*pg.c2+pg.c3*pg.c3);
      pg.c2=t2*cos(t1);
      pg.c3=t2*sin(t1);
// The signs of c2 and c3 are most probably incorrect here!!! ööö
// but the situation is unusual and probably t2=0 always when 
// b2=1 and a2=0. Check and make sure the new pol parameters give
// the orthogonal polarization.....       
      }
    }
  if(a2s > 0)
    {
    c1=sqrt(a2s);
    sina=sqrt(b2s);
    c2=sina*re/sqrt(t2);
    c3=-sina*im/sqrt(t2);
    t1=sqrt(c1*c1+c2*c2+c3*c3);
    if(c2 < 0)t1=-t1;
    c1/=t1;
    c2/=t1;
    c3/=t1;
    }
  else
    {
    if(a2 > b2)
      {
      c1=1;
      c2=0;
      }
    else
      {
      c1=0;
      c2=1;
      }
    c3=0;
    }    
// The current a and b signals were produced by:
// re_a=pg.c1*re_x+pg.c2*re_y+pg.c3*im_y (1)
// im_a=pg.c1*im_x+pg.c2*im_y-pg.c3*re_y (2)
// re_b=pg.c1*re_y-pg.c2*re_x+pg.c3*im_x (3)
// im_b=pg.c1*im_y-pg.c2*im_x-pg.c3*re_x (4)
// We have now found that improved a and b signals will be obtained from:
// new_re_a=c1*re_a+c2*re_b+c3*im_b (5)
// new_im_a=c1*im_a+c2*im_b-c3*re_b (6)
// new_re_b=c1*re_b-c2*re_a+c3*im_a (7)
// new_im_b=c1*im_b-c2*im_a-c3*re_a (8)
// Eliminate the old a and b signals
// new_re_a=c1*(pg.c1*re_x+pg.c2*re_y+pg.c3*im_y)
//         +c2*(pg.c1*re_y-pg.c2*re_x+pg.c3*im_x)
//         +c3*(pg.c1*im_y-pg.c2*im_x-pg.c3*re_x)
// new_re_a=c1*pg.c1*re_x+c1*pg.c2*re_y+c1*pg.c3*im_y
//         +c2*pg.c1*re_y-c2*pg.c2*re_x+c2*pg.c3*im_x
//         +c3*pg.c1*im_y-c3*pg.c2*im_x-c3*pg.c3*re_x
// new_re_a=(c1*pg.c1-c2*pg.c2-c3*pg.c3)*re_x
//         -(c3*pg.c2-c2*pg.c3)*im_x
//         +(c1*pg.c2+c2*pg.c1)*re_y
//         +(c1*pg.c3+c3*pg.c1)*im_y
  t1=c1*pg.c1-c2*pg.c2-c3*pg.c3;
  t2=c3*pg.c2-c2*pg.c3;
  t3=c1*pg.c2+c2*pg.c1;
  t4=c1*pg.c3+c3*pg.c1;
// We want t2 to be zero so we adjust the phase.
  c1=sqrt(t1*t1+t2*t2);
  r2=sqrt(t3*t3+t4*t4);
  r1=atan2(t3,t4)+atan2(t1,t2);
  c2=-r2*cos(r1);
  c3=r2*sin(r1);
  t1=sqrt(c1*c1+c2*c2+c3*c3);
  if(t1 > 0)
    {
    t2=c1*pg.c1+c2*pg.c2+c3*pg.c3;
    if(t2 < 0)t1=-t1;
    c1/=t1;
    c2/=t1;
    c3/=t1;
    t1=pg.avg-1;
    pg.c1=(t1*pg.c1+c1)/pg.avg;
    pg.c2=(t1*pg.c2+c2)/pg.avg;
    pg.c3=(t1*pg.c3+c3)/pg.avg;
    t1=sqrt(pg.c1*pg.c1+pg.c2*pg.c2+pg.c3*pg.c3);
    if(pg.c2 < 0)t1=-t1;
    pg.c1/=t1;
    pg.c2/=t1;
    pg.c3/=t1;
    if(recent_time-show_pol_time > 0.1)
      {  
      sc[SC_SHOW_POL]++;
      show_pol_time=current_time();
      }
    }
  poleval_pointer=0;
  goto good_poldata;
bad_poldata:;
  if(poleval_pointer > 3*POLEVAL_SIZE/4)
    {
    j=poleval_pointer;
    k=POLEVAL_SIZE/4;
    poleval_pointer=0;
    for(i=k; i<j; i++)
      {
      poleval_data[poleval_pointer].a2=poleval_data[poleval_pointer+k].a2;
      poleval_data[poleval_pointer].b2=poleval_data[poleval_pointer+k].b2;
      poleval_data[poleval_pointer].re=poleval_data[poleval_pointer+k].re;
      poleval_data[poleval_pointer].im=poleval_data[poleval_pointer+k].im;
      poleval_pointer++;
      }
    }        
good_poldata:;
// Select mix2.size points centered at fft3_size/2, and move them 
// to mi2_tmp while multiplying with a filter function that is 
// bg.coh_factor times narrower than the baseband filter function
// Just process the first polarization. 
// The user is responsible for the polarization to be right!
  if(fm_pilot_size == 0)
    {    
    nn=2*(mix2.size-1);
    k=fft3_size/2;
    p0=fft3_px+2*fft3_size;
    if(fft1_correlation_flag >= 2)
      {
      for(i=0; i<sizhalf; i++)
        {
        d_carr_tmp[4*i  ]=d_fft3[p0+4*i  ]*d_basebcarr_fir[k+i];    
        d_carr_tmp[4*i+1]=d_fft3[p0+4*i+1]*d_basebcarr_fir[k+i];   
        d_carr_tmp[4*i+2]=d_fft3[p0+4*i+2]*d_basebcarr_fir[k+i];    
        d_carr_tmp[4*i+3]=d_fft3[p0+4*i+3]*d_basebcarr_fir[k+i];   
        }
      for(i=0; i<sizhalf; i++)
        {
        d_carr_tmp[2*nn-4*i  ]=d_fft3[p0-4*i-4]*d_basebcarr_fir[k-i-1];    
        d_carr_tmp[2*nn-4*i+1]=d_fft3[p0-4*i-3]*d_basebcarr_fir[k-i-1];    
        d_carr_tmp[2*nn-4*i+2]=d_fft3[p0-4*i-2]*d_basebcarr_fir[k-i-1];    
        d_carr_tmp[2*nn-4*i+3]=d_fft3[p0-4*i-1]*d_basebcarr_fir[k-i-1];    
        }
      }
    else
      { 
      for(i=0; i<sizhalf; i++)
        {
        carr_tmp[2*i  ]=fft3[p0+4*i  ]*bg_carrfilter[k+i];    
        carr_tmp[2*i+1]=fft3[p0+4*i+1]*bg_carrfilter[k+i];   
        }
      for(i=0; i<sizhalf; i++)
        {
        carr_tmp[nn-2*i  ]=fft3[p0-4*i-4]*bg_carrfilter[k-i-1];
        carr_tmp[nn-2*i+1]=fft3[p0-4*i-3]*bg_carrfilter[k-i-1];    
        }
      }
    }
if(kill_all_flag || thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
  if(bg.mixer_mode == 1)
    {
    if(fft1_correlation_flag > 1)
      {
      lirerr(7385);
      }
    dual_fftback(mix2.size, mix2.n, mi2_tmp, 
                              mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// Store baseb_raw and baseb_raw_orthog.
    if(genparm[THIRD_FFT_SINPOW] == 2)
      {
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*baseb_pa+2*i  ]+=mi2_tmp[4*i  ];     
        baseb_raw[2*baseb_pa+2*i+1]+=mi2_tmp[4*i+1];     
        baseb_raw_orthog[2*baseb_pa+2*i  ]+=mi2_tmp[4*i+2];     
        baseb_raw_orthog[2*baseb_pa+2*i+1]+=mi2_tmp[4*i+3];     
        }
      k=sizhalf;
      p0=(baseb_pa+sizhalf)&baseband_mask;        
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*p0+2*i  ]=mi2_tmp[4*(i+k)  ];     
        baseb_raw[2*p0+2*i+1]=mi2_tmp[4*(i+k)+1];     
        baseb_raw_orthog[2*p0+2*i  ]=mi2_tmp[4*(i+k)+2];     
        baseb_raw_orthog[2*p0+2*i+1]=mi2_tmp[4*(i+k)+3];     
        }
      }
    else
      {
      p0=baseb_pa;
      k=mix2.interleave_points-2*(mix2.crossover_points/2);
      j=k/2+mix2.crossover_points;
      k*=2;
      ia=2*mix2.crossover_points;
      for(i=0; i<ia; i+=2)
        {
        baseb_raw[2*p0  ]=baseb_raw[2*p0  ]*mix2.cos2win[i>>1]+
                             mi2_tmp[2*i+k  ]*mix2.sin2win[i>>1];
        baseb_raw[2*p0+1]=baseb_raw[2*p0+1]*mix2.cos2win[i>>1]+
                             mi2_tmp[2*i+k+1]*mix2.sin2win[i>>1];
        baseb_raw_orthog[2*p0  ]=baseb_raw_orthog[2*p0  ]*mix2.cos2win[i>>1]+
                             mi2_tmp[2*i+k+2]*mix2.sin2win[i>>1];
        baseb_raw_orthog[2*p0+1]=baseb_raw_orthog[2*p0+1]*mix2.cos2win[i>>1]+
                             mi2_tmp[2*i+k+3]*mix2.sin2win[i>>1];
        p0=(p0+1)&baseband_mask;
        }
      ib=mix2.new_points+2+2*(mix2.crossover_points/2);
      for(i=ia; i<ib; i+=2)
        {
        baseb_raw[2*p0  ]=mi2_tmp[2*i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=mi2_tmp[2*i+k+1]*mix2.window[j];
        baseb_raw_orthog[2*p0  ]=mi2_tmp[2*i+k+2]*mix2.window[j];
        baseb_raw_orthog[2*p0+1]=mi2_tmp[2*i+k+3]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        j++;
        } 
      j--;  
      ic=2*mix2.new_points;  
      for(i=ib; i<ic; i+=2)
        {
        j--;
        baseb_raw[2*p0  ]=mi2_tmp[2*i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=mi2_tmp[2*i+k+1]*mix2.window[j];
        baseb_raw_orthog[2*p0  ]=mi2_tmp[2*i+k+2]*mix2.window[j];
        baseb_raw_orthog[2*p0+1]=mi2_tmp[2*i+k+3]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        } 
      id=2*(mix2.crossover_points+mix2.new_points);
      for(i=ic; i<id; i+=2)
        {
        baseb_raw[2*p0  ]=mi2_tmp[2*i+k  ];
        baseb_raw[2*p0+1]=mi2_tmp[2*i+k+1];
        baseb_raw_orthog[2*p0  ]=mi2_tmp[2*i+k+2];
        baseb_raw_orthog[2*p0+1]=mi2_tmp[2*i+k+3];
        p0=(p0+1)&baseband_mask;
        }
      }
    }        
  if(bg.mixer_mode == 2)
    {
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebraw_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebraw_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebraw_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
    if(fft1_correlation_flag <= 1)
      {
      p0=baseb_pa;
      p0=(p0+1)&baseband_mask;
      for(k=1; k<=(int)mix2.new_points; k++)
        {
        pa=(timf3_py+4*(1-basebraw_fir_pts+fft3_size-fft3_new_points+k*resamp)
                                                    +timf3_size)&timf3_mask;
        mm=pa;
        t1=timf3_float[pa  ]*basebraw_fir[basebraw_fir_pts/2];
        t2=timf3_float[pa+1]*basebraw_fir[basebraw_fir_pts/2];
        t3=timf3_float[pa+2]*basebraw_fir[basebraw_fir_pts/2];
        t4=timf3_float[pa+3]*basebraw_fir[basebraw_fir_pts/2];
        for(i=basebraw_fir_pts/2-1; i>=0; i--)
          {
          pa=(pa+4)&timf3_mask;
          mm=(mm-4+timf3_size)&timf3_mask;
          t1+=(timf3_float[pa  ]+timf3_float[mm  ])*basebraw_fir[i];
          t2+=(timf3_float[pa+1]+timf3_float[mm+1])*basebraw_fir[i];
          t3+=(timf3_float[pa+2]+timf3_float[mm+2])*basebraw_fir[i];
          t4+=(timf3_float[pa+3]+timf3_float[mm+3])*basebraw_fir[i];
          }
        baseb_raw[2*p0  ]=pg.c1*t1+pg.c2*t3+pg.c3*t4;
        baseb_raw[2*p0+1]=pg.c1*t2+pg.c2*t4-pg.c3*t3;
        baseb_raw_orthog[2*p0  ]=pg.c1*t3-pg.c2*t1+pg.c3*t2;
        baseb_raw_orthog[2*p0+1]=pg.c1*t4-pg.c2*t2-pg.c3*t1;
        p0=(p0+1)&baseband_mask;
        }
      }
    else
      {
/* We do not need this, no audio in analyzer modes!
      p0=baseb_pa;
      p0=(p0+1)&baseband_mask;
      for(k=1; k<=(int)mix2.new_points; k++)
        {
        pa=(timf3_py+4*(1-basebraw_fir_pts+fft3_size-fft3_new_points+k*resamp)
                                                    +timf3_size)&timf3_mask;
        mm=pa;
        dt1=d_timf3_float[pa  ]*basebraw_fir[basebraw_fir_pts/2];
        dt2=d_timf3_float[pa+1]*basebraw_fir[basebraw_fir_pts/2];
        dt3=d_timf3_float[pa+2]*basebraw_fir[basebraw_fir_pts/2];
        dt4=d_timf3_float[pa+3]*basebraw_fir[basebraw_fir_pts/2];
        for(i=basebraw_fir_pts/2-1; i>=0; i--)
          {
          pa=(pa+4)&timf3_mask;
          mm=(mm-4+timf3_size)&timf3_mask;
          dt1+=(d_timf3_float[pa  ]+d_timf3_float[mm  ])*basebraw_fir[i];
          dt2+=(d_timf3_float[pa+1]+d_timf3_float[mm+1])*basebraw_fir[i];
          dt3+=(d_timf3_float[pa+2]+d_timf3_float[mm+2])*basebraw_fir[i];
          dt4+=(d_timf3_float[pa+3]+d_timf3_float[mm+3])*basebraw_fir[i];
          }
        baseb_raw[2*p0  ]=pg.c1*dt1+pg.c2*dt3+pg.c3*dt4;
        baseb_raw[2*p0+1]=pg.c1*dt2+pg.c2*dt4-pg.c3*dt3;
        baseb_raw_orthog[2*p0  ]=pg.c1*dt3-pg.c2*dt1+pg.c3*dt2;
        baseb_raw_orthog[2*p0+1]=pg.c1*dt4-pg.c2*dt2-pg.c3*dt1;
        p0=(p0+1)&baseband_mask;
        }
*/      }
    }
  }
if(kill_all_flag || thread_command_flag[THREAD_MIX2] != THRFLAG_ACTIVE)return;
if(fm_pilot_size == 0)
  {    
  if(bg.mixer_mode == 1)
    {
// Filter and decimate in the frequency domain.    
// We already stored the selected part of the spectrum in carr.
// take the back transform and construct the time function
// of baseb_carrier
    if(fft1_correlation_flag >= 2)
      {
      d_dual_fftback(mix2.size, mix2.n, d_carr_tmp,d_mix2_table,mix2.permute,
                                                  yieldflag_ndsp_mix2);
// Store baseb_raw and baseb_carrier.
// Compute baseb, the product of the carrier and the signal.
      if(genparm[THIRD_FFT_SINPOW] == 2)
        {
        for(i=0; i<sizhalf; i++)
          {
          d_baseb_carrier[4*baseb_pa+4*i  ]+=d_carr_tmp[4*i  ];
          d_baseb_carrier[4*baseb_pa+4*i+1]+=d_carr_tmp[4*i+1];
          d_baseb_carrier[4*baseb_pa+4*i+2]+=d_carr_tmp[4*i+2];
          d_baseb_carrier[4*baseb_pa+4*i+3]+=d_carr_tmp[4*i+3];
          }
        p0=(baseb_pa+sizhalf)&baseband_mask;        
        k=2*mix2.size;
        for(i=0; i<k; i++)
          {
          d_baseb_carrier[4*p0+i]=d_carr_tmp[i+k];
          }
        }
      else
        {
        p0=baseb_pa;
        k=mix2.interleave_points-2*(mix2.crossover_points/2);
        j=k/2+mix2.crossover_points;
        k*=2;
        ia=2*mix2.crossover_points;
        for(i=0; i<ia; i+=2)
          {
          d_baseb_carrier[4*p0  ]=d_baseb_carrier[4*p0  ]*mix2.cos2win[i>>1]+
                                        d_carr_tmp[2*i+k  ]*mix2.sin2win[i>>1];
          d_baseb_carrier[4*p0+1]=d_baseb_carrier[4*p0+1]*mix2.cos2win[i>>1]+
                                        d_carr_tmp[2*i+k+1]*mix2.sin2win[i>>1];
          d_baseb_carrier[4*p0+2]=d_baseb_carrier[4*p0+2]*mix2.cos2win[i>>1]+
                                        d_carr_tmp[2*i+k+2]*mix2.sin2win[i>>1];
          d_baseb_carrier[4*p0+3]=d_baseb_carrier[4*p0+3]*mix2.cos2win[i>>1]+
                                        d_carr_tmp[2*i+k+3]*mix2.sin2win[i>>1];
          p0=(p0+1)&baseband_mask;
          } 
        ib=mix2.new_points+2+2*(mix2.crossover_points/2);
        for(i=ia; i<ib; i+=2)
          {
          d_baseb_carrier[4*p0  ]=d_carr_tmp[2*i+k  ]*mix2.window[j];
          d_baseb_carrier[4*p0+1]=d_carr_tmp[2*i+k+1]*mix2.window[j];
          d_baseb_carrier[4*p0+2]=d_carr_tmp[2*i+k+2]*mix2.window[j];
          d_baseb_carrier[4*p0+3]=d_carr_tmp[2*i+k+3]*mix2.window[j];
          p0=(p0+1)&baseband_mask;
          j++;
          }
        j--;  
        ic=2*mix2.new_points;  
        for(i=ib; i<ic; i+=2)
          {
          j--;
          d_baseb_carrier[4*p0  ]=d_carr_tmp[2*i+k  ]*mix2.window[j];
          d_baseb_carrier[4*p0+1]=d_carr_tmp[2*i+k+1]*mix2.window[j];
          d_baseb_carrier[4*p0+2]=d_carr_tmp[2*i+k+2]*mix2.window[j];
          d_baseb_carrier[4*p0+3]=d_carr_tmp[2*i+k+3]*mix2.window[j];
          p0=(p0+1)&baseband_mask;
          } 
        id=2*(mix2.crossover_points+mix2.new_points);
        for(i=ic; i<id; i+=2)
          {
          d_baseb_carrier[4*p0  ]=d_carr_tmp[2*i+k  ];
          d_baseb_carrier[4*p0+1]=d_carr_tmp[2*i+k+1];
          d_baseb_carrier[4*p0+2]=d_carr_tmp[2*i+k+2];
          d_baseb_carrier[4*p0+3]=d_carr_tmp[2*i+k+3];
          p0=(p0+1)&baseband_mask;
          }
        }
      }
    else
      {
      fftback(mix2.size, mix2.n, carr_tmp,mix2.table,mix2.permute,
                                                  yieldflag_ndsp_mix2);
// Store baseb_raw and baseb_carrier.
// Compute baseb, the product of the carrier and the signal.
      if(genparm[THIRD_FFT_SINPOW] == 2)
        {
        for(i=0; i<sizhalf; i++)
          {
          baseb_carrier[2*baseb_pa+2*i  ]+=carr_tmp[2*i  ];
          baseb_carrier[2*baseb_pa+2*i+1]+=carr_tmp[2*i+1];
          }
        p0=(baseb_pa+sizhalf)&baseband_mask;        
        k=mix2.size;
          for(i=0; i<k; i++)
          {
          baseb_carrier[2*p0+i]=carr_tmp[i+k];
          }
        }
      else
        {
        p0=baseb_pa;
        k=mix2.interleave_points-2*(mix2.crossover_points/2);
        j=k/2+mix2.crossover_points;
        ia=2*mix2.crossover_points;
        for(i=0; i<ia; i+=2)
          {
          baseb_carrier[2*p0  ]=baseb_carrier[2*p0  ]*mix2.cos2win[i>>1]+carr_tmp[i+k  ]*mix2.sin2win[i>>1];
          baseb_carrier[2*p0+1]=baseb_carrier[2*p0+1]*mix2.cos2win[i>>1]+carr_tmp[i+k+1]*mix2.sin2win[i>>1];
          p0=(p0+1)&baseband_mask;
          } 
        ib=mix2.new_points+2+2*(mix2.crossover_points/2);
        for(i=ia; i<ib; i+=2)
          {
          baseb_carrier[2*p0  ]=carr_tmp[i+k  ]*mix2.window[j];
          baseb_carrier[2*p0+1]=carr_tmp[i+k+1]*mix2.window[j];
          p0=(p0+1)&baseband_mask;
          j++;
          } 
        j--;  
        ic=2*mix2.new_points;  
        for(i=ib; i<ic; i+=2)
          {
          j--;
          baseb_carrier[2*p0  ]=carr_tmp[i+k  ]*mix2.window[j];
          baseb_carrier[2*p0+1]=carr_tmp[i+k+1]*mix2.window[j];
          p0=(p0+1)&baseband_mask;
          } 
        id=2*(mix2.crossover_points+mix2.new_points);
        for(i=ic; i<id; i+=2)
          {
          baseb_carrier[2*p0  ]=carr_tmp[i+k  ];
          baseb_carrier[2*p0+1]=carr_tmp[i+k+1];
          p0=(p0+1)&baseband_mask;
          }
        }
      }
    }
  if(bg.mixer_mode == 2)
    {
// Filter and decimate in the time domain.    
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebcarr_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebcarr_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebcarr_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
    p0=baseb_pa;
    if(sw_onechan)
      {
      for(k=1; k<=(int)mix2.new_points; k++)
        {
        pa=(timf3_py+2*(1-basebcarr_fir_pts+fft3_size-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
        mm=pa;
        t1=timf3_float[pa  ]*basebcarr_fir[basebcarr_fir_pts/2];
        t2=timf3_float[pa+1]*basebcarr_fir[basebcarr_fir_pts/2];
        for(i=basebcarr_fir_pts/2-1; i>=0; i--)
          {
          pa=(pa+2)&timf3_mask;
          mm=(mm-2+timf3_size)&timf3_mask;
          t1+=(timf3_float[pa  ]+timf3_float[mm  ])*basebcarr_fir[i];
          t2+=(timf3_float[pa+1]+timf3_float[mm+1])*basebcarr_fir[i];
          }
        baseb_carrier[2*p0  ]=t1;
        baseb_carrier[2*p0+1]=t2;
        p0=(p0+1)&baseband_mask;
        }
      }
    else
      {
      if(fft1_correlation_flag >= 2)
        {
        for(k=1; k<=(int)mix2.new_points; k++)
          {
          pa=(timf3_py+4*(1-d_basebcarr_fir_pts+fft3_size-
                            fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
          mm=pa;
          dt1=d_timf3_float[pa  ]*d_basebcarr_fir[d_basebcarr_fir_pts/2];
          dt2=d_timf3_float[pa+1]*d_basebcarr_fir[d_basebcarr_fir_pts/2];
          dt3=d_timf3_float[pa+2]*d_basebcarr_fir[d_basebcarr_fir_pts/2];
          dt4=d_timf3_float[pa+3]*d_basebcarr_fir[d_basebcarr_fir_pts/2];
          for(i=d_basebcarr_fir_pts/2-1; i>=0; i--)
            {
            pa=(pa+4)&timf3_mask;
            mm=(mm-4+timf3_size)&timf3_mask;
            dt1+=(d_timf3_float[pa  ]+d_timf3_float[mm  ])*d_basebcarr_fir[i];
            dt2+=(d_timf3_float[pa+1]+d_timf3_float[mm+1])*d_basebcarr_fir[i];
            dt3+=(d_timf3_float[pa+2]+d_timf3_float[mm+2])*d_basebcarr_fir[i];
            dt4+=(d_timf3_float[pa+3]+d_timf3_float[mm+3])*d_basebcarr_fir[i];
            }
          d_baseb_carrier[4*p0  ]=dt1;
          d_baseb_carrier[4*p0+1]=dt2;
          d_baseb_carrier[4*p0+2]=dt3;
          d_baseb_carrier[4*p0+3]=dt4;
          p0=(p0+1)&baseband_mask;
          }
        }
      else
        {  
        for(k=1; k<=(int)mix2.new_points; k++)
          {
          pa=(timf3_py+4*(1-basebcarr_fir_pts+fft3_size-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
          mm=pa;
          t1=timf3_float[pa  ]*basebcarr_fir[basebcarr_fir_pts/2];
          t2=timf3_float[pa+1]*basebcarr_fir[basebcarr_fir_pts/2];
          t3=timf3_float[pa+2]*basebcarr_fir[basebcarr_fir_pts/2];
          t4=timf3_float[pa+3]*basebcarr_fir[basebcarr_fir_pts/2];
          for(i=basebcarr_fir_pts/2-1; i>=0; i--)
            {
            pa=(pa+4)&timf3_mask;
            mm=(mm-4+timf3_size)&timf3_mask;
            t1+=(timf3_float[pa  ]+timf3_float[mm  ])*basebcarr_fir[i];
            t2+=(timf3_float[pa+1]+timf3_float[mm+1])*basebcarr_fir[i];
            t3+=(timf3_float[pa+2]+timf3_float[mm+2])*basebcarr_fir[i];
            t4+=(timf3_float[pa+3]+timf3_float[mm+3])*basebcarr_fir[i];
            }
          baseb_carrier[2*p0  ]=pg.c1*t1+pg.c2*t3+pg.c3*t4;
          baseb_carrier[2*p0+1]=pg.c1*t2+pg.c2*t4-pg.c3*t3;
          p0=(p0+1)&baseband_mask;
          }
        }
      }  
    }        
  }
if(kill_all_flag || thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
if(genparm[CW_DECODE_ENABLE] != 0 && fft1_correlation_flag == 0)
  {
// We want cw decoding. Compute baseb_wb_raw, a signal with a bandwidth
// that is at least two times bigger than the bandwidth of the
// filtered signal. (see computation of baseband_sampling_speed in
// baseb_graph.c)
  if(sw_onechan)
    {
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[2*i  ]=fft3[fft3_px+fft3_size+2*i  ];    
      mi2_tmp[2*i+1]=fft3[fft3_px+fft3_size+2*i+1];    
      }
    nn=2*(mix2.size-1);
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[nn-2*i  ]=fft3[fft3_px+fft3_size-2*i-2];    
      mi2_tmp[nn-2*i+1]=fft3[fft3_px+fft3_size-2*i-1];    
      }
    }
  else
    {
// !!!!!!!!!!!!!! There are two rx channels. !!!!!!!!!!!!!!!!!!
// Select mix2.size points centered at fft3_size/2, and move them 
// to mi2_tmp. Transform the polarization at the same time.
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[2*i  ]=pg.c1*fft3[fft3_px+2*fft3_size+4*i  ]+
                      pg.c2*fft3[fft3_px+2*fft3_size+4*i+2]+
                      pg.c3*fft3[fft3_px+2*fft3_size+4*i+3];
      mi2_tmp[2*i+1]=pg.c1*fft3[fft3_px+2*fft3_size+4*i+1]+
                      pg.c2*fft3[fft3_px+2*fft3_size+4*i+3]-
                      pg.c3*fft3[fft3_px+2*fft3_size+4*i+2];
      }
    nn=2*(mix2.size-1);
    for(i=0; i<sizhalf; i++)
      {
      mi2_tmp[nn-2*i  ]=pg.c1*fft3[fft3_px+2*fft3_size-4*i-4]+
                         pg.c2*fft3[fft3_px+2*fft3_size-4*i-2]+
                         pg.c3*fft3[fft3_px+2*fft3_size-4*i-1];
      mi2_tmp[nn-2*i+1]=pg.c1*fft3[fft3_px+2*fft3_size-4*i-3]+
                         pg.c2*fft3[fft3_px+2*fft3_size-4*i-1]-
                         pg.c3*fft3[fft3_px+2*fft3_size-4*i-2];
      }
    }
  fftback(mix2.size, mix2.n, mi2_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// Store baseb_wb_raw
  if(genparm[THIRD_FFT_SINPOW] == 2)
    {
    for(i=0; i<sizhalf; i++)
      {
      baseb_wb_raw[2*baseb_pa+2*i  ]+=mi2_tmp[2*i  ];
      baseb_wb_raw[2*baseb_pa+2*i+1]+=mi2_tmp[2*i+1];
      }
    k=sizhalf;
    p0=(baseb_pa+sizhalf)&baseband_mask;
    for(i=0; i<sizhalf; i++)
      {
      baseb_wb_raw[2*p0+2*i  ]=mi2_tmp[2*(i+k)  ];
      baseb_wb_raw[2*p0+2*i+1]=mi2_tmp[2*(i+k)+1];
      }
    }
  else
    {
    p0=baseb_pa;
    k=mix2.interleave_points-2*(mix2.crossover_points/2);
    j=k/2+mix2.crossover_points;
    ia=2*mix2.crossover_points;
    for(i=0; i<ia; i+=2)
      {
      baseb_wb_raw[2*p0  ]=baseb_wb_raw[2*p0  ]*mix2.cos2win[i>>1]+mi2_tmp[i+k  ]*mix2.sin2win[i>>1];
      baseb_wb_raw[2*p0+1]=baseb_wb_raw[2*p0+1]*mix2.cos2win[i>>1]+mi2_tmp[i+k+1]*mix2.sin2win[i>>1];
      p0=(p0+1)&baseband_mask;
      } 
    ib=mix2.new_points+2+2*(mix2.crossover_points/2);
    for(i=ia; i<ib; i+=2)
      {
      baseb_wb_raw[2*p0  ]=mi2_tmp[i+k  ]*mix2.window[j];
      baseb_wb_raw[2*p0+1]=mi2_tmp[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      j++;
      } 
    j--;  
    ic=2*mix2.new_points;  
    for(i=ib; i<ic; i+=2)
      {
      j--;
      baseb_wb_raw[2*p0  ]=mi2_tmp[i+k  ]*mix2.window[j];
      baseb_wb_raw[2*p0+1]=mi2_tmp[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      } 
    id=2*(mix2.crossover_points+mix2.new_points);
    for(i=ic; i<id; i+=2)
      {
      baseb_wb_raw[2*p0  ]=mi2_tmp[i+k  ];
      baseb_wb_raw[2*p0+1]=mi2_tmp[i+k+1];
      p0=(p0+1)&baseband_mask;
      }
    }    
  }
last_point=(baseb_pa+mix2.new_points)&baseband_mask;
ampfac=cg_size*daout_gain/bg_amplimit;
// *******************************************************************
if(kill_all_flag || thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
switch (fft1_correlation_flag)
  {
  case 0:
  case 1:
// Calculate the amplitude of the signal and the amplitude of the carrier.
// Store sines and cosines of the carrier phase.
// Make an I/Q demodulator using the carrier phase.
// Use this "zero level" coherent detect data to make the upper coherent
// graph that will help the user select coherence ratio for unstable
// signals.
  if(fm_pilot_size == 0)
    {    
    t2=0;
    t5=0;
    cg_mid=cg_size/2;
    big=0.499*cg_size;
    ia=baseb_pa;
    i=(baseb_pa+baseband_mask)&baseband_mask;
    if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
      {
      t2=baseb_upthreshold[i];
      }
    if(bg.agc_flag == 2)
      {
      t2=baseb_upthreshold[2*i  ];
      t5=baseb_upthreshold[2*i+1];
      }
    t3=0;
    t4=0;
    if(bg_twopol == 0)
      {
      while(ia != last_point)
        {
        baseb_totpwr[ia]=baseb_raw[2*ia  ]*baseb_raw[2*ia  ]+
                         baseb_raw[2*ia+1]*baseb_raw[2*ia+1];
        t4+=baseb_totpwr[ia];
        if(t3 < baseb_totpwr[ia])t3=baseb_totpwr[ia];
        baseb_carrier_ampl[ia]=sqrt(baseb_carrier[2*ia  ]*baseb_carrier[2*ia  ]+
                                    baseb_carrier[2*ia+1]*baseb_carrier[2*ia+1]);
        if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
          {
          t2*=cg_decay_factor;
          if(t2 < baseb_totpwr[ia])t2=baseb_totpwr[ia];
          baseb_upthreshold[ia]=t2;
          }
        if(baseb_carrier_ampl[ia]<0.0000000000001)
          {
          baseb_carrier_ampl[ia]=0.0000000000001;
          }
        else
          {   
          baseb_carrier[2*ia  ]/=baseb_carrier_ampl[ia];
          baseb_carrier[2*ia+1]/=baseb_carrier_ampl[ia];
          }
        baseb[2*ia  ]=baseb_carrier[2*ia  ]*baseb_raw[2*ia  ]+
                      baseb_carrier[2*ia+1]*baseb_raw[2*ia+1];
        baseb[2*ia+1]=baseb_carrier[2*ia  ]*baseb_raw[2*ia+1]-
                      baseb_carrier[2*ia+1]*baseb_raw[2*ia  ];
        if(bg.agc_flag == 2)
          {
          baseb[2*ia  ]-=baseb_carrier_ampl[ia];
          t2*=cg_decay_factor;
          if(t2 < baseb[2*ia  ]*baseb[2*ia  ])t2=baseb[2*ia  ]*baseb[2*ia  ];
          baseb_upthreshold[2*ia  ]=t2;
          t5*=cg_decay_factor;
          if(t5 < baseb[2*ia+1]*baseb[2*ia+1])t5=baseb[2*ia+1]*baseb[2*ia+1];
          baseb_upthreshold[2*ia+1]=t5;
          }
        r1=ampfac*baseb[2*ia  ];
        r2=ampfac*baseb[2*ia+1];
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          baseb_fit[2*ia  ]=0;
          baseb_fit[2*ia+1]=0;
          baseb_envelope[2*ia  ]=0;
          baseb_envelope[2*ia+1]=0;
          }
        ia=(ia+1)&baseband_mask;
        if(fabs(r1)>big || fabs(r2)>big)
          {
          t1=big/sqrt(r1*r1+r2*r2);
          r1*=t1;
          r2*=t1;
          }
        ix=cg_mid+r1;
        iy=cg_mid+r2;
        cg_map[iy*cg_size+ix]+=1;
        }
      }  
    else
      {
      while(ia != last_point)
        {
        baseb_totpwr[ia]=baseb_raw[2*ia  ]*baseb_raw[2*ia  ]+
                         baseb_raw[2*ia+1]*baseb_raw[2*ia+1];
        t4+=baseb_totpwr[ia];
        if(t3 < baseb_totpwr[ia])t3=baseb_totpwr[ia];
        baseb_carrier_ampl[ia]=sqrt(baseb_carrier[2*ia  ]*baseb_carrier[2*ia  ]+
                                    baseb_carrier[2*ia+1]*baseb_carrier[2*ia+1]);
        if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
          {
          t2*=cg_decay_factor;
          if(t2 < baseb_totpwr[ia])t2=baseb_totpwr[ia];
          baseb_upthreshold[ia]=t2;
          }
        if(baseb_carrier_ampl[ia]<0.0000000000001)
          {
          baseb_carrier_ampl[ia]=0.0000000000001;
          }
        else
          {   
          baseb_carrier[2*ia  ]/=baseb_carrier_ampl[ia];
          baseb_carrier[2*ia+1]/=baseb_carrier_ampl[ia];
          }
        baseb[2*ia  ]=baseb_carrier[2*ia  ]*baseb_raw[2*ia  ]+
                      baseb_carrier[2*ia+1]*baseb_raw[2*ia+1];
        baseb[2*ia+1]=baseb_carrier[2*ia  ]*baseb_raw[2*ia+1]-
                    baseb_carrier[2*ia+1]*baseb_raw[2*ia  ];
        if(bg.agc_flag == 2)
          {
          t2*=cg_decay_factor;
          if(t2 < baseb_totpwr[ia])t2=baseb_totpwr[ia];
          baseb_upthreshold[2*ia]=t2;
          t5*=cg_decay_factor;
          if(t5 < baseb_raw_orthog[2*ia  ]*baseb_raw_orthog[2*ia  ]+
                  baseb_raw_orthog[2*ia+1]*baseb_raw_orthog[2*ia+1])
          t5=baseb_raw_orthog[2*ia  ]*baseb_raw_orthog[2*ia  ]+
             baseb_raw_orthog[2*ia+1]*baseb_raw_orthog[2*ia+1];
          baseb_upthreshold[2*ia+1]=t5;
          }
        r1=ampfac*baseb[2*ia  ];
        r2=ampfac*baseb[2*ia+1];
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          baseb_fit[2*ia  ]=0;
          baseb_fit[2*ia+1]=0;
          baseb_envelope[2*ia  ]=0;
          baseb_envelope[2*ia+1]=0;
          }
        ia=(ia+1)&baseband_mask;
        if(fabs(r1)>big || fabs(r2)>big)
          {
          t1=big/sqrt(r1*r1+r2*r2);
          r1*=t1;
          r2*=t1;
          }
        ix=cg_mid+r1;
        iy=cg_mid+r2;
        cg_map[iy*cg_size+ix]+=1;
        }
      }  
    }
  else
    {
    ia=baseb_pa;
    t3=0;
    t4=0;
    while(ia != last_point)
      {
      baseb_totpwr[ia]=baseb_raw[2*ia  ]*baseb_raw[2*ia  ]+
                       baseb_raw[2*ia+1]*baseb_raw[2*ia+1];
      t4+=baseb_totpwr[ia];
      if(t3 < baseb_totpwr[ia])t3=baseb_totpwr[ia];
      ia=(ia+1)&baseband_mask;
      }
    }
  basblock_maxpower[basblock_pa]=t3;
  basblock_avgpower[basblock_pa]=t4/sizhalf;
  basblock_pa=(basblock_pa+1)&basblock_mask;
  break;  
  
  case 2:
  ia=baseb_pa;
  p0=timf3_pc;
  while(ia != last_point)
    {
    dt1=sqrt(d_baseb_carrier[4*ia  ]*d_baseb_carrier[4*ia  ]+
             d_baseb_carrier[4*ia+1]*d_baseb_carrier[4*ia+1])/P2SCALE;
    dt2=sqrt(d_baseb_carrier[4*ia+2]*d_baseb_carrier[4*ia+2]+
             d_baseb_carrier[4*ia+3]*d_baseb_carrier[4*ia+3])/P2SCALE;
    if(dt1 < 1.E-20)
      {
      dt1=1.E-20;
      }
    if(dt2 < 1.E-20)
      {
      dt2=1.E-20;
      }
    d_baseb[4*ia  ]=(d_baseb_carrier[4*ia  ]*d_timf3_float[p0  ]+
                     d_baseb_carrier[4*ia+1]*d_timf3_float[p0+1])/dt1;
    d_baseb[4*ia+1]=(d_baseb_carrier[4*ia  ]*d_timf3_float[p0+1]-
                     d_baseb_carrier[4*ia+1]*d_timf3_float[p0  ])/dt1;
    d_baseb[4*ia+2]=(d_baseb_carrier[4*ia+2]*d_timf3_float[p0+2]+
                     d_baseb_carrier[4*ia+3]*d_timf3_float[p0+3])/dt2;
    d_baseb[4*ia+3]=(d_baseb_carrier[4*ia+2]*d_timf3_float[p0+3]-
                     d_baseb_carrier[4*ia+3]*d_timf3_float[p0+2])/dt2;
    p0=(p0+4)&timf3_mask;
    ia=(ia+1)&baseband_mask;
    }
  timf3_pc=(timf3_pc+2*mix2.new_points*ui.rx_rf_channels)&timf3_mask;
  break;
  
  case 3:
  timf3_pc=(timf3_pc+2*mix2.new_points*ui.rx_rf_channels)&timf3_mask;
  break;
  }
if(kill_all_flag || thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
// **********************************************************
// If morse decoding is enabled, compute the fourier transform
// of the real part of baseb_wb. Since this array is not needed
// for any other purpose, we do not store it so its real part
// is computed from baseb_carrier and baseb_wb_raw.
// The phase is zero in good
// regions where the carrier is correct. Use the level of the
// carrier as a weight factor to suppress regions where
// the signal is likely to be absent or incorrect.
if(genparm[CW_DECODE_ENABLE] != 0 && fft1_correlation_flag == 0)
  {
  if(fft3_slowsum_cnt < 1)
    {
    lir_sched_yield();
    if(mix1_selfreq[0]>0)lirerr(341296);
    return;
    }
  noise_floor*=2.818/fft3_slowsum_cnt;
  baseband_noise_level=noise_floor*bgfil_weight;
  carrier_noise_level=noise_floor*carrfil_weight;
  ia=(last_point-mix2.size+baseband_mask)&baseband_mask;
  if( ((ia-baseb_px+baseband_size)&baseband_mask) < baseband_neg)
    {
    for(i=0; i<(int)mix2.size; i++)
      {
      t1=P2SCALE*baseb_carrier_ampl[ia]*baseb_carrier_ampl[ia];
      if(t1 > 0)
        {
        mix2_tmp[2*i]=P4SCALE*(baseb_carrier[2*ia  ]*baseb_wb_raw[2*ia  ]+
                               baseb_carrier[2*ia+1]*baseb_wb_raw[2*ia+1])*
                               sqrt(t1)*cw_carrier_window[i];
        }
      else
        {
        mix2_tmp[2*i  ]=0;
        }
      mix2_tmp[2*i+1]=0;
      ia=(ia+1)&baseband_mask;
      }
    fftforward(mix2.size, mix2.n, mix2_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// The fourier transform here should have been computed
// with a real to hermitian fft implementation. Just filling
// the complex part with zeroes is inefficient.
//
// The keying spectra arrive at a rate of 2*baseband_sampling_speed/mix2.size.
// Accumulate power spectra in a leaky integrator that has a life time
// of 15 seconds.
    t2=1-keying_spectrum_f1;
    keying_spectrum_cnt++;
    for(i=0; i<keying_spectrum_size; i++)
      {
      keying_spectrum[i]=keying_spectrum_f1*keying_spectrum[i]+
                 t2*(pow(mix2_tmp[2*i  ],2.0)+pow(mix2_tmp[2*i+1],2.0));
      }
    sc[SC_SHOW_KEYING_SPECTRUM]++;
    }
  }
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if(fft1_correlation_flag <= 1)
  {
  cg_update_count++;
  if(cg_update_count > cg_update_interval)
    {
    sc[SC_SHOW_COHERENT]++;
    cg_update_count=0;
    }  
// Make data for the S-meter graph.
  if(mg.avgnum > 0 && new_baseb_flag == 0)
    {
    j=((last_point-baseb_pm+baseband_mask)&baseband_mask)/mg.avgnum;
    if(j > 0)
      {
      sellim_correction=1;  
      if(genparm[SECOND_FFT_ENABLE] != 0)
        {
        if(!swfloat && mix1_selfreq[0]>=0)
          {
          ia=mix1_selfreq[0]*fftx_points_per_hz;
          ia/=fft2_to_fft1_ratio;
          if(liminfo[ia]>0)
            {
            sellim_correction=1/pow(liminfo[ia],2.0);
            }
          }
        }
      fac=baseband_pwrfac*sellim_correction;
      if(timf2_blockpower_px == -1)
        {
        i=(int)((float)fft3_size*timf3_sampling_speed/timf1_sampling_speed);
        timf2_blockpower_px=(timf2_blockpower_pa-4*(mg.avgnum+i)/3+
                                   timf2_blockpower_size)&timf2_blockpower_mask;
        }
      i=(timf2_blockpower_pa-timf2_blockpower_px+
                                   timf2_blockpower_size)&timf2_blockpower_mask;
      if(i <= j*mg.avgnum)
        {
        i=j*mg.avgnum+1;
        timf2_blockpower_px=(timf2_blockpower_pa-i+
                                   timf2_blockpower_size)&timf2_blockpower_mask;
        }
      for(i=0; i<j; i++)
        {
        dt1=0.000000001;
        dt2=0.000000001;
        dr3=0.000000001;
        dr4=0.000000001;
        dt3=0.000000001;
        dt4=0.000000001;
        ia=(baseb_pm+mg.avgnum)&baseband_mask;
        if(sw_onechan)
          {
          if(mg.scale_type == MG_SCALE_STON)
            {
            while(baseb_pm != ia)
              {
              dt1+=baseb_totpwr[baseb_pm];
              if(dt2 < baseb_totpwr[baseb_pm])dt2=baseb_totpwr[baseb_pm];
              baseb_pm=(baseb_pm+1)&baseband_mask;
              dr3+=(double)timf2_blockpower[timf2_blockpower_px];
              timf2_blockpower_px=(timf2_blockpower_px+1)&timf2_blockpower_mask;
              }
            mg_peak_meter[mg_pa]=log10(fac*dt1/mg.avgnum)+mg.cal_ston_sigshift/10;
            dr3*=liminfo_amplitude_factor;
            dt1/=dr3;
            dt1*=(double)(20000.)*mg.avgnum*timf2_blockpower_block;
            dt1/=(double)fft2_to_fft1_ratio*(double)fft2_to_fft1_ratio;
// Compensating for the percentage of cleared points may be a bad idea.
// The dumb blanker will not remove the entire pulse and a compensation
// might make things worse.
//          dt1*=(1-0.01*stupid_blanker_rate);
            }
          else
            {
            while(baseb_pm != ia)
              {
              dt1+=baseb_totpwr[baseb_pm];
              if(dt2 < baseb_totpwr[baseb_pm])dt2=baseb_totpwr[baseb_pm];
              baseb_pm=(baseb_pm+1)&baseband_mask;
              }
            mg_peak_meter[mg_pa]=log10(fac*dt2);
            }
          mg_rms_meter[mg_pa]=log10(fac*dt1/mg.avgnum);      
          }
        else
          {  
          if(mg.scale_type == MG_SCALE_STON)
            {
            while(baseb_pm != ia)
              {
              dt1+=baseb_totpwr[baseb_pm];
              if(dt2 < baseb_totpwr[baseb_pm])dt2=baseb_totpwr[baseb_pm];
              r1=baseb_raw_orthog[2*baseb_pm  ]*baseb_raw_orthog[2*baseb_pm  ]+
                 baseb_raw_orthog[2*baseb_pm+1]*baseb_raw_orthog[2*baseb_pm+1];
              dt3+=r1;
              if(dt4 < r1)dt4=r1;
              baseb_pm=(baseb_pm+1)&baseband_mask;
              dr3+=(double)timf2_blockpower[2*timf2_blockpower_px  ];
              dr4+=(double)timf2_blockpower[2*timf2_blockpower_px+1];
              timf2_blockpower_px=(timf2_blockpower_px+1)&timf2_blockpower_mask;
              }
            mg_peak_meter[2*mg_pa  ]=log10(fac*dt1/mg.avgnum)+mg.cal_ston_sigshift/10;
            mg_peak_meter[2*mg_pa+1]=log10(fac*dt3/mg.avgnum)+mg.cal_ston_sigshift/10;
            dr3*=liminfo_amplitude_factor;
            dr4*=liminfo_amplitude_factor;
            dt1/=dr3;
            dt3/=dr4;
            dt1*=(double)(20000.)*mg.avgnum*timf2_blockpower_block;
            dt3*=(double)(20000.)*mg.avgnum*timf2_blockpower_block;
            dt1/=(double)fft2_to_fft1_ratio*(double)fft2_to_fft1_ratio;
            dt3/=(double)fft2_to_fft1_ratio*(double)fft2_to_fft1_ratio;
            }
          else
            {
            r1=0;
            while(baseb_pm != ia)
              {
              dt1+=baseb_totpwr[baseb_pm];
              if(dt2 < baseb_totpwr[baseb_pm])dt2=baseb_totpwr[baseb_pm];
              r1=baseb_raw_orthog[2*baseb_pm  ]*baseb_raw_orthog[2*baseb_pm  ]+
                 baseb_raw_orthog[2*baseb_pm+1]*baseb_raw_orthog[2*baseb_pm+1];
              dt3+=r1;
              if(dt4 < r1)dt4=r1;
              baseb_pm=(baseb_pm+1)&baseband_mask;
              }
            mg_peak_meter[2*mg_pa  ]=log10(fac*dt2);      
            mg_peak_meter[2*mg_pa+1]=log10(fac*dt4);      
            }          
          mg_rms_meter[2*mg_pa  ]=log10(fac*dt1/mg.avgnum);      
          mg_rms_meter[2*mg_pa+1]=log10(fac*dt3/mg.avgnum);
          }
        mg_pa=(mg_pa+1)&mg_mask;
        mg_valid++;    
        }
      if(cg.meter_graph_on != 0)
        {
        if(recent_time-update_meter_time > 0.1)
          {
          sc[SC_UPDATE_METER_GRAPH]++;
          update_meter_time=recent_time;
          }
        }
      }
    if(mg_valid > mg_size)mg_valid=mg_size;  
    }
  }
// baseb_upthreshold is a fast attack, slow release signal follower 
// to baseb_totpwr. Make a fast attack, slow release signal
// follower in the reverse direction and store the largest of the 
// forward and backward peak detector -6dB in baseb_threshold.
ia=last_point;
ib=(baseb_pa+baseband_mask)&baseband_mask;
t2=0;
t5=0;
k=2;
ic=baseb_pb;
while(k!=0 && ia!=ic)
  {
  if(ia == ib)
    {
    k=1;
    }
  else
    {
    k|=1;
    }
  ia=(ia+baseband_mask)&baseband_mask;
  if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
    {
    t2*=cg_decay_factor;
    if(t2 < baseb_totpwr[ia])
      {
      t2=baseb_totpwr[ia];
      }
    t1=t2;
    if(t1<baseb_upthreshold[ia])
      {
      t1=baseb_upthreshold[ia];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[ia]=0.25*t1;
    }
  if(bg.agc_flag == 2)
    {
    t2*=cg_decay_factor;
    if(t2 < baseb[2*ia]*baseb[2*ia])
      {
      t2=baseb[2*ia]*baseb[2*ia];
      }
    t1=t2;
    if(t1<baseb_upthreshold[2*ia])
      {
      t1=baseb_upthreshold[2*ia];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[2*ia]=0.25*t1;
    t5*=cg_decay_factor;
    if(t5 < baseb[2*ia+1]*baseb[2*ia+1])
      {
      t5=baseb[2*ia+1]*baseb[2*ia+1];
      }
    t1=t5;
    if(t1<baseb_upthreshold[2*ia+1])
      {
      t1=baseb_upthreshold[2*ia+1];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[2*ia+1]=0.25*t1;
    }
  }
if(bg.agc_flag == 1)
  {
  latest_hold_pos1=0;
  latest_hold_max1=0;
  if(bg.agc_hang != 0)
    {
    j=ia;
    for(k=0; k<bg_agc_hang_pts; k++)
      {
      j=(j+baseband_mask)&baseband_mask;
      if(latest_hold_max1<baseb_agc_det[j])
        {
        latest_hold_max1=baseb_agc_det[j];
        latest_hold_pos1=k;
        }
      }
    }     
  while(ia != last_point)
    {
    t1=baseb_threshold[ia];
    rx_agc_fastsum1=rx_agc_fast_factor1*rx_agc_fastsum1+
                                                  t1*rx_agc_fast_factor2;
    if(t1 > 4*rx_agc_factor1)t1=4*rx_agc_factor1;
    rx_agc_sumpow1=agc_attack_factor1*rx_agc_sumpow1+t1*agc_attack_factor2;
    if(rx_agc_sumpow1 >rx_agc_fastsum1)rx_agc_sumpow1=rx_agc_fastsum1;
    rx_agc_sumpow2=agc_attack_factor1*rx_agc_sumpow2+
                                 agc_attack_factor2*rx_agc_sumpow1;
    if(rx_agc_sumpow2 >rx_agc_fastsum1)rx_agc_sumpow2=rx_agc_fastsum1;
    rx_agc_factor1*=agc_release_factor;
    if(rx_agc_factor1 < rx_agc_sumpow2)
      {
      rx_agc_factor1=rx_agc_sumpow2;
      }
    t1=rx_agc_factor1;
    baseb_agc_det[ia]=t1;
    if(bg.agc_hang != 0)
      {
      if(t1 > latest_hold_max1)
        {
        latest_hold_max1=t1;
        latest_hold_pos1=0;
        }
      latest_hold_pos1++;
      if(latest_hold_pos1 > bg_agc_hang_pts)
        {
        j=ia;
        for(k=0; k<bg_agc_hang_pts; k++)
          {
          j=(j+baseband_mask)&baseband_mask;
          if(latest_hold_max1<baseb_agc_det[j])
            {
            latest_hold_max1=baseb_agc_det[j];
            latest_hold_pos1=k;
            }
          }
        }
      if(t1<latest_hold_max1)
        {
        t1=latest_hold_max1;
        }
      }  
// Prevent clipping.
    if(t1 < 0.5*rx_agc_fastsum1)t1=0.5*rx_agc_fastsum1;
    if(t1 < 0.3*baseb_threshold[ia])t1=0.3*baseb_threshold[ia];  
    baseb_agc_level[ia]=sqrt(t1);
    ia=(ia+1)&baseband_mask;
    }
  }
if(bg.agc_flag == 2)
  {
  latest_hold_pos1=0;
  latest_hold_max1=0;
  latest_hold_pos2=0;
  latest_hold_max2=0;
  if(bg.agc_hang != 0)
    {
    j=ia;
    for(k=0; k<bg_agc_hang_pts; k++)
      {
      j=(j+baseband_mask)&baseband_mask;
      if(latest_hold_max1<baseb_agc_det[2*j])
        {
        latest_hold_max1=baseb_agc_det[2*j];
        latest_hold_pos1=k;
        }
      if(latest_hold_max2<baseb_agc_det[2*j+1])
        {
        latest_hold_max2=baseb_agc_det[2*j+1];
        latest_hold_pos2=k;
        }
      }
    }     
  while(ia != last_point)
    {
    t1=baseb_threshold[2*ia];
    t2=baseb_threshold[2*ia+1];
    rx_agc_fastsum1=rx_agc_fast_factor1*rx_agc_fastsum1+
                                                  t1*rx_agc_fast_factor2;
    rx_agc_fastsum2=rx_agc_fast_factor1*rx_agc_fastsum2+
                                                  t2*rx_agc_fast_factor2;
    if(t1 > 4*rx_agc_factor1)t1=4*rx_agc_factor1;
    if(t2 > 4*rx_agc_factor2)t2=4*rx_agc_factor2;
    rx_agc_sumpow1=agc_attack_factor1*rx_agc_sumpow1+t1*agc_attack_factor2;
    rx_agc_sumpow3=agc_attack_factor1*rx_agc_sumpow3+t2*agc_attack_factor2;
    if(rx_agc_sumpow1 >rx_agc_fastsum1)rx_agc_sumpow1=rx_agc_fastsum1;
    if(rx_agc_sumpow3 >rx_agc_fastsum2)rx_agc_sumpow3=rx_agc_fastsum2;
    rx_agc_sumpow2=agc_attack_factor1*rx_agc_sumpow2+
                                 agc_attack_factor2*rx_agc_sumpow1;
    rx_agc_sumpow4=agc_attack_factor1*rx_agc_sumpow4+
                                 agc_attack_factor2*rx_agc_sumpow2;
    if(rx_agc_sumpow2 >rx_agc_fastsum1)rx_agc_sumpow2=rx_agc_fastsum1;
    if(rx_agc_sumpow4 >rx_agc_fastsum2)rx_agc_sumpow4=rx_agc_fastsum2;
    rx_agc_factor1*=agc_release_factor;
    rx_agc_factor2*=agc_release_factor;
    if(rx_agc_factor1 < rx_agc_sumpow2)
      {
      rx_agc_factor1=rx_agc_sumpow2;
      }
    if(rx_agc_factor2 < rx_agc_sumpow4)
      {
      rx_agc_factor2=rx_agc_sumpow4;
      }
    t1=rx_agc_factor1;
    t2=rx_agc_factor2;
    baseb_agc_det[2*ia]=t1;
    baseb_agc_det[2*ia+1]=t2;
    if(bg.agc_hang != 0)
      {
      if(t1 > latest_hold_max1)
        {
        latest_hold_max1=t1;
        latest_hold_pos1=0;
        }
      if(t2 > latest_hold_max2)
        {
        latest_hold_max2=t2;
        latest_hold_pos2=0;
        }
      latest_hold_pos1++;
      latest_hold_pos2++;
      if(latest_hold_pos1 > bg_agc_hang_pts)
        {
        j=ia;
        for(k=0; k<bg_agc_hang_pts; k++)
          {
          j=(j+baseband_mask)&baseband_mask;
          if(latest_hold_max1<baseb_agc_det[2*j])
            {
            latest_hold_max1=baseb_agc_det[2*j];
            latest_hold_pos1=k;
            }
          }
        }
      if(latest_hold_pos2 > bg_agc_hang_pts)
        {
        j=ia;
        for(k=0; k<bg_agc_hang_pts; k++)
          {
          j=(j+baseband_mask)&baseband_mask;
          if(latest_hold_max2<baseb_agc_det[2*j+1])
            {
            latest_hold_max2=baseb_agc_det[2*j+1];
            latest_hold_pos2=k;
            }
          }
        }
      if(t1<latest_hold_max1)
        {
        t1=latest_hold_max1;
        }
      if(t2<latest_hold_max2)
        {
        t2=latest_hold_max2;
        }
      }  
// Prevent clipping.
    if(t1 < 0.5*rx_agc_fastsum1)t1=0.5*rx_agc_fastsum1;
    if(t2 < 0.5*rx_agc_fastsum2)t2=0.5*rx_agc_fastsum2;
    if(t1 < 0.3*baseb_threshold[2*ia])t1=0.3*baseb_threshold[2*ia];  
    if(t2 < 0.3*baseb_threshold[2*ia+1])t2=0.3*baseb_threshold[2*ia+1];  
    baseb_agc_level[2*ia]=sqrt(t1);
    baseb_agc_level[2*ia+1]=sqrt(t2);
    ia=(ia+1)&baseband_mask;
    }
  }
// Put whatever output the operator asked for in baseb_out    
p0=baseb_pa;
if(bg_delay != 0)
  {
// Synthetic stereo from mono by time delay of one channel is selected.  
// just send data direct and delayed. Do not worry about orthog
// if we have two channels.
  pb=(baseb_pa+bg.delay_points)&baseband_mask;
  while(p0 != last_point)
    {
    baseb_out[4*pb  ]=baseb_raw[2*p0];
    baseb_out[4*p0+2]=baseb_raw[2*p0];     
    baseb_out[4*pb+1]=baseb_raw[2*p0+1];
    baseb_out[4*p0+3]=baseb_raw[2*p0+1];
    pb=(pb+1)&baseband_mask;
    p0=(p0+1)&baseband_mask;
    }
  }
else  
  {
  switch (bg_coherent)
    {     
    case 0:
// No fancy processing at all. 
// Just store signal as it is or with a detector to fit the current mode.
    if(use_bfo != 0)
      {
      if(sw_onechan || bg_twopol == 0)
        {
        while(p0 != last_point)
          {
          baseb_out[2*p0  ]=baseb_raw[2*p0  ];     
          baseb_out[2*p0+1]=baseb_raw[2*p0+1];     
          p0=(p0+1)&baseband_mask;
          }
        }  
      else    
        {
        while(p0 != last_point)
          {
          baseb_out[4*p0  ]=baseb_raw[2*p0  ];     
          baseb_out[4*p0+1]=baseb_raw[2*p0+1];     
          baseb_out[4*p0+2]=baseb_raw_orthog[2*p0  ];     
          baseb_out[4*p0+3]=baseb_raw_orthog[2*p0+1];     
          p0=(p0+1)&baseband_mask;
          }
        }
      }
    else
      {  
      if(rx_mode == MODE_AM)
        {
// The user selected AM. Make amplitude detection.
        if(sw_onechan || bg_twopol == 0)
          {
          while(p0 != last_point)
            {
            t1=sqrt(baseb_totpwr[p0]);
            am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[2*p0  ]=t1-am_dclevel1;     
            baseb_out[2*p0+1]=baseb_out[2*p0  ];     
            p0=(p0+1)&baseband_mask;
            }
          }  
        else    
          {
          while(p0 != last_point)
            {
            t1=sqrt(baseb_totpwr[p0]);
            am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[4*p0  ]=t1-am_dclevel1;     
            baseb_out[4*p0+1]=0;     
            t1=sqrt(baseb_raw_orthog[2*p0  ]*baseb_raw_orthog[2*p0  ]+
                    baseb_raw_orthog[2*p0+1]*baseb_raw_orthog[2*p0+1]);
            am_dclevel2=am_dclevel2*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[4*p0+2]=t1-am_dclevel2;     
            baseb_out[4*p0+3]=0;     
            p0=(p0+1)&baseband_mask;
            }
          }
        }
      else
        {
// The user selected FM.
        detect_fm();
        }  
      }
    break;       

    case 1:
// Send the signal to one ear, the carrier to the other.
    if(use_bfo != 0)
      {
      while(p0 != last_point)
        {
        baseb_out[4*p0  ]=baseb_raw[2*p0  ];     
        baseb_out[4*p0+1]=baseb_raw[2*p0+1];  
        baseb_out[4*p0+2]=baseb_carrier[2*p0  ]*baseb_carrier_ampl[p0];     
        baseb_out[4*p0+3]=baseb_carrier[2*p0+1]*baseb_carrier_ampl[p0];     
        p0=(p0+1)&baseband_mask;
        }
      }
    else
      {
      if(rx_mode == MODE_AM)
        {
// The user selected AM. Make amplitude detection.
// Here it is possible to listen to AM in two separate filters.
        while(p0 != last_point)
          {
          t1=sqrt(baseb_totpwr[p0]);
          am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
          baseb_out[4*p0  ]=t1-am_dclevel1;     
          baseb_out[4*p0+1]=0;     
          t1=sqrt(baseb_carrier_ampl[p0]);
          am_dclevel2=am_dclevel2*am_dclevel_factor1+t1*am_dclevel_factor2;
          baseb_out[4*p0+2]=t1-am_dclevel2;     
          baseb_out[4*p0+3]=0;     
          p0=(p0+1)&baseband_mask;
          }
        }
      else
        {
// The user selected FM. 
// Here it is possible to listen to FM in two separate filters.
        detect_fm();
        }  
      }    
    break;

    case 2:
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send I to one ear, Q to the other.
// This means that we send AM modulation to one ear, FM modulation to
// the other ear.
    if(rx_mode == MODE_AM || rx_mode == MODE_FM)
      {
      while(p0 != last_point)
        {
        am_dclevel1=am_dclevel1*
                             am_dclevel_factor1+baseb[2*p0  ]*am_dclevel_factor2;
        baseb_out[4*p0  ]=baseb[2*p0  ]-am_dclevel1;     
        baseb_out[4*p0+1]=0;     
        am_dclevel2=am_dclevel2*
                             am_dclevel_factor1+baseb[2*p0+1]*am_dclevel_factor2;
        baseb_out[4*p0+2]=baseb[2*p0+1];     
        baseb_out[4*p0+3]=0;     
        p0=(p0+1)&baseband_mask;
        }
      }  
    else
      {
      while(p0 != last_point)
        {
        baseb_out[4*p0  ]=baseb[2*p0  ];     
        baseb_out[4*p0+1]=0;     
        baseb_out[4*p0+2]=baseb[2*p0+1];     
        baseb_out[4*p0+3]=0;     
        p0=(p0+1)&baseband_mask;
        }
      }
    break;
       
    case 3:
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send I to both ears, but only if greater than zero. Skip Q.
    if(use_bfo != 0)
      {
      while(p0 != last_point)
        {
        if(baseb[2*p0] > 0)
          {
          baseb_out[2*p0]=baseb[2*p0];
          }     
        else 
          {
          baseb_out[2*p0]=0;
          }     
        baseb_out[2*p0+1]=baseb_out[2*p0];     
        p0=(p0+1)&baseband_mask;
        }
      }  
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send I to both ears, Remove thr DC component Skip Q.
    else
      {
      while(p0 != last_point)
        {
        baseb_out[2*p0]=baseb[2*p0];
        am_dclevel1=am_dclevel1*am_dclevel_factor1+baseb_out[2*p0]*am_dclevel_factor2;
        baseb_out[2*p0  ]-=am_dclevel1;     
        baseb_out[2*p0+1]=baseb_out[2*p0];     
        p0=(p0+1)&baseband_mask;
        }
      }
    break;
       
    case 4:
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send Q to both ears, but only if greater than zero. Skip I.
    if(use_bfo != 0)
      {
      while(p0 != last_point)
        {
        if(baseb[2*p0] > 0)
          {
          baseb_out[2*p0]=baseb[2*p0];
          }     
        else 
          {
          baseb_out[2*p0]=0;
          }     
        baseb_out[2*p0+1]=baseb_out[2*p0];     
        p0=(p0+1)&baseband_mask;
        }
      }  
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send Q to both ears, remove the DC component. Skip I.
    else
      {
      while(p0 != last_point)
        {
        baseb_out[2*p0]=baseb[2*p0+1];
        am_dclevel1=am_dclevel1*am_dclevel_factor1+baseb_out[2*p0]*am_dclevel_factor2;
        baseb_out[2*p0  ]-=am_dclevel1;     
        baseb_out[2*p0+1]=baseb_out[2*p0];     
        p0=(p0+1)&baseband_mask;
        }
      }
    break;
       
    default:
    lirerr(82615);
    }
  }
if(thread_command_flag[THREAD_MIX2] != THRFLAG_ACTIVE)return;
if(fft1_correlation_flag == 2)
  {
// Baseb contains coh-detected amplitude and phase modulation.
// Four arrays of size mix2.new_points.
// Wait until we have fft3_size 
// last_point=(baseb_pa+mix2.new_points)&baseband_mask;
  if(correlation_reset_flag != fft1corr_reset_flag)
    {
    make_siganal_graph(TRUE,TRUE);
    correlation_reset_flag=fft1corr_reset_flag; 
    goto skip;
    }
  while(((baseb_pa-baseb_px+baseband_size)&baseband_mask) > sg_siz)
    {
    if(skip_siganal != 0)
      {
      baseb_px=(baseb_px+sg_interleave_points)&baseband_mask;
      skip_siganal--;
      }
    else
      {  
      do_siganal();
      }
    if(kill_all_flag || 
              thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
    }
  }
if(fft1_correlation_flag == 3)
  {
// d_baseb_carrier contains a signal with frequency near zero.
  if(correlation_reset_flag != fft1corr_reset_flag)
    {
    make_allan_graph(TRUE,TRUE);
    make_allanfreq_graph(TRUE,TRUE);
    correlation_reset_flag=fft1corr_reset_flag; 
    goto skip;
    }
  while(((baseb_pa-baseb_px+baseband_size)&baseband_mask) > vg_siz)
    {
    do_allan();
    if(kill_all_flag || 
                   thread_command_flag[THREAD_MIX2]!=THRFLAG_ACTIVE)return;
    }
  }
skip:;
#if NET_BASEBRAW_MODE == BASEBAND_IQ_MASTER_TEST
p0=baseb_pa;
for(k=0; k<(int)mix2.new_points; k++)
  {
  baseb_raw[2*p0  ]=basebraw_test_cnt1;
  baseb_raw[2*p0+1]=basebraw_test_cnt1+256;
  baseb_raw_orthog[2*p0  ]=basebraw_test_cnt1+512;
  baseb_raw_orthog[2*p0+1]=basebraw_test_cnt1+768;
  basebraw_test_cnt1+=1024;
  p0=(p0+1)&baseband_mask;
  } 
#endif
// baseb_threshold is inaccurate since the backwards fast attack
// slow release peak detector can not have the decay corresponding
// to what will happen after baseb_pa.
// Set baseb_pb to point at a distance where the next key down
// has decayed backwards. 
baseb_pa=last_point;
baseb_pb=(baseb_pa-(int)(20*cg_code_unit)+baseband_size)&baseband_mask;
fft3_px=(fft3_px+fft3_block)&fft3_mask;
timf3_py=(timf3_py+twice_rxchan*fft3_new_points)&timf3_mask;
if(fft1_correlation_flag > 1)
  {
  baseb_py=baseb_px;
  return;
  }
if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) >= baseb_output_block)
  {
  lir_set_event(EVENT_BASEB);
  }
}
