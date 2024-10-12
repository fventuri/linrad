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
#include "sigdef.h"
#include "screendef.h"
#include "options.h"

// float[2] baseb_out=data to send to loudspeaker
// float[2] baseb=complex amplitude of first level coherent data.
// float[2] baseb_raw=complex amplitude of baseband signal. Raw data, pol. adapted.
// float[2] baseb_raw_orthog=complex amplitude of pol. orthog signal. Raw data.
// float[2] baseb_carrier=phase of carrier. Complex data, cosines and sines.
// float[1] baseb_carrier_ampl=amplitude of carrier
// float[1] baseb_totpwr=total power of baseb_raw
// float[2] baseb_envelope=complex amplitude from fitted dots and dashes.
// float[1] baseb_upthreshold=forward peak detector for power
// float[1] baseb_threshold=combined forward and backward peak detector at -6dB
// float[2] baseb_fit=fitted dots and dashes.
// float[2] baseb_tmp=array for intermediate data in complex format
// float[1] baseb_agc_level=used only when AGC is enabled.
// float[2] baseb_wb_raw=complex amplitude like baseb_raw at a larger bandwidth.
// float[2] baseb_wb=complex amplitude of coherent detect like baseb, at larger bw.
// short int[1] baseb_ramp=indicator for time of power above/below threshold.
// short_int[1] baseb_clock=CW code clock
// float[2] baseb_tmp=for debug during development

// baseb_pa  The starting point of the latest mix2.size/2 points.
// baseb_pb  The point up to which thresholds exist.
// baseb_pc  The point up to which cw speed statistics and ramp is collected.
// baseb_pd  A key up region close before baseb_pc
// baseb_pe  The point up to which we have run first detect. 
// baseb_pf
// baseb_px  The oldest point that contains valid data.


// ************************************************************
// ************************************************************
// These defines allow a lot of information to be written to dmp
#if DUMPFILE == TRUE
#define PR00 0
#define PR01 0
#define PR02 0
#define PR03 0
#define PR04 0
#define PR05 0
#else
#define PR00 0
#define PR01 0
#define PR02 0
#define PR03 0
#define PR04 0
#define PR05 0
#endif

#define PRT00 if(PR00 != 0)DEB
#define PRT01 if(PR01 != 0)DEB
#define PRT02 if(PR02 != 0)DEB
#define PRT03 if(PR03 != 0)DEB
#define PRT04 if(PR04 != 0)DEB
#define PRT05 if(PR05 != 0)DEB
// ************************************************************
// ************************************************************

#define ZZ 0.0000000001
#define MAX_CW_IDEAL_RISE 16
float cw_ideal_rise[MAX_CW_IDEAL_RISE];


