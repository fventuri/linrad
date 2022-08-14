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
#include "fft3def.h"
#include "sigdef.h"
#include "seldef.h"
#include "screendef.h"
#include "options.h"


float fmpow[32];
float fmbuf[64];

int iniflag=0;

float fm_phase;
float fm_phasediff_avg;

float fm_lock_factor=0.9;

float fmfix(int p0, float *z, float *pha)
{
int i, nn;
int i0, i1, i2;
float t1,t2;
t2=pha[0];
if(fm_n==0)
  {
  t1=atan2(z[2*p0+1],z[2*p0]);
  }
else
  {  
  nn=(p0-fm_size+baseband_size)&baseband_mask;
  for(i=0; i<fm_size; i++)
    {
    fmbuf[2*i  ]=fm_win[i]*z[2*nn  ];
    fmbuf[2*i+1]=fm_win[i]*z[2*nn+1];
    nn=(nn+1)&baseband_mask;
    }
  fftforward(fm_size, fm_n, fmbuf, fm_tab, fm_permute, FALSE);
  t1=0;
  i1=0;
  for(i=0; i<fm_size; i++)
    {
    fmpow[i]=fmbuf[2*i]*fmbuf[2*i]+fmbuf[2*i+1]*fmbuf[2*i+1];
    if(t1 < fmpow[i])
      {
      t1=fmpow[i];
      i1=i;
      }
    }
  i0=(i1-1+fm_size)&(fm_size-1);    
  i2=(i1+1)&(fm_size-1);
  t1=atan2(  -fmpow[i0]*fmbuf[2*i0+1]+
              fmpow[i0]*fmbuf[2*i1+1]-
              fmpow[i0]*fmbuf[2*i2+1],
             -fmpow[i0]*fmbuf[2*i0  ]+
              fmpow[i0]*fmbuf[2*i1  ]-
              fmpow[i0]*fmbuf[2*i2  ]);
  t1+=(i0&1)*PI_L;           
  }
pha[0]=t1;
t1-=t2;
if(t1 > PI_L)t1-=2*PI_L;                
if(t1 < -PI_L)t1+=2*PI_L;
return t1;
}