void fit_dash(void)
{
int i,k,m,ss,pp,n1,n2,n3,trials;
float t1,t2,t3,r1,r2;
float im_1,im_2,im_3;
float re_1,re_2,re_3;
float p1,p2,p3,old_re,old_im;
float midpoint[4],offset[4],ampl[4];
int ref[4],shift[4];
// Compare the current waveform to the waveform of a dash.
// Set cg_wave_start to the position giving the best fit.
// Set cg_wave_midpoint to the center of the waveform.
// Return similarity of waveforms.
m=0;
trials=0;
old_re=0;
old_im=0;
try_fit:;
pp=cg_wave_start;
ref[trials]=pp;
ss=(cg_wave_start+dash_pts)&baseband_mask;
re_1=0;
re_2=0;
re_3=0;
im_1=0;
im_2=0;
im_3=0;
p2=0;
n2=pp;
k=dash_pts/12;
if(k<3)k=3;
n1=(pp-k+baseband_size)&baseband_mask;
n3=(pp+k)&baseband_mask;
i=0;
while(n2 != ss)
  {
  p2+=baseb_totpwr[n2];
  re_1+=baseb[2*n1  ]*dash_waveform[2*i  ]+
      baseb[2*n1+1]*dash_waveform[2*i+1];
  im_1+=baseb[2*n1  ]*dash_waveform[2*i+1]-
      baseb[2*n1+1]*dash_waveform[2*i  ];
  re_2+=baseb[2*n2  ]*dash_waveform[2*i  ]+
      baseb[2*n2+1]*dash_waveform[2*i+1];
  im_2+=baseb[2*n2  ]*dash_waveform[2*i+1]-
      baseb[2*n2+1]*dash_waveform[2*i  ];
  re_3+=baseb[2*n3  ]*dash_waveform[2*i  ]+
      baseb[2*n3+1]*dash_waveform[2*i+1];
  im_3+=baseb[2*n3  ]*dash_waveform[2*i+1]-
      baseb[2*n3+1]*dash_waveform[2*i  ];
  i++;
  n1=(n1+1)&baseband_mask;
  n2=(n2+1)&baseband_mask;
  n3=(n3+1)&baseband_mask;
  }
p1=p2;
p3=p2;
n1=(pp-k+baseband_size)&baseband_mask;
n3=(pp+k)&baseband_mask;
while(n1 != pp)
  {
  p1+=baseb_totpwr[n1];
  n1=(n1+1)&baseband_mask;
  }
while(n1 != n3)
  {
  p3-=baseb_totpwr[n1];
  n1=(n1+1)&baseband_mask;
  }
n1=(ss-k+baseband_size)&baseband_mask;
n3=(ss+k)&baseband_mask;
while(n1 != ss)
  {
  p1-=baseb_totpwr[n1];
  n1=(n1+1)&baseband_mask;
  }
while(n1 != n3)
  {
  p3+=baseb_totpwr[n1];
  n1=(n1+1)&baseband_mask;
  }
t1=sqrt((re_1*re_1+im_1*im_1)/(p1*dash_sumsq));
t2=sqrt((re_2*re_2+im_2*im_2)/(p2*dash_sumsq));
t3=sqrt((re_3*re_3+im_3*im_3)/(p3*dash_sumsq));
// t1,t2 and t3 reflect how well the current waveform fits to the reference
// when shifted by -k,0 and +k.
// These points should have a maximum close to t2.
if( (t1<0.33 && t2<0.33 && t3<0.33) )
  {
// If all points are really small, return zero to indicate an error.  
skip:;
  cg_wave_fit=0;
  return;
  }
parabolic_fit(&r2,&r1,t1,t2,t3);
r1*=k;
offset[trials]=r1;
ampl[trials]=r2;
cg_wave_midpoint=cg_wave_start+0.5*dash_pts+r1;
if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
midpoint[trials]=cg_wave_midpoint;
if(trials >= 3)goto skip;  
m=(fabs(r1)+0.5)*r1/fabs(r1);
if(m==0)goto pos_ok;
if(trials != 0)
  {
  if(m*shift[trials-1] <0)
    {
    m=(m-shift[trials-1])/2;
    }
  for(i=0; i<=trials; i++)
    {
    if(ref[i] == ((cg_wave_start+m)&baseband_mask) )m/=2;
    }
  if(abs(m) <= 1)goto pos_ok;
  }  
shift[trials]=m;
cg_wave_start=(cg_wave_start+m)&baseband_mask;
trials++;
old_re=re_2;
old_im=im_2;
goto try_fit;
pos_ok:;
cg_wave_coh_re=re_2;
cg_wave_coh_im=im_2;
if(trials != 0)
  {
  if(shift[trials]*shift[trials-1] < 0)
    {
    if(fabs(offset[trials]) > 2*fabs(offset[trials-1]) )
      {
      if(fabs(offset[trials]) > 0.5*fabs(offset[trials-1]) )
        {
// The previous fit was much better. Use it !!
        cg_wave_coh_re=old_re;
        cg_wave_coh_im=old_im;
        trials--;
        }
      else
        {
// We have two results, one from each side. 
// None is much better than the other.
// Use the weighted average.        
        t1=midpoint[trials];
        t2=midpoint[trials-1];
        if(t1-t2 >  (baseband_size >>1))t1=t1-baseband_size;                    
        if(t1-t2 < -(baseband_size >>1))t1=t1+baseband_size;                    
        p1=offset[trials];
        p2=offset[trials-1];
        p1=1/(0.2+fabs(p1));
        p2=1/(0.2+fabs(p2));
        p1=p1*p1;
        p2=p2*p2;
        t1=(p1*t1+p2*t2)/(p1+p2);
        if(t1 >= baseband_size)t1-=baseband_size;
        if(t1<0)t1+=baseband_size;
        midpoint[trials]=t1;
        t1=ampl[trials];
        t2=ampl[trials-1];
        ampl[trials]=(p1*t1+p2*t2)/(p1+p2);
        cg_wave_coh_re=(p1*re_2+p2*old_re)/(p1+p2);
        cg_wave_coh_im=(p1*im_2+p2*old_im)/(p1+p2);
        }
      }
    }
  }  
cg_wave_midpoint=midpoint[trials];
cg_wave_fit=ampl[trials];
cg_wave_coh_re/=dash_sumsq;
cg_wave_coh_im/=dash_sumsq;
}


int store_symmetry_adapted_dash(int ideal)
{
unsigned int i, j, k, ih;
int ia, ib, ic, im;
float t1, t2, t3, r1, r2, r3, der1, der2, phmax, phslope;
float maxpow, afac;
// Dashes contain a lot of energy. The AFC has placed the
// center of gravity of the carrier at freq=0 and the 
// coherent detect procedure should ensure that the
// average phase over the duration of a dash should be close to zero.
// The amplitude should be constant while the phase should
// be zero at the midpoint.
// Knowing that the curve shape should be symmetrical around it's
// midpoint, fold the signal over and form sums and differences.

if(PR00 != 0)
  {
  for(j=0; j<cw_avg_points; j++)
    {
    DEB"\nAVG in: %d  %f  ",j,  fftn_tmp[2*j  ]);
    }
  }        
ia=0;
ib=2*(cw_avg_points-1);

while (ia < ib)
  {
  t1=fftn_tmp[ib];
  t2=fftn_tmp[ia];
PRT00"\nre: [%d]=%f  [%d]=%f     %f,%f",ia/2, ZZ*t1, ib/2,ZZ*t2,ZZ*(t1+t2),ZZ*(t1-t2));
  fftn_tmp[ia]=t1+t2;
  fftn_tmp[ib]=t1-t2;

  t1=fftn_tmp[ia+1];
  t2=fftn_tmp[ib+1];
PRT00"\nim: [%d]=%f  [%d]=%f     %f,%f",ia/2, ZZ*t1, ib/2,ZZ*t2,ZZ*(t1-t2),ZZ*(t1+t2));
  fftn_tmp[ia+1]=t1-t2;
  fftn_tmp[ib+1]=t1+t2;
  ia+=2;
  ib-=2;
  }
// Step from the center until we reach the 6dB point.
// Get it as the summed powers of the symmetric real part and 
// the antisymmetric imaginary part.
// Collect the total power.
// Also collect the noise as the deviation from symmetry.
maxpow=0;
r1=0;
r2=0;
r3=0;
t1=0;
t3=0;
while(ia >= 0)
  {
  ia-=2;
  ib+=2;
  t1=fftn_tmp[ia]*fftn_tmp[ia]+fftn_tmp[ia+1]*fftn_tmp[ia+1];
PRT00"\nA: %d  %d  %f",ia/2,ib/2,ZZ*ZZ*t1);
  if(maxpow < t1)maxpow=t1;
  if(maxpow > 4*t1)goto collect_power_x;
  t3=t1;
  r1+=t1;
  r2+=fftn_tmp[ib  ]*fftn_tmp[ib  ];
  r3+=fftn_tmp[ib+1]*fftn_tmp[ib+1];
  }
collect_power_x:;
PRT02"\nTest for error: r1 %f ia %d ib %d  %.1f < %d < %.1f",
               ZZ*ZZ*r1,ia/2,ib/2,4.5*cwbit_pts,(ib-ia), 7.5*cwbit_pts);