void detect_fm(void)
{
float *zr;
float t1, t2, t3;
float pil2_sin, pil2_cos, pil2_phstep_sin, pil2_phstep_cos;
int p0, p1, last_point;
int use_audio_filter;
int i, mm, nn;
#if NET_BASEBRAW_MODE == BASEBAND_IQ || \
    NET_BASEBRAW_MODE == BASEBAND_IQ_TEST || \
    NET_BASEBRAW_MODE == BASEBAND_IQ_MASTER_TEST
// We do absolutely nothing when the user specified 
// that he wants the raw baseband signal.
if( (ui.network_flag&NET_RXOUT_BASEBRAW) != 0)return;
#endif
if(genparm[DA_OUTPUT_SPEED]/2 > 1000*bg.fm_audio_bw)
  {
  use_audio_filter=TRUE;
  }
else
  {
  use_audio_filter=FALSE;
  }
if(bg_coherent == 0)
  {
  zr=baseb_raw_orthog;
  }
else
  {
  zr=baseb_carrier;
  }  
// ************************  Step 1 ***********************
// Make an FM detector by computing the phase with atan2.
// then take the derivative to get the instantaneous 
// frequency.
// The phase can also be determined from an FFT in fmfix()
// Return immediately if bg.fm_audio_bw equals 10 since the
// corresponding filter would not reduce the bandwidth.
// ********************************************************
p0=baseb_pa;
last_point=(baseb_pa+mix2.new_points)&baseband_mask;
if(baseb_channels == 1)
  {
  if(use_audio_filter)
    {
    while(p0 != last_point)
      {
      baseb_fm_demod[p0 ]=fmfix(p0,baseb_raw,&fm0_ph1);     
      if(bg.fm_subtract != 0)
        {
        baseb_fm_phase[p0]=fm0_ph1;
        }
      p0=(p0+1)&baseband_mask;
      }  
    }
  else
    {
    while(p0 != last_point)
      {
      baseb_out[2*p0  ]=fmfix(p0,baseb_raw,&fm0_ph1);
      baseb_out[2*p0+1]=fmfix(p0,baseb_raw,&fm0_ph1);
      p0=(p0+1)&baseband_mask;
      }
    return;
    }
  }
else
  {
  if(use_audio_filter)
    {
    while(p0 != last_point)
      {
      baseb_fm_demod[2*p0  ]=fmfix(p0,baseb_raw,&fm0_ph1);
      baseb_fm_demod[2*p0+1]=fmfix(p0,zr,&fm0_ph2);
      p0=(p0+1)&baseband_mask;
      }
    }
  else
    {
    while(p0 != last_point)
      {
      baseb_out[4*p0  ]=fmfix(p0,baseb_raw,&fm0_ph1);
      baseb_out[4*p0+1]=0;      
      baseb_out[4*p0+2]=fmfix(p0,zr,&fm0_ph2);
      baseb_out[4*p0+3]=0;      
      p0=(p0+1)&baseband_mask;
      }
    return;  
    }
  }
#if NET_BASEBRAW_MODE == WFM_FM_FULL_BANDWIDTH || \
    NET_BASEBRAW_MODE == WFM_AM_FULL_BANDWIDTH
if( (ui.network_flag & NET_RXOUT_BASEBRAW) != 0)return;
#endif
// **********************************************************
if(bg.fm_subtract == 2 ||
   fm_pilot_size == 0 || 
   bg_coherent != 0 ||
   baseb_channels != 1)goto nbfm;
if(bg.fm_subtract == 1 && (ui.network_flag & NET_RXOUT_BASEBRAW) != 0)
  {
// We need the amplitude and the frequency of the dominating signal
// without any contribution from the intermodulation products at 100 kHz
// which would come from stations on neighbouring channels.
// A good enough filter directly at the baseband frequency is very
// CPU intensive since we want filters with steep fall-off.
// We apply the filters in steps. First apply a filter that cuts at
// about 70 kHz.
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SYNTHETIZED_FROM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SUBTRACT_FROM_FIRST_LOWPASS
// we would normally resample here, but if the user wants to analyze
// the first low pass filter, or to run fm subtraction without resamplers
// here is an option that applies a low pass filter at the full sampling
// rate.
  p0=baseb_pa;
  p1=(baseb_pa-fmfil70_points/2+baseband_mask)&baseband_mask;
  while(p0 != last_point)
    {
    nn=p1;
    t1=fmfil70_fir[fmfil70_points/2]*baseb_fm_demod[nn];
    t2=fmfil70_fir[fmfil70_points/2]*baseb_totpwr[nn];
    mm=nn;
    for(i=fmfil70_points/2-1; i>=0; i--)
      {
      nn=(nn+baseband_mask)&baseband_mask;
      mm=(mm+1)&baseband_mask;
      t1+=fmfil70_fir[i]*(baseb_fm_demod[nn]+baseb_fm_demod[mm]);
      t2+=fmfil70_fir[i]*(baseb_totpwr[nn]+baseb_totpwr[mm]);
      }
    baseb_fm_demod_low[p0]=t1;
    baseb_carrier_ampl[p0]=t2;
    p1=(p1+1)&baseband_mask;
    p0=(p0+1)&baseband_mask;
    }
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS
  if( (ui.network_flag & NET_RXOUT_BASEBRAW) != 0)return;
#endif
  p1=(baseb_pa-fmfil70_points/2+baseband_mask)&baseband_mask;
  p0=baseb_pa;
  while(p0 != last_point)
    {
    fm_phase+=baseb_fm_demod_low[p0];
    if(fm_phase > PI_L)fm_phase-=2*PI_L;
    if(fm_phase < -PI_L)fm_phase+=2*PI_L;
    t1=fm_phase-baseb_fm_phase[p1];
    if(t1 > PI_L)t1-=2*PI_L;
    if(t1 < -PI_L)t1+=2*PI_L;
    fm_phasediff_avg=fm_lock_factor*fm_phasediff_avg+(1-fm_lock_factor)*t1;
    if(fabs(t1) > PI_L/2)
      {
      fm_phase=baseb_fm_phase[p1];
      }
    fm_phase-=fm_phasediff_avg*(1-fm_lock_factor);  
    t1=sqrt(baseb_carrier_ampl[p1]);
    baseb_carrier[2*p0  ]=-t1*cos(fm_phase);
    baseb_carrier[2*p0+1]=-t1*sin(fm_phase);
#if NET_BASEBRAW_MODE == WFM_SUBTRACT_FROM_FIRST_LOWPASS
    baseb_carrier[2*p0]+=baseb_raw[2*p1];
    baseb_carrier[2*p0+1]+=baseb_raw[2*p1+1];
#endif  
    p1=(p1+1)&baseband_mask;
    p0=(p0+1)&baseband_mask;
    }
  return;
#endif

/*
  Various code pieces for future implementation.

// Wi will eventually subtract the dominating signal, but here
// we detect it properly to improve S/N of the modulation and
// to allow fine tuning of the different modulation components.
// Here we resample after the low pass filters.
// The cpu load becomes much smaller since we have to compute only
// every Nth point for the output.
  p1=(baseb_pf-fmfil70_points/2+baseband_mask)&baseband_mask;
  p2=(baseb_pa-fmfil70_points/2+baseband_mask)&baseband_mask;
  np=(p2-p1+baseband_size)&baseband_mask;
  np=fm1_resample_ratio*(np/fm1_resample_ratio);
  k=(p1+np)&baseband_mask;
  while(p1 != k)
    {
    nn=p1;
    t2=fmfil70_fir[fmfil70_points/2]*baseb_fm_demod[nn];
    mm=nn;
    for(i=fmfil70_points/2-1; i>=0; i--)
      {
      nn=(nn+baseband_mask)&baseband_mask;
      mm=(mm+1)&baseband_mask;
      t2+=fmfil70_fir[i]*(baseb_fm_demod[nn]+baseb_fm_demod[mm]);
      }
    fm1_all[fm1_pa]=t2;
    p1=(p1+fm1_resample_ratio)&baseband_mask;
    fm1_pa=(fm1_pa+1)&fm1_mask;
    }
  baseb_pf=(baseb_pf+np)&baseband_mask;
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS_RESAMP || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS_RESAMP
  return;
#endif



  fm1_p0=0;
  p0=fm1_p0;
  p1=(baseb_p0-fm1_resample_ratio/2+baseband_mask)&baseband_mask;
  p2=(baseb_pa-fm1_resample_ratio/2+baseband_size)&baseband_mask;
  np=(p2-p1+baseband_size)&baseband_mask;
  np=fm1_resample_ratio*(np/fm1_resample_ratio);
  k=(p1+np)&baseband_mask;
  while(p1 != k)
    {
    t1=fm1_all[p0];
    nn=p1;
    mm=p2;
    while(mm != nn)
      {
      nn=(nn+baseband_mask)&baseband_mask;
      mm=(mm+1)&baseband_mask;
      baseb_fm_phase[nn]=baseb_fm_demod[nn];
      nn=(nn+1)&baseband_mask;
      }
    p1=(p1+fm1_resample_ratio)&baseband_mask;
    p2=(p2+fm1_resample_ratio)&baseband_mask;
    p0=(p0+1)&fm1_mask;
    }
  return;
*/  
  
  }
// **********  Step 2  (for WFM stereo)  *************
// Apply low pass filters as specified by fmfil55fir and 
// fmfil70fir
// Put the lowest frequency range, 0 to 55 kHz in baseb_fm_demod_low[]
// Put the middle part, 55 to 70 kHz in baseb_fm_high[]
// ******************************************************

p1=(baseb_pa-fmfil55_points/2+baseband_mask)&baseband_mask;
p0=baseb_pa;
while(p0 != last_point)
  {
  nn=p1;
  t1=fmfil55_fir[fmfil55_points/2]*baseb_fm_demod[nn];
  mm=nn;
  for(i=fmfil55_points/2-1; i>=0; i--)
    {
    nn=(nn+baseband_mask)&baseband_mask;
    mm=(mm+1)&baseband_mask;
    t1+=fmfil55_fir[i]*(baseb_fm_demod[nn]+baseb_fm_demod[mm]);
    }
#if NET_BASEBRAW_MODE != WFM_STEREO_HIGH
  baseb_fm_demod_low[p0]=t1;
#endif
#if NET_BASEBRAW_MODE != WFM_STEREO_LOW
  nn=p1;
  t2=fmfil70_fir[fmfil70_points/2]*baseb_fm_demod[nn];
  mm=nn;
  for(i=fmfil70_points/2-1; i>=0; i--)
    {
    nn=(nn+baseband_mask)&baseband_mask;
    mm=(mm+1)&baseband_mask;
    t2+=fmfil70_fir[i]*(baseb_fm_demod[nn]+baseb_fm_demod[mm]);
    }
#if NET_BASEBRAW_MODE == WFM_STEREO_HIGH    
  baseb_fm_demod_low[p0]=t2-t1;
#else
  baseb_fm_demod_high[p0]=t2-t1;
#endif
#endif  
  p1=(p1+1)&baseband_mask;
  p0=(p0+1)&baseband_mask;
  }
#if NET_BASEBRAW_MODE == WFM_STEREO_LOW || \
    NET_BASEBRAW_MODE == WFM_STEREO_HIGH
if( (ui.network_flag & NET_RXOUT_BASEBRAW) != 0)return;
#endif    
// **********  Step 3  (for wideband stereo)  *************
// Look for the pilot tone. 
// ********************************************************
//  baseb_fm_demod[0]
//  baseb_fm_demod_low[fmfil55_points/2]
//  baseb_fm_demod_high[fmfil55_points/2]
  p1=last_point;
  t1=0;
  t2=0;
  for(i=0; i<fm_pilot_size; i++)
    {
    p1=(p1+baseband_mask)&baseband_mask;
    t1+=baseb_fm_demod_low[p1]*fm_pilot_tone[2*i  ];
    t2+=baseb_fm_demod_low[p1]*fm_pilot_tone[2*i+1];
    }
// **********  Step 4  (for wideband stereo)  *************
// Detect the stereo channel and the RDS channel.
// for development purposes, remember the pilot tone and its overtones.
// ********************************************************
  p0=(baseb_pa+baseband_mask)&baseband_mask;
  p1=last_point;
  t1=atan2(t1,t2);
  t2=2*PI_L*19000./baseband_sampling_speed;
  pil2_sin=sin(2*t1);
  pil2_cos=cos(2*t1);
  pil2_phstep_sin=sin(2*t2);
  pil2_phstep_cos=cos(2*t2);
  while(p0 != p1)
    {
    p1=(p1+baseband_mask)&baseband_mask;
    baseb_fm_pil2det[p1]=pil2_sin*baseb_fm_demod_low[p1];
    t3=pil2_cos;
    pil2_cos= pil2_cos*pil2_phstep_cos-pil2_sin*pil2_phstep_sin;
    pil2_sin= pil2_sin*pil2_phstep_cos+t3*pil2_phstep_sin;
//    baseb_fm_pil3[2*p1  ]=pil3_sin;
//    baseb_fm_pil3det[2*p1  ]=pil3_sin*baseb_fm_demod_high[p1];
//    baseb_fm_pil3[2*p1+1]=pil3_cos;
//    baseb_fm_pil3det[2*p1+1]=pil3_cos*baseb_fm_demod_high[p1];
// **************  probe the RDS signal ****************
//  baseb_out[2*p1  ]=baseb_fm_pil3det[2*p1  ];
//  baseb_out[2*p1+1]=baseb_fm_pil3det[2*p1+1];
//Ö    t3=pil3_cos;
//Ö    pil3_cos= pil3_cos*pil3_phstep_cos-pil3_sin*pil3_phstep_sin;
//Ö    pil3_sin= pil3_sin*pil3_phstep_cos+t3*pil3_phstep_sin;
    }
// **********  Step 5  (for wideband stereo)  *************
// Apply a low pass filter with cutoff frequency as specified
// by fm_audiofil_fir to the sound channels.
// ********************************************************
//  baseb_fm_demod[0]
//  baseb_fm_demod_low[fmfil55_points/2]
//  baseb_fm_demod_high[fmfil55_points/2]
//  baseb_fm_pil2[fmfil55_points/2]
//  baseb_fm_pil2det[fmfil55_points/2]
//  baseb_fm_pil3[fmfil55_points/2]
//  baseb_fm_pil3det[fmfil55_points/2]
p0=baseb_pa;
p1=(baseb_pa-fm_audiofil_points/2+baseband_mask)&baseband_mask;
while(p0 != last_point)
  {
  nn=p1;
  t1=fm_audiofil_fir[fm_audiofil_points/2]*baseb_fm_demod_low[nn];
  t2=fm_audiofil_fir[fm_audiofil_points/2]*baseb_fm_pil2det[nn];
  mm=nn;
  for(i=fm_audiofil_points/2-1; i>=0; i--)
    {
    nn=(nn+baseband_mask)&baseband_mask;
    mm=(mm+1)&baseband_mask;
    t1+=fm_audiofil_fir[i]*(baseb_fm_demod_low[nn]+baseb_fm_demod_low[mm]);
    t2+=fm_audiofil_fir[i]*(baseb_fm_pil2det[nn]+baseb_fm_pil2det[mm]);
    }
  baseb_out[2*p0  ]=t1+t2;
  baseb_out[2*p0+1]=t1-t2;
  p1=(p1+1)&baseband_mask;
  p0=(p0+1)&baseband_mask;
  }
return;

/*
// **********  Step 6  (for wideband stereo)  *************
// Low pass filter the RDS channels using fmfil_rds_fir.
// ********************************************************
//  baseb_fm_demod[0]
//  baseb_fm_demod_low[fmfil55_points/2]
//  baseb_fm_demod_high[fmfil55_points/2]
//  baseb_fm_pil2[fmfil55_points/2]
//  baseb_fm_pil2det[fmfil55_points/2]
//  baseb_fm_pil3[fmfil55_points/2]
//  baseb_fm_pil3det[fmfil55_points/2]
//  baseb_fm_sumchan[fmfil55_points/2+fmfil_points/2]
//  baseb_fm_diffchan[fmfil55_points/2+fmfil_points/2]
lir_fillbox(0,0,101,101,color_scale[1]);
p2=(baseb_pa-(fmfil_rds_points+fmfil55_points)/2+baseband_mask)&baseband_mask;
p1=(baseb_pa-fmfil_rds_points/2+baseband_mask)&baseband_mask;
p0=baseb_pa;
while(p0 != last_point)
  {
  nn=p1;
  r1=fmfil_rds_fir[fmfil_rds_points/2]*baseb_fm_pil3det[2*nn  ];
  r2=fmfil_rds_fir[fmfil_rds_points/2]*baseb_fm_pil3det[2*nn+1];
  mm=nn;
  for(i=fmfil_rds_points/2-1; i>=0; i--)
    {
    nn=(nn+baseband_mask)&baseband_mask;
    mm=(mm+1)&baseband_mask;
    r1+=fmfil_rds_fir[i]*(baseb_fm_pil3det[2*nn  ]+baseb_fm_pil3det[2*mm  ]);
    r2+=fmfil_rds_fir[i]*(baseb_fm_pil3det[2*nn+1]+baseb_fm_pil3det[2*mm+1]);
    }
//  baseb_fm_rdsraw[2*p0  ]=r1;
//  baseb_fm_rdsraw[2*p0+1]=r2;
//  *************  probe the RDS signal after the filter  ****************
//baseb_out[2*p0  ]=r1;
//baseb_out[2*p0+1]=r2;


  t2=r1*r1+r2*r2;
  if(t2 > 0.00000000001)
    {
    t1=atan2(r1,r2);
    }
  else
    {
    t1=0;
    }
  i=1;    
  if(t1-rds_phase<PI_L)rds_phase-=2*PI_L;
  if(t1-rds_phase>PI_L)rds_phase+=2*PI_L;
  if(t1-rds_phase<-0.5*PI_L)
    {
    t1+=PI_L;
    i=-1;
    }
  if(t1-rds_phase>0.5*PI_L)
    {
    t1-=PI_L;
    i=-1;
    }
    
   
  rds_power=rds_f1*rds_power+rds_f2*t2;
  if(rds_power > 0.000000001)
    {
    rds_phase=(1-(t2/rds_power))*rds_phase + 
               (t2/rds_power)*(rds_f1*rds_phase+rds_f2*t1);
    }
// Here it would be favourable to decode the rds signal, correct errors
// and to construct the correct rds signal with correct amplitude
// and rise times. For this test of principles, just use whatever
// amplitude that we have detected.
// the rds signal is i*sqrt(t2)  
// Here we could put it into a downsampled array.
//
// Start to build a cleaned up version of the detected composite
// signal. First we store the rds signal.

  baseb_fm_composite[p0]=(sin(rds_phase)*baseb_fm_pil3[2*p1  ]+
                   cos(rds_phase)*baseb_fm_pil3[2*p1+1])*rds_gain*i*sqrt(t2);
// here we should perhaps use the filtered signals and construct
// the ideal pilot tone. At good S/N it should not be needed.

  baseb_fm_composite[p0]+=baseb_fm_demod_low[p1];


fm_phase+=baseb_fm_composite[p0];
if(fm_phase > PI_L)fm_phase-=2*PI_L;
if(fm_phase < -PI_L)fm_phase+=2*PI_L;

//baseb_out[2*p0]=baseb_fm_composite[p0]-baseb_fm_demod[p2]; 
//baseb_out[2*p0+1]=0;

t1=fm_phase-baseb_fm_phase[p2];
if(t1 > PI_L)t1-=2*PI_L;
if(t1 < -PI_L)t1+=2*PI_L;

fm_phasediff_avg=fm_lock_factor*fm_phasediff_avg+(1-fm_lock_factor)*t1;

if(fabs(t1) > PI_L/2)
  {
  fm_phase=baseb_fm_phase[p2];
  }
  
fm_phase-=fm_phasediff_avg*(1-fm_lock_factor);  


t1=baseb_totpwr[p2]+baseb_totpwr[(p2+baseband_mask)&baseband_mask]+
                    baseb_totpwr[(p2+baseband_mask-1)&baseband_mask];
t1/=3;
baseb_out[2*p0  ]=-sqrt(t1)*cos(fm_phase);
baseb_out[2*p0+1]=-sqrt(t1)*sin(fm_phase);

//baseb_out[2*p0]+=baseb_raw[2*p2]; 
//baseb_out[2*p0+1]+=baseb_raw[2*p2+1];





  if(rx_daout_channels ==1)
    {
    baseb_out[2*p0  ]=r1;
    baseb_out[2*p0+1]=0;
    }
  else
    {
    baseb_out[2*p0  ]=r1;
    baseb_out[2*p0+1]=r2;
    }

if(p0%32==0)
{
mailbox[0]=50+1000*r1;
mailbox[1]=50+1000*r2;
if(mailbox[0]<0)mailbox[0]=0;
if(mailbox[0]>100)mailbox[0]=100;
if(mailbox[1]<0)mailbox[1]=0;
if(mailbox[1]>100)mailbox[1]=100;
lir_setpixel(mailbox[0],mailbox[1],15);
mailbox[2]=50+45*sin(rds_phase);
mailbox[3]=50+45*cos(rds_phase);
lir_setpixel(mailbox[2],mailbox[3],15);
}

  p2=(p2+1)&baseband_mask;
  p1=(p1+1)&baseband_mask;
  p0=(p0+1)&baseband_mask;
  }




return;

*/


// ********************  Step 2 for NBFM *************************
// Apply a low pass filter with cutoff frequency as specified
// by fm_audiofil_fir.
nbfm:;
p0=baseb_pa;
p1=(baseb_pa-fm_audiofil_points/2+baseband_mask)&baseband_mask;
if(baseb_channels == 1)
  {
  while(p0 != last_point)
    {
    nn=p1;
    t1=fm_audiofil_fir[fm_audiofil_points/2]*baseb_fm_demod[nn];
    mm=nn;
    for(i=fm_audiofil_points/2-1; i>=0; i--)
      {
      nn=(nn+baseband_mask)&baseband_mask;
      mm=(mm+1)&baseband_mask;
      t1+=fm_audiofil_fir[i]*(baseb_fm_demod[nn]+baseb_fm_demod[mm]);
      }
    baseb_out[2*p0  ]=t1;
    baseb_out[2*p0+1]=t1;
    p1=(p1+1)&baseband_mask;
    p0=(p0+1)&baseband_mask;
    }
  }  
else
  {
  while(p0 != last_point)
    {
    nn=p1;
    t1=fm_audiofil_fir[fm_audiofil_points/2]*baseb_fm_demod[2*nn  ];
    t2=fm_audiofil_fir[fm_audiofil_points/2]*baseb_fm_demod[2*nn+1];
    mm=nn;
    for(i=fm_audiofil_points/2-1; i>=0; i--)
      {
      nn=(nn+baseband_mask)&baseband_mask;
      mm=(mm+1)&baseband_mask;
      t1+=fm_audiofil_fir[i]*(baseb_fm_demod[2*nn  ]+baseb_fm_demod[2*mm  ]);
      t2+=fm_audiofil_fir[i]*(baseb_fm_demod[2*nn+1]+baseb_fm_demod[2*mm+1]);
      }
    baseb_out[2*p0  ]=t1;
    baseb_out[2*p0+1]=t2;
    p1=(p1+1)&baseband_mask;
    p0=(p0+1)&baseband_mask;
    }
  }


/*          
// *********************  Step 3 ******************************
// These locations correspond to the same moment in time:
// baseb_fm_fil1[p0]
// baseb_fm_raw[p0-len]
// baseb_fm_demod1[p0-len]
//
// The filtered data is more accurate than the unfiltered
// data. 
// Frequency modulate the original signal with the
// filtered demodulated signal. That should compress
// the bandwidth because we apply (nearly) the same
// FM modulation but in the opposite phase.
p0=baseb_pa;
p1=(baseb_pa-len+baseband_size)&baseband_mask;
t1=fm1_ph1;
if(baseb_channels == 1)
  {
  while(p0 != last_point)
    {
    t1+=baseb_fm_fil1[p0];
    if(t1>PI_L)t1-=2*PI_L;
    if(t1<-PI_L)t1+=2*PI_L;

    baseb_compressed_fm1[2*p0  ]= baseb_raw[2*p1  ]*cos(t1)+
                                  baseb_raw[2*p1+1]*sin(t1);
    baseb_compressed_fm1[2*p0+1]= baseb_raw[2*p1+1]*cos(t1)+
                                 -baseb_raw[2*p1  ]*sin(t1);                                 
//    baseb_compressed_fm1[2*p0  ]=baseb_raw[2*p1  ];
//    baseb_compressed_fm1[2*p0+1]=baseb_raw[2*p1+1];
    p0=(p0+1)&baseband_mask;
    p1=(p1+1)&baseband_mask;
    }
  }
fm1_ph1=t1;
*/

}  