if(r1 == 0 || ib-ia < 4.5*cwbit_pts || ib-ia > 7.5*cwbit_pts )
  {
  PRT01"Waveform error 1");    
  return FALSE;
  }
r2/=r1;
r3/=r1;
PRT02"\nTest for error: r2(<.05) %f  r3(<.03) %f",r2,r3);
if(r2 > 0.05 || r3>0.03)
  { 
  PRT01"Waveform error 2");    
  return FALSE;
  }
ih=ia;
t2=0.05*maxpow;
while(ia > 0 && t1 < t3 && t1 > t2 && fftn_tmp[ia]>0)
  {
  t3=t1;
  ia-=2;
  t1=fftn_tmp[ia]*fftn_tmp[ia]+fftn_tmp[ia+1]*fftn_tmp[ia+1];
  }
PRT00"\nia%d,ih%d",ia/2,ih/2);


// The phase angle should vary linearly with time and go
// through zero at the midpoint.
// Normalize for a peak amplitude=1
// Store the power and the phase angle.
afac=1/sqrt(maxpow);
k=cw_avg_points+2;
for(i=ia; i<cw_avg_points; i+=2)
  {
  fftn_tmp[i]*=afac;
  fftn_tmp[i+1]*=afac;
  fftn_tmp[k+i  ]=fftn_tmp[i]*fftn_tmp[i]+fftn_tmp[i+1]*fftn_tmp[i+1];
  fftn_tmp[k+i+1]=atan2(fftn_tmp[i+1],fftn_tmp[i]);
  }
if(PR00 != 0)
  {
  for(j=ia/2; j<cw_avg_points/2; j++)
    {
    DEB"\nB: %d  %f %f ",j,  fftn_tmp[2*j  ],  fftn_tmp[2*j+1  ]);
    }
  }        
// The phase angle has to be zero halfway between ib and the next
// point ib+2 since we work in the symmetry adapted function.
// Collect the power weighted average slope.
r1=0;
phslope=0;
for(i=ia; i<cw_avg_points; i+=2)
  {
  r1+=fftn_tmp[k+i];
  phslope+=fftn_tmp[k+i]*fftn_tmp[k+i+1]/(cw_avg_points-i-1);
  }
phslope/=r1;
// Use the average slope to construct the phase function 
// The real part is cos(x), make the imaginary part sin(x) 
// while collecting the rms error and total power.
t1=0;
t2=0;
for(i=ia; i<cw_avg_points; i+=2)
  {
  t1+=fftn_tmp[i]*fftn_tmp[i];
  r1=fftn_tmp[i]*tan((cw_avg_points-i-1)*phslope);
  t2+=(fftn_tmp[i+1]-r1)*(fftn_tmp[i+1]-r1);
  fftn_tmp[i+1]=r1;
  }
PRT02"\nTest for error: t2/t1 (<.05) %f ",t2/t1);
if(t2/t1 > 0.05)
  {
  PRT01"Waveform error 3");    
  return FALSE;
  }
phmax=phslope*(cw_avg_points-ih+1);
cg_chirpx=cg_x0-1.5*cg_size*cos(phmax);
cg_chirpy=cg_y0-1.5*cg_size*sin(phmax);
// ih is the point of half amplitude or nearest below.
// Our data may be corrupted by noise so it is perhaps not
// very reliable at low amplitudes.
// Remember the beginning of the curve in case we have an ideal
// waveform as input.
// We will use it for the beginning of the dot waveform
// on the averaged data that is less accurate at low levels.

if(PR00 != 0)
  {
  for(j=ia/2; j<cw_avg_points/2; j++)
    {
    DEB"\nC: %d  %f %f ",j,  fftn_tmp[2*j  ],  fftn_tmp[2*j+1  ]);
    }
  }        

if(ideal == TRUE)
  {
  im=ia;
  while(im>0 && fftn_tmp[im-2]*afac > 0.01)
    {
    im-=2;
    fftn_tmp[im]*=afac;
    }
  if(im > 0)im-=2;  
  ib=im;
  while(ib >=0 )
    {
    fftn_tmp[ib]=0;
    ib-=2;
    }


if(PR00 != 0)
  {
  DEB"\nim%d,ia%d,ih%d",im/2,ia/2,ih/2);
  for(j=ia/2; j<cw_avg_points/2; j++)
    {
    DEB"\nD: %d  %f %f ",j,  fftn_tmp[2*j  ],  fftn_tmp[2*j+1  ]);
    }
  }        
  i=0;  
  ib=im;
  if(fftn_tmp[ib] != 0)
    {
    cw_ideal_rise[0]=0;
    i=1;
    }
  while( i< MAX_CW_IDEAL_RISE)
    {
    cw_ideal_rise[i]=fftn_tmp[ib];
    ib+=2;
    i++;
    }

  if(PR00 != 0)
    {
    for(j=0; j<i; j++)
      {
      DEB"\nE: %d  %f",j,cw_ideal_rise[j]);
      }
    }  
  for(i=0; i<cw_avg_points; i++)fftn_tmp[2*ib]=0;
  }
else
  {
// Replace the first part of the rising edge by
// the stored ideal waveform.
// Point im to the point that comes nearest the -6dB point.
  im=ia;
  while(fabs(fftn_tmp[im]-0.5) > fabs(fftn_tmp[im+2]-0.5))
    {
    im+=2;
    }
  t1=fftn_tmp[im];
  ih=im;
  PRT00"\n im= %d  %f",im/2,t1);
// Point i to the first point in the ideal curve with a value
// higher than the value on our average at im.
  i=0;
  while(cw_ideal_rise[i] < t1)
    {
    i++;
    }
// Fit a parabola to the three points i-2,i-1 and i.
  der2=0;
  der1=cw_ideal_rise[i+1]-cw_ideal_rise[i];
next_ideal:;
  r3=cw_ideal_rise[i];
  if(i > 1)
    {
    r2=cw_ideal_rise[i-1];
    r1=cw_ideal_rise[i-2];
    der2=r3+r1-2*r2;
    der1=r3-r1;
    }
  else
    {
    if(i > 0)
      {
      r2=cw_ideal_rise[i-1];
      der2=0;
      der1=r3-r1;
      }
    }  
  PRT00"\nder1 %f   der2 %f",der1,der2);    
  im-=2;
  i--; 
  PRT02"\nTest for error im(>0) %d    i(<1) %d",im,i);
  if(im < 0 || i< 1)
    {
    PRT01"Waveform error 8");    
    return FALSE;
    }
  t1-=der1-0.5*der2;
  fftn_tmp[im]=t1;
  PRT00"\nH: %d  %f  i=%d",im/2,  fftn_tmp[im],i);
  if(t1 > 0)goto next_ideal;
  fftn_tmp[im]=0;
// Put the imaginary part in place.
  for(i=im; i<=ih; i+=2)
    {
    fftn_tmp[i+1]=r1=fftn_tmp[i]*tan((cw_avg_points-i-1)*phslope);
    }
  }
// Unfold the waveform to get the symmetry adapted original waveform back.
ia=im;
ib=2*(cw_avg_points-1)-im;
while (ia < ib)
  {
  fftn_tmp[ib]=fftn_tmp[ia];
  fftn_tmp[ib+1]=-fftn_tmp[ia+1];
  ia+=2;
  ib-=2;
  }
ic=im;
while(fftn_tmp[ic]==0)ic+=2;
dash_pts=1;
dash_waveform[0]=0;
dash_waveform[1]=0;
while(fftn_tmp[ic] != 0 )
  {
  dash_waveform[2*dash_pts  ]=fftn_tmp[ic  ];
  dash_waveform[2*dash_pts+1]=fftn_tmp[ic+1];
  ic+=2;
  dash_pts++;
  if(dash_pts > cw_waveform_max-2)
    {
    PRT01"Waveform error 4");    
    return FALSE;
    }
  } 
PRT02"\nTest for error %d < %d ",dash_pts, cw_waveform_max-2);     
dash_waveform[2*dash_pts  ]=0;
dash_waveform[2*dash_pts+1]=0;
dash_pts++;
dash_sumsq=0;
t1=0;
for(ia=0; ia<dash_pts; ia++)
  {
PRT00"\nDASH WAVEFORM %d %f %f",ia,dash_waveform[2*ia  ],dash_waveform[2*ia+1]);
  
  t2=dash_waveform[2*ia  ]*dash_waveform[2*ia  ]+
     dash_waveform[2*ia+1]*dash_waveform[2*ia+1];
  dash_sumsq+=t2;
  if(t1<t2)t1=t2;
  }
t1/=6;
cwbit_pts=0;
for(ia=0; ia<dash_pts; ia++)
  {
  t2=dash_waveform[2*ia  ]*dash_waveform[2*ia  ]+
     dash_waveform[2*ia+1]*dash_waveform[2*ia+1];
  if(t2>t1)cwbit_pts++;
  }
cwbit_pts/=3;
return TRUE;
}



void clear_coherent(void)
{
int i,n,nn,k;
float t1;
// ********   Clear graphics data *********
n=cg_size*cg_size;
for(i=0; i<n; i++)cg_map[i]=0;
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  clear_cg_traces();
// *********** Clear data for cw speed determination *********
  for(i=0; i<keying_spectrum_size; i++)keying_spectrum[i]=0;
// The keying spectra arrive at a rate of 2*baseband_sampling_speed/mix2.size.
// The baseband bandwidth (user selected filter) is keying_spectrum_size/2
// so the frequency of a string of morse code dots that fit the
// filter would be keying_spectrum_size/4.
// Accumulate power spectra in a leaky integrator that has a life time
// of 150 morse code dots.
  t1=((150*4)/keying_spectrum_size);
  keying_spectrum_max=0.5+t1/3;
  keying_spectrum_f1=pow(0.25,1/t1);
  keying_spectrum_cnt=0;
  no_of_cwdat=0;
  correlate_no_of_cwdat=0;
  cw_detect_flag=CWDETECT_CLEARED;
  }
// ********* Clear old data so the oscilloscope will not 
// display incorrect information. *********
nn=(baseb_ps-cg_osc_offset+cg_osc_points-cg_oscw+baseband_size)&baseband_mask;
k=(baseb_pa-nn+baseband_size)&baseband_mask;
if(k+nn <= baseband_size)
  { 
  clear_baseb_arrays(nn,k);
  }
else
  {
  clear_baseb_arrays(nn,baseband_size-nn);
  clear_baseb_arrays(0,k-baseband_size+nn);
  }  
// ***********  Clear all the baseb pointers ********
baseb_pb=baseb_pa;
baseb_pc=baseb_pa;
baseb_pd=baseb_pa;
baseb_pe=baseb_pa;
baseb_pf=baseb_pa;
baseb_ps=baseb_pa;
baseb_pm=baseb_pa;
baseb_px=baseb_pa;
nn=0.5+daout_pa/(rx_daout_channels*rx_daout_bytes*da_resample_ratio);
baseb_fx=(baseb_pa-2-nn+baseband_size)&baseband_mask;
// *********** Clear basblock. Used for S-meter averaging ************ 
basblock_pa=2*baseb_pa/mix2.size;
nn=basblock_pa;
for(i=0; i<basblock_hold_points; i++)
  {
  basblock_maxpower[nn]=0;
  basblock_avgpower[nn]=0;
  nn=(nn+basblock_mask)&basblock_mask;
  }
cg_chirpx=cg_x0;
cg_chirpy=cg_y0;
s_meter_peak_hold=0;
cg_maxavg=0;
if(s_meter_avgnum > 0)
  {
  s_meter_avg=0;
  s_meter_avgnum=0;
  }
}

