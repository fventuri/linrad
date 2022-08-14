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
#include <ctype.h>
#include <string.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "screendef.h"
#include "seldef.h"
#include "thrdef.h"
#include "txdef.h"
#include "vernr.h"
#include "hwaredef.h"
#include "keyboard_def.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

float tx_resamp_maxamp;
float tx_resamp_old_maxamp;
void tx_ssb_buftim(char cc);  

int tx_ssb_step2(float *totpwr)
{
int i, k;
float t1, t2;
// ************* Step 2. *************
// Compute the power for each bin within the passband.
// Apply muting in the frequency domain by setting bins with too small
// signal power to zero.
// Collect the sum of the powers from surviving bins and mute the entire
// block in case the total power is below threshold.
// Here we use the agc factor as determined from the the
// previous block and attenuated with the decay factor.
// Note that the agc factor is limited to 20 dB at this point.
// A big pulse will not kill the signal for a long time!!
t1=0;
k=0;
for(i=tx_filter_ia1; i<mic_fftsize; i++)
  {
  t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
     micfft[micfft_px+2*i+1]*micfft[micfft_px+2*i+1];
  if(t2 > micfft_bin_minpower)
    {
    k++;
    t1+=t2;
    }
  else
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  }
for(i=0; i<=tx_filter_ib1; i++)
  {
  t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
     micfft[micfft_px+2*i+1]*micfft[micfft_px+2*i+1];
  if(t2 > micfft_bin_minpower)
    {
    k++;
    t1+=t2;
    }
  else
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  }
totpwr[0]=t1;    
return k;
}

int tx_ssb_step4(int no_of_bins, float *pmax, float *prat1a, float *prat1b)
{
int i, k, mm, panew;
char mute_current, mute_previous;
float r1, t1, t2;
float local_af_gain;
int muted;
float maxa, maxb;
pmax[0]=0;
prat1a[0]=0;
prat1b[0]=0;
maxa=0;
maxb=0;
muted=0;
panew=(cliptimf_pa+mic_fftsize)&micfft_mask;
if(no_of_bins == 0)
  {
  mute_current=TRUE;
  }
else
  {  
  mute_current=FALSE;
  }
cliptimf_mute[cliptimf_pa/mic_fftsize]=mute_current;
mute_previous=cliptimf_mute[((cliptimf_pa-mic_fftsize+micfft_bufsiz)&
                           micfft_mask)/mic_fftsize];
// We do nothing in case both the current transform and the
// previous one are muted.
if( (mute_current&mute_previous) == TRUE)
  {
  muted=mic_fftsize/2;
  for(i=0; i<mic_fftsize; i++)
    {
    cliptimf[panew+i]=0;
    }
  if(sw_cliptimf)
    {
    for(i=0; i<mic_fftsize; i++)
      {
      cliptimf_oscilloscope[panew+i]=0;
      }
    }  
  }
else
  {  
  local_af_gain=af_gain;
//  if( (mute_current|mute_previous) == FALSE)
    {
// None of the transforms is muted. 
// Do normal (continous) back transformation.
// Compute the largest power and store in t2.
    t2=0;
    local_af_gain*=tx_agc_factor;
    t1=0;
    for(i=0; i<mic_fftsize; i+=2)
      {
      cliptimf[cliptimf_pa+i  ]=tx_ph1*
                   (cliptimf[cliptimf_pa+i  ]+local_af_gain*micfft[micfft_px+i  ]);
      cliptimf[cliptimf_pa+i+1]=tx_ph1*
                   (cliptimf[cliptimf_pa+i+1]+local_af_gain*micfft[micfft_px+i+1]);
      t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
         cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
      if(t2>t1)t1=t2;
      }
    if(tx_fft_phase_flag)local_af_gain*=-1;
// This part will be added next time. Look for the levels already
// now but note that we have to divide by the window function
// this is the cos squared part. Next time the sin squared part
// will be added.
    mm=45*mic_fftsize/100;
    k=mic_fftsize/2;
    for(i=0; i<mm; i++)
      {
      cliptimf[panew+2*i  ]=local_af_gain*micfft[micfft_px+mic_fftsize+2*i  ];
      cliptimf[panew+2*i+1]=local_af_gain*micfft[micfft_px+mic_fftsize+2*i+1];
      t2=cliptimf[panew+2*i  ]*cliptimf[panew+2*i  ]+
         cliptimf[panew+2*i+1]*cliptimf[panew+2*i+1];
      t2/=(micfft_winfac*micfft_win[k-i+1])*(micfft_winfac*micfft_win[k-i+1]);;
      if(t2>t1)t1=t2;
      }
// Here the window function is small so we will not divide by it.      
    for(i=mm; i<k; i++)
      {
      cliptimf[panew+2*i  ]=local_af_gain*micfft[micfft_px+mic_fftsize+2*i  ];
      cliptimf[panew+2*i+1]=local_af_gain*micfft[micfft_px+mic_fftsize+2*i+1];
      }
// change the AGC factor if the peak amplitude is above 1.0
// First turn down gain with the cosine squared window for
// 25% of a transform. then use constant reduced gain.
    if(t1 > 1)
      {
      t2=1/sqrt(t1);
      if(sw_cliptimf)
        {
        for(i=0; i<mic_fftsize/4; i++)
          {
          t1=micfft_winfac*micfft_win[2*i];
          cliptimf[cliptimf_pa+2*i  ]*=t2*t1+(1-t1);
          cliptimf[cliptimf_pa+2*i+1]*=t2*t1+(1-t1);
          cliptimf_oscilloscope[cliptimf_pa+2*i  ]=cliptimf[cliptimf_pa+2*i  ];
          cliptimf_oscilloscope[cliptimf_pa+2*i+1]=cliptimf[cliptimf_pa+2*i+1];
          r1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          prat1a[0]+=r1;
          if(r1 > maxa)maxa=r1;
          cliptimf[cliptimf_pa+2*i  ]*=rf_gain;
          cliptimf[cliptimf_pa+2*i+1]*=rf_gain;
          t1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          if(t1>pmax[0])pmax[0]=t1;
          if(t1 > 1.0F)
            {
            t1=1/sqrt(t1);
            cliptimf[cliptimf_pa+2*i  ]*=t1;
            cliptimf[cliptimf_pa+2*i+1]*=t1;
            t1=1.0F;
            }
          if(t1 < tx_minlevel)
            {
            muted++;
            cliptimf[cliptimf_pa+2*i  ]=0;
            cliptimf[cliptimf_pa+2*i+1]=0;
            t1=0;
            }              
          prat1b[0]+=t1;
          if(t1 > maxb)maxb=t1;
          }
        for(i=mic_fftsize/4; i<mic_fftsize/2; i++)
          {
          cliptimf[cliptimf_pa+2*i  ]*=t2;
          cliptimf[cliptimf_pa+2*i+1]*=t2;
          cliptimf_oscilloscope[cliptimf_pa+2*i  ]=cliptimf[cliptimf_pa+2*i  ];
          cliptimf_oscilloscope[cliptimf_pa+2*i+1]=cliptimf[cliptimf_pa+2*i+1];
          r1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          prat1a[0]+=r1;
          if(r1 > maxa)maxa=r1;
          cliptimf[cliptimf_pa+2*i  ]*=rf_gain;
          cliptimf[cliptimf_pa+2*i+1]*=rf_gain;
          t1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          if(t1>pmax[0])pmax[0]=t1;
          if(t1 > 1.0F)
            {
            t1=1/sqrt(t1);
            cliptimf[cliptimf_pa+2*i  ]*=t1;
            cliptimf[cliptimf_pa+2*i+1]*=t1;
            t1=1.0F;
            }        
          if(t1 < tx_minlevel)
            {
            muted++;
            cliptimf[cliptimf_pa+2*i  ]=0;
            cliptimf[cliptimf_pa+2*i+1]=0;
            t1=0;
            }              
          prat1b[0]+=t1;
          if(t1 > maxb)maxb=t1;
          }
        }
      else
        {
        for(i=0; i<mic_fftsize/4; i++)
          {
          t1=micfft_winfac*micfft_win[2*i];
          r1=(t2*t1+(1-t1))*rf_gain;;
          cliptimf[cliptimf_pa+2*i  ]*=r1;
          cliptimf[cliptimf_pa+2*i+1]*=r1;
          t1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          if(t1>pmax[0])pmax[0]=t1;
          if(t1 > 1.0F)
            {
            t1=1/sqrt(t1);
            cliptimf[cliptimf_pa+2*i  ]*=t1;
            cliptimf[cliptimf_pa+2*i+1]*=t1;
            t1=1.0F;
            }
          if(t1 < tx_minlevel)
            {
            muted++;
            cliptimf[cliptimf_pa+2*i  ]=0;
            cliptimf[cliptimf_pa+2*i+1]=0;
            t1=0;
            }              
          prat1b[0]+=t1;
          if(t1 > maxb)maxb=t1;
          }
        r1=t2*rf_gain;
        for(i=mic_fftsize/4; i<mic_fftsize/2; i++)
          {
          cliptimf[cliptimf_pa+2*i  ]*=r1;
          cliptimf[cliptimf_pa+2*i+1]*=r1;
          t1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
             cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
          if(t1>pmax[0])pmax[0]=t1;
          if(t1 > 1.0F)
            {
            t1=1/sqrt(t1);
            cliptimf[cliptimf_pa+2*i  ]*=t1;
            cliptimf[cliptimf_pa+2*i+1]*=t1;
            t1=1.0F;
            }        
          if(t1 < tx_minlevel)
            {
            muted++;
            cliptimf[cliptimf_pa+2*i  ]=0;
            cliptimf[cliptimf_pa+2*i+1]=0;
            t1=0;
            }              
          prat1b[0]+=t1;
          if(t1 > maxb)maxb=t1;
          }
        }
      for(i=0; i<mic_fftsize; i+=2)
        {
        cliptimf[panew+i  ]*=t2;
        cliptimf[panew+i+1]*=t2;
        }
      tx_agc_factor*=t2;
      }        
    else
      {
      if(sw_cliptimf)
        {
        for(i=0; i<mic_fftsize; i+=2)
          {
          cliptimf_oscilloscope[cliptimf_pa+i  ]=cliptimf[cliptimf_pa+i  ];
          cliptimf_oscilloscope[cliptimf_pa+i+1]=cliptimf[cliptimf_pa+i+1];
          r1=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
             cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
          prat1a[0]+=r1;
          if(r1 > maxa)maxa=r1;
          }
        }
      for(i=0; i<mic_fftsize/2; i++)
        {
        cliptimf[cliptimf_pa+2*i  ]*=rf_gain;
        cliptimf[cliptimf_pa+2*i+1]*=rf_gain;
        t1=cliptimf[cliptimf_pa+2*i  ]*cliptimf[cliptimf_pa+2*i  ]+
           cliptimf[cliptimf_pa+2*i+1]*cliptimf[cliptimf_pa+2*i+1];
        if(t1>pmax[0])pmax[0]=t1;
        if(t1 > 1.0F)
          {
          t1=1/sqrt(t1);
          cliptimf[cliptimf_pa+2*i  ]*=t1;
          cliptimf[cliptimf_pa+2*i+1]*=t1;
          t1=1.0F;
          }        
        if(t1 < tx_minlevel)
          {
          muted++;
          cliptimf[cliptimf_pa+2*i  ]=0;
          cliptimf[cliptimf_pa+2*i+1]=0;
          t1=0;
          }              
        prat1b[0]+=t1;
        if(t1 > maxb)maxb=t1;
        }
      }  
    }
/*
  else
    {
// This is the first or the last transform.
// Multiply the beginning and the end with a sine squared window
// to make the start and end filtered with a sine to power 4
// window in total.    
// The first and last transforms are likely to be incorrect
// because muting in the frequency domain will distort the
// signal if only the very strongest bins have survived
// the muting process. This is particularly obvious if the
// microphone signal is a keyed tone which is present only
// a short time in the last or first transform. In such a
// situation the signal would be extended over the entire
// transform in case only a single bin would survive.
// The added sine squared window will remove the abrupt
// start that the signal otherwise would have.
    if(mute_current)
      {
      k=0;
      for(i=mic_fftsize-2; i>=0; i-=2)
        {
        r1=micfft_winfac*micfft_win[k];
        cliptimf[cliptimf_pa+i  ]=tx_ph1*cliptimf[cliptimf_pa+i  ]*r1;
        cliptimf[cliptimf_pa+i+1]=tx_ph1*cliptimf[cliptimf_pa+i+1]*r1;
        k++;
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
        if(pmax[0] < MAX_DYNRANGE && t2 < MAX_DYNRANGE)
          {
          cliptimf[cliptimf_pa+i  ]=0;
          cliptimf[cliptimf_pa+i+1]=0;
          }
        else
          {
          prat[0]+=t2;
          if(t2>pmax[0])pmax[0]=t2;
          if(t2 > 1)
            {
            t2=1/sqrt(t2);
            cliptimf[cliptimf_pa+i  ]*=t2;
            cliptimf[cliptimf_pa+i+1]*=t2;
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
            }
          }
        }
      for(i=0; i<mic_fftsize; i++)
        {
        cliptimf[panew+i]=0;
        }
      }
    else // if(mute_previous)
      {
      r1=pow(10.0,0.05*txssb.rf1_gain);
      t2=0;
      r1*=ampinv;
      for(i=0; i<mic_fftsize; i+=2)
        {
        r2=r1*micfft_winfac*micfft_win[i>>1];
        cliptimf[cliptimf_pa+i  ]=tx_ph1*r2*micfft[micfft_px+i  ];
        cliptimf[cliptimf_pa+i+1]=tx_ph1*r2*micfft[micfft_px+i+1];
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
        if(pmax[0] < MAX_DYNRANGE && t2 < MAX_DYNRANGE)
          {
          cliptimf[cliptimf_pa+i  ]=0;
          cliptimf[cliptimf_pa+i+1]=0;
          }
        else
          {
          prat[0]+=t2;
          if(t2>pmax[0])pmax[0]=t2;
          if(t2 > 1)
            {
            t2=1/sqrt(t2);
            cliptimf[cliptimf_pa+i  ]*=t2;
            cliptimf[cliptimf_pa+i+1]*=t2;
            }
          }
        }
      if( (tx_filter_ia1&1) != 0)r1*=-1;
      for(i=0; i<mic_fftsize; i++)
        {
        cliptimf[panew+i]=r1*micfft[micfft_px+mic_fftsize+i];
        }
      }
    }
*/
  }

if(sw_cliptimf)
  {
  prat1b[0]/=mic_sizhalf;
  prat1a[0]/=mic_sizhalf;
  prat1b[0]=maxb/prat1b[0];
  prat1a[0]=maxa/prat1a[0];
  }
cliptimf_pa=panew;
micfft_px=(micfft_px+micfft_block)&micfft_mask;
return muted;
}

void tx_ssb_step5(void)
{
int i, j, pb, nx;
while( ((cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask) >= 
                                                           2*mic_fftsize)
  {                                                  
  pb=(cliptimf_px+2*(mic_fftsize-1))&micfft_mask;
  nx=cliptimf_px/mic_fftsize;
  if(cliptimf_mute[nx]==FALSE)
    {
    j=mic_fftsize-1;
    for(i=0; i<mic_sizhalf; i++)
      {
      clipfft[clipfft_pa+2*i  ]=cliptimf[cliptimf_px+2*i  ]*micfft_win[i]; 
      clipfft[clipfft_pa+2*i+1]=cliptimf[cliptimf_px+2*i+1]*micfft_win[i]; 
      clipfft[clipfft_pa+2*j  ]=cliptimf[pb  ]*micfft_win[i]; 
      clipfft[clipfft_pa+2*j+1]=cliptimf[pb+1]*micfft_win[i]; 
      j--;
      pb-=2;
      }      
    fftforward(mic_fftsize, mic_fftn, &clipfft[clipfft_pa], 
                                             micfft_table, micfft_permute,FALSE);
    if(tx_setup_flag == TRUE)
      {
      if(tx_setup_mode == TX_SETUP_RF1CLIP)
        {
        if(tx_display_mode == TX_DISPLAY_CLIPFFT_WIDE)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            display_color[0]=10;
            show_txfft(&clipfft[clipfft_pa],0,0,mic_fftsize);
            tx_oscilloscope_time=recent_time;
            }
          }
        if(tx_display_mode == TX_DISPLAY_CLIPFFT_NARROW)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            display_color[0]=11;
            contract_tx_spectrum(&clipfft[clipfft_px]); 
            show_txfft(tx_oscilloscope_tmp,0,0,micdisplay_fftsize);
            tx_oscilloscope_time=recent_time;
            }
          }  
        }
      }
    for(i=tx_filter_ib1+1; i<tx_filter_ia1; i++)
      {
      clipfft[clipfft_pa+2*i  ]=0;  
      clipfft[clipfft_pa+2*i+1]=0;  
      }
    clipfft[clipfft_pa+2*tx_filter_ib1  ]*=0.5F;
    clipfft[clipfft_pa+2*tx_filter_ib1+1]*=0.5F;
    clipfft[clipfft_pa+2*tx_filter_ia1  ]*=0.5F;
    clipfft[clipfft_pa+2*tx_filter_ia1+1]*=0.5F;
    clipfft[clipfft_pa+2*tx_filter_ib1-2]*=0.875F;
    clipfft[clipfft_pa+2*tx_filter_ib1-1]*=0.875F;
    clipfft[clipfft_pa+2*tx_filter_ia1+2]*=0.875F;
    clipfft[clipfft_pa+2*tx_filter_ia1+3]*=0.875F;
    if(tx_setup_flag == TRUE)
      {
      if(tx_setup_mode == TX_SETUP_RF1CLIP)
        {
        if(tx_display_mode == TX_DISPLAY_CLIPFFT_WIDE_FILT)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            display_color[0]=13;
            show_txfft(&clipfft[clipfft_pa],0,0,mic_fftsize);
            tx_oscilloscope_time=recent_time;
            }
          }
        if(tx_display_mode == TX_DISPLAY_CLIPFFT_NARROW_FILT)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            display_color[0]=14;
            contract_tx_spectrum(&clipfft[clipfft_px]); 
            show_txfft(tx_oscilloscope_tmp,0,0,micdisplay_fftsize);
            tx_oscilloscope_time=recent_time;
            }
          }  
        }
      }
    clipfft_mute[clipfft_pa/alc_block]=FALSE;
    }
  else
    {
    clipfft_mute[clipfft_pa/alc_block]=TRUE;
    }      
  cliptimf_px=(cliptimf_px+mic_fftsize)&micfft_mask;
  clipfft_pa=(clipfft_pa+alc_block)&alc_mask;
  }
}

float tx_ssb_step6(float *prat)
{
int i, k, pb, mute_previous;
float t1, t2, pmax;
mute_previous=clipfft_mute[((clipfft_px-alc_block+alc_bufsiz)
                                                     &alc_mask)/alc_block];
pmax=0;
prat[0]=0;
if(clipfft_mute[clipfft_px/alc_block] == FALSE)
  {
  alctimf_mute[alctimf_pa/alc_fftsize]=FALSE;
  if(mic_fftsize < alc_fftsize)
    {
    i=tx_filter_ia1;
    k=tx_filter_ia2;
    while(i<mic_fftsize)
      {
      clipfft[clipfft_px+2*k  ]=clipfft[clipfft_px+2*i  ];
      clipfft[clipfft_px+2*k+1]=clipfft[clipfft_px+2*i+1];
      i++;
      k++;
      }      
    }  
  for(i=tx_filter_ib1+1; i<tx_filter_ia2; i++)
    {
    clipfft[clipfft_px+2*i]=0;
    clipfft[clipfft_px+2*i+1]=0;
    }      
  fftback(alc_fftsize, alc_fftn, &clipfft[clipfft_px], 
                                         alc_table, alc_permute, FALSE);
  if(mute_previous) 
    {
    for(i=0; i<alc_sizhalf; i++)
      {
      alctimf[alctimf_pa+2*i  ]=clipfft[clipfft_px+2*i  ];

      alctimf[alctimf_pa+2*i+1]=clipfft[clipfft_px+2*i+1];
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  else
    {
    for(i=0; i<alc_sizhalf; i++)
      {
      alctimf[alctimf_pa+2*i  ]+=clipfft[clipfft_px+2*i  ];
      alctimf[alctimf_pa+2*i+1]+=clipfft[clipfft_px+2*i+1];
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  }
else
  {
  if(mute_previous)
    {
    tx_forwardpwr=0;
    alctimf_mute[alctimf_pa/alc_fftsize]=TRUE;
    }
  else
    {
    alctimf_mute[alctimf_pa/alc_fftsize]=FALSE;
    for(i=0; i<alc_sizhalf; i++)
      {
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  }  
if( alctimf_mute[alctimf_pa/alc_fftsize] == FALSE)
  {
// Store an exponential power fall-off with the same time constant,
// in the reverse direction.
  i=alc_sizhalf-1;
  t1=0;
  while(i >= 0)
    {
    t1*=txpwr_decay;
    if(t1 < alctimf_pwrf[(alctimf_pa>>1)+i])
      {
      t1=alctimf_pwrf[(alctimf_pa>>1)+i];
      }
    alctimf_pwrd[(alctimf_pa>>1)+i]=t1;
    i--;
    }
  pb=alctimf_pa;
  t1=alctimf_pwrd[(pb>>1)];
  i=0;
  while( pb != ((int)(alctimf_fx+4+alc_bufsiz)&alc_mask) && i == 0)
    {
    t1*=txpwr_decay;
    pb=(pb+alc_mask)&alc_mask;
    if(t1 > alctimf_pwrf[(pb>>1)])
      {
      alctimf_pwrd[(pb>>1)]=t1;
      }
    else
      {
      i=1;
      }  
    }
  }  
alctimf_pa=(alctimf_pa+alc_fftsize)&alc_mask;
if(clipfft_mute[clipfft_px/alc_block] == FALSE)
  {
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pa+i]=clipfft[clipfft_px+alc_fftsize+i];
    }
  }
else
  {
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pa+i]=0;
    }
  }
clipfft_px=(clipfft_px+alc_block)&alc_mask;
return pmax;
}

float tx_ssb_step7(float *prat)
{
int i, p0;
float r1, t1, t2, t3, pmax;
pmax=0;
if(alctimf_mute[alctimf_pb/alc_fftsize]==FALSE)
  {
  p0=(alctimf_pb-2+alc_bufsiz)&alc_mask;
  t1=alctimf[p0-2]*alctimf[p0-2]+
     alctimf[p0-1]*alctimf[p0-1];
  t2=alctimf[p0  ]*alctimf[p0  ]+
     alctimf[p0+1]*alctimf[p0+1];
  prat[0]=0;
  for(i=0; i<alc_sizhalf; i++)
    {
    r1=alctimf_pwrd[(alctimf_pb>>1)+i];    
    if(r1 > 1)
      {
      r1=1/sqrt(r1);
      alctimf[alctimf_pb+2*i  ]*=r1;
      alctimf[alctimf_pb+2*i+1]*=r1;
      }
    t3=alctimf[alctimf_pb+2*i  ]*alctimf[alctimf_pb+2*i  ]+
       alctimf[alctimf_pb+2*i+1]*alctimf[alctimf_pb+2*i+1];
// We do not want more than MAX_DYNRANGE dynamic range.
// any signal below is just rounding errors in muted periods.
    if(t1 < MAX_DYNRANGE && t2 < MAX_DYNRANGE && t3 < MAX_DYNRANGE)
      {  
      alctimf[p0  ]=0;
      alctimf[p0+1]=0;
      }
    p0=(p0+2)&alc_mask;
    if(t3 > pmax)pmax=t3;
    prat[0]+=t3;
    }
  }
else
  {
  prat[0]=0;
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pb+i]=0;
    }
  }
alctimf_pb=(alctimf_pb+alc_fftsize)&alc_mask;
return pmax;
}

void tx_ssb_step8(void)
{
// tx_ssb_buftim('a');

if(tx_output_flag == 0)
  {
  if(timinfo_flag!=0)tx_ssb_buftim('0');
// Something is wrong with the delay time computations.
// the factor 2 (or possibly a factor of 4) here seems to work,
// but I do not think it is correct - or there are other errors
// in tx_ssb_buftim and associated functions.
  if( 2*(float)((alctimf_pb-(int)(alctimf_fx)+alc_bufsiz)&alc_mask)/
                          (tx_pre_resample*(float)ui.tx_ad_speed) > 
                        0.001*txssb.delay_margin+SSB_DELAY_EXTRA)
    {
    lir_sched_yield();
    tx_output_flag=1;
    tx_resamp_pa=0;
    tx_resamp_px=0;
    txout_px=txout_pa;
// tx_ssb_buftim('X');
    }
  }  
if(tx_output_flag > 0)
  {
  if(timinfo_flag!=0)tx_ssb_buftim('1');  
  resample_tx_output();
  }
}

void use_tx_resamp(float ampfac)
{
int i, j, k, m, mm;
float t1, t2, t3, r1, r2, pilot_ampl;
double dt1;
// The latest half block of data resides in tx_resamp[alc_fftsize]
// to tx_resamp[2*alc_fftsize-1]
// Copy the previous block from resamp_tmp and multiply with
// the window function.
if(tx_setup_flag == TRUE)
  {
  if(tx_setup_mode == TX_SETUP_ALC)
    {
    if(tx_display_mode == TX_DISPLAY_RESAMP)
      {
      mm=tx_oscilloscope_tmp_size-1;
      for(i=0; i<alc_fftsize; i+=2)
        {
        tx_oscilloscope_tmp[tx_oscilloscope_ptr  ]=resamp_tmp[i  ];
        tx_oscilloscope_tmp[tx_oscilloscope_ptr+1]=resamp_tmp[i+1];
        tx_oscilloscope_ptr=(tx_oscilloscope_ptr+2)&mm;
        }
      if(current_time()-tx_oscilloscope_time >0.2F)
        {
        k=(tx_oscilloscope_ptr-2*(screen_width+mic_fftsize)+
                                       tx_oscilloscope_tmp_size)&mm;
        for(i=0; i<screen_width; i++)
          {
          lir_setpixel(i,tx_oscilloscope[2*i  ],0);
          lir_setpixel(i,tx_oscilloscope[2*i+1],0);
          
          tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                          tx_oscilloscope_tmp[k  ]+tx_oscilloscope_zero;   
          tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                          tx_oscilloscope_tmp[k+1]+tx_oscilloscope_zero;   
          if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
          if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
          lir_setpixel(i,tx_oscilloscope[2*i  ],13);
          lir_setpixel(i,tx_oscilloscope[2*i+1],10);
          k=(k+2)&mm;
          }
        tx_oscilloscope_time=current_time();

        }
      }
    }    
  }
dt1=1/sqrt(tx_daout_sin*tx_daout_sin+tx_daout_cos*tx_daout_cos);
tx_daout_sin*=dt1;
tx_daout_cos*=dt1;
k=alc_sizhalf;
m=k-1;
t1=0;
for(i=0; i<alc_sizhalf; i++)
  {
  tx_resamp[2*i  ]=resamp_tmp[2*i  ]*alc_win[i]; 
  tx_resamp[2*i+1]=resamp_tmp[2*i+1]*alc_win[i]; 
  resamp_tmp[2*i  ]=ampfac*tx_resamp[2*k  ];
  resamp_tmp[2*i+1]=ampfac*tx_resamp[2*k+1];
  tx_resamp[2*k  ]*=ampfac*alc_win[m];
  tx_resamp[2*k+1]*=ampfac*alc_win[m];
  m--;
  k++;
  } 
fftforward(alc_fftsize, alc_fftn, tx_resamp, alc_table, alc_permute, FALSE);
// Clear the spectrum outside the desired passband.
// These data points should be very small.
k=txout_fftsize-alc_fftsize;
if(k > 0)
  {
  for(i=alc_sizhalf; i<alc_fftsize; i++)
    {
    tx_resamp[2*(k+i)  ]=tx_resamp[2*i  ];
    tx_resamp[2*(k+i)+1]=tx_resamp[2*i+1];
    }
  i=tx_filter_ib3+1;
  k=tx_filter_ia3-1;
  t1=1;
  t3=1./tx_filter_ib3;
  t2=0;
  while(k>=i && t1 >0)
    {
    tx_resamp[2*k]*=t1;
    tx_resamp[2*k+1]*=t1;
    tx_resamp[2*i]*=t1;
    tx_resamp[2*i+1]*=t1;
    t2+=t3;
    t1=1-t2*t2;
    i++;
    k--;
    }
  while(k>=i)
    {
    tx_resamp[2*k]=0;
    tx_resamp[2*k+1]=0;
    tx_resamp[2*i]=0;
    tx_resamp[2*i+1]=0;
    i++;
    k--;
    }
  }
if(k < 0)
  {
  lirerr(77676);
  }
// Now we have the output in the frequency domain at the size txout_fftsize
// with a sampling rate that fits our D/A converter. Transform it back
// to the time domain into txout.
if(tx_onoff_flag !=0)
  {
  fftback(txout_fftsize, txout_fftn, tx_resamp, txout_table,
                                                  txout_permute, FALSE);
  for(i=0; i<txout_fftsize; i+=2)
    {
    tx_pilot_tone*=-1;
    if(tx_resamp_maxamp < 0.0001 && tx_resamp_old_maxamp < 0.0001)
      {
      pilot_ampl=0;
      hware_set_ptt(FALSE);
      }
    else
      {
      pilot_ampl=tx_pilot_tone;
      hware_set_ptt(TRUE);
      }
    dt1=tx_daout_cos;
    tx_daout_cos=dt1*tx_daout_phstep_cos+tx_daout_sin*tx_daout_phstep_sin;
    tx_daout_sin=tx_daout_sin*tx_daout_phstep_cos-dt1*tx_daout_phstep_sin;
    r1=tx_resamp[i  ]+txout_tmp[i  ];
    r2=tx_resamp[i+1]+txout_tmp[i+1];
    t1=tx_daout_cos*tx_output_amplitude;
    t2=-tx_daout_sin*tx_output_amplitude;
    txout[txout_pa+i  ]=r1*t1+r2*t2+pilot_ampl;
    txout[txout_pa+i+1]=-r1*t2+r2*t1-pilot_ampl;
    }
  for(i=0; i<txout_fftsize; i++)
    {
    txout_tmp[i]=tx_resamp[txout_fftsize+i];
    }
  }
else
  {
  for(i=0; i<txout_fftsize; i++)
    {
    txout_tmp[i]=0;
    txout[txout_pa+i]=0;
    }   
  }
txout_pa=(txout_pa+txout_fftsize)&txout_mask;
// Compute the total time from tx input to tx output and
// express it as samples of the tx input clock and store in tx_wsamps. 
tx_wsamps=lir_tx_input_samples();
tx_wsamps+=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
// The microphone input is real data at the soundcard speed.
// The complex output of micfft is at half the speed
// and transforms overlap by 50%.
tx_wsamps*=2;
tx_wsamps+=((micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask)/2;
tx_wsamps+=(cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask; 
j=((clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask)/2;
j+=(alctimf_pa-((int)(alctimf_fx))+4+alc_bufsiz)&alc_mask;
tx_wsamps+=j/tx_pre_resample;
// The maximum amplitude should ideally be 1.0 but since
// a litte power outside the passband was removed in the last 
// backwards transform, a small amount of re-peaking could occur
// Set the maximum level to TX_DA_MARGIN of full scale to make sure
// D/A converters will not overflow.
tx_send_to_da();
}

void resample_tx_output(void)
{
int i1, i2, i3, i4, mask2;
float t1, t2, t3, t4, t5, t6, t7, r1, r2;
float a1, a2, rdiff;
// The microphone is best sampled at a low sampling speed since
// we do not want much bandwidth nor dynamic range.
// For the output the requirements depend on the circumstances.
// In case the transmitter is used in a wideband up-conversion
// system, the output sampling rate may be very high and the
// requirements on the signal purity might be very high.
// We do the resampling in two steps. First do a fractional
// resampling to a speed near the speed of the microphone
// signal (but perhaps some power of two below the desired output
// speed as indicated by tx_output_upsample.)
// Subsequently, use an FFT to sample up the signal to the
// desired frequency while filtering out the false signals
// that non-linearities in the fractional resampling have
// introduced.
mask2=alc_mask-1;
begin:;
r1=tx_resamp_pa/tx_resample_ratio;
resamp:;
r2=(tx_resamp_pa+2)/tx_resample_ratio;
i2=alctimf_fx+r1+1;
i3=alctimf_fx+r2+1;
i2&=mask2;
i3&=mask2;
if(abs(i3-i2) > 2)
  {
  i2=alctimf_fx+(r1+r2)/2;
  i3=i2+2;
  i2&=mask2;
  i3&=mask2;
  }
else
  {
  if(i3==i2)
    {
    i2=alctimf_fx+r1;
    i2&=mask2;
    if(i3==i2)
      {
      i3=i2+2;
      i3&=mask2;
      }
    }
  }        
i4=(i3+2)&(mask2);
i1=(i2+mask2)&(mask2);
if( ((alctimf_pb-i4+alc_bufsiz)&alc_mask) > 4)
  {
  if(alctimf_mute[i2/alc_fftsize]==FALSE ||
     alctimf_mute[i3/alc_fftsize]==FALSE)
    {
    rdiff=r1+alctimf_fx-i2;
    if(rdiff > alc_bufsiz/2)
      {
      rdiff-=alc_bufsiz;
      }
    rdiff/=2; 
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  a1=-rdiff *   (rdiff-1)*(rdiff-2)*alctimf[i1]/6
//     +(rdiff+1)*(rdiff-1)*(rdiff-2)*alctimf[i2]/2
//     -(rdiff+1)* rdiff   *(rdiff-2)*alctimf[i3]/2
//     +(rdiff+1)* rdiff   *(rdiff-1)*alctimf[i4]/6; 
// Rewrite slightly to save a few multiplications - do not
// think the compiler is smart enough to do it for us.
    t1=rdiff-1;
    t2=rdiff-2;
    t3=rdiff+1;
    t4=t1*t2;
    t5=t3*rdiff;
    t6=rdiff*t4;
    t4=t3*t4;
    t7=t5*t2;
    t5=t5*t1;
    a1=((t5*alctimf[i4  ]-t6*alctimf[i1  ])/3+t4*alctimf[i2  ]-t7*alctimf[i3  ])/2;
    a2=((t5*alctimf[i4+1]-t6*alctimf[i1+1])/3+t4*alctimf[i2+1]-t7*alctimf[i3+1])/2;
// The curve fitting is (of course) just an approximation.
// Make sure we do not go outside our range!
    t2=a1*a1+a2*a2;
    if(t2 > tx_resamp_maxamp)tx_resamp_maxamp=t2;

    if(t2 > tx_resamp_ampfac1)tx_resamp_ampfac1=t2;
    tx_resamp[tx_resamp_pa+alc_fftsize  ]=a1;
    tx_resamp[tx_resamp_pa+alc_fftsize+1]=a2;
    }
  else
    {
    tx_resamp[tx_resamp_pa+alc_fftsize  ]=0;
    tx_resamp[tx_resamp_pa+alc_fftsize+1]=0;
    }
  r1=r2;
  tx_resamp_pa+=2;
  if(tx_resamp_pa >= alc_fftsize)
    {
    alctimf_fx+=r2;
    if(alctimf_fx>alc_bufsiz)alctimf_fx-=alc_bufsiz;
    t1=tx_resamp_ampfac1;
    if(t1 < tx_resamp_ampfac2)t1=tx_resamp_ampfac2;
    use_tx_resamp(sqrt(1/t1));
    tx_resamp_old_maxamp=tx_resamp_maxamp;
    tx_resamp_maxamp=0;
    tx_resamp_ampfac2=tx_resamp_ampfac1;
    tx_resamp_ampfac1=1;
    tx_resamp_pa=0;
    goto begin;
    }
  goto resamp;  
  }
}

void tx_bar(int xt,int yt,int val1, int val2)
{
int x, y;
x=xt*text_width;
y=yt*text_height+1;
if(val1 == val2)
  {
  if(val1 >= 0)return;
  lir_fillbox(x,y,TX_BAR_SIZE,text_height-2,7);
  return;
  }
if(val2 > val1)
  { 
  lir_fillbox(x+val1,y,val2-val1,text_height-2,12);
  }
else
  {  
  lir_fillbox(x+val2,y,val1-val2,text_height-2,7);
  } 
}

void show_txfft(float *z, float lim, int type, int siz)
{
int i, ia, ib, k, m;
int pixels_per_bin, bins_per_pixel;
float t1,t2;
char color;
short int *trc;
if(type >= MAX_DISPLAY_TYPES)
  {
  lirerr(762319); 
  return;
  }
trc=&txtrace[type*tx_show_siz];  
ia=0;
t2=0;
pixels_per_bin=(screen_width-1)/siz;
bins_per_pixel=1;
while(pixels_per_bin == 0)
  {
  siz/=2;
  pixels_per_bin=(screen_width-1)/siz;
  bins_per_pixel*=2;
  }
ib=pixels_per_bin;
for(i=1; i<siz; i++)
  {
  lir_line(ia,trc[i-1],ib,trc[i],0);
  ia=ib;
  ib+=pixels_per_bin;
  }
ia=0;
ib=pixels_per_bin;
m=0;
for(i=0; i<siz; i++)
  {
  t1=0;
  for(k=0; k<bins_per_pixel; k++)
    {
    t1=z[2*m]*z[2*m]+z[2*m+1]*z[2*m+1];
    m++;
    }
  t1/=bins_per_pixel;  
  k=-0.07*log10(txtrace_gain*(t1+0.0000000000001))*screen_height;
  if(k>txtrace_height)k=txtrace_height;
  if(k<0)k=0;
  trc[i]=k+screen_height-txtrace_height-1;
  if(i>0)
    {
    color=display_color[type];
    if(type==0)
      {
      if(t2 < lim)
        {
        color=12;
        }
      }
    lir_line(ia,trc[i-1],ib,trc[i],color);
    ia=ib;
    ib+=pixels_per_bin;
    }
  t2=t1;
  }
}

void spproc_setup(void)
{
char s[80];
int i, k, n, m, mm;
int ad_pix1;
int ad_pix2;
int mute_pix1;
int mute_pix2;
int micagc_pix1;
int micagc_pix2;
int rf1clip_pix1;
int rf1clip_pix2;
int alc_pix1;
int alc_pix2;
int valid_bins, muted_points;
float t1, t2, r2;
float prat, prat1a, prat1b, pmax;
int old_display_ptr;
int old_admax[DISPLAY_HOLD_SIZE];
float old_micagc[DISPLAY_HOLD_SIZE];
float old_rf1clip[DISPLAY_HOLD_SIZE];
float old_prat1a[DISPLAY_HOLD_SIZE];
float old_prat1b[DISPLAY_HOLD_SIZE];
float old_pmax1[DISPLAY_HOLD_SIZE];
float old_prat2[DISPLAY_HOLD_SIZE];
float old_pmax2[DISPLAY_HOLD_SIZE];
float old_prat3[DISPLAY_HOLD_SIZE];
float old_pmax3[DISPLAY_HOLD_SIZE];
int default_spproc_no;
rx_mode=MODE_SSB;
set_hardware_tx_frequency();
default_spproc_no=tg.spproc_no;
txtrace_gain=0.3;
tx_setup_flag=TRUE;
tx_setup_mode=TX_SETUP_AD;
tx_display_mode=TX_DISPLAY_MICTIMF;
if(read_txpar_file()==FALSE)
  {
  set_default_spproc_parms();
  }
restart:
sw_cliptimf=( (tx_setup_mode == TX_SETUP_MUTE ||
               tx_setup_mode == TX_SETUP_MICAGC ||
               tx_setup_mode == TX_SETUP_RF1CLIP) &&
               (tx_display_mode == TX_DISPLAY_AGC ||
                tx_display_mode == TX_DISPLAY_CLIP ));
tx_oscilloscope_ptr=0;
if(txssb.minfreq < SSB_MINFQ_LOW)txssb.minfreq=SSB_MINFQ_LOW;
if(txssb.minfreq > SSB_MINFQ_HIGH)txssb.minfreq=SSB_MINFQ_HIGH;
if(txssb.maxfreq < SSB_MAXFQ_LOW)txssb.maxfreq=SSB_MAXFQ_LOW;
if(txssb.maxfreq > SSB_MAXFQ_HIGH)txssb.maxfreq=SSB_MAXFQ_HIGH;
if(txssb.slope > SSB_MAXSLOPE)txssb.slope=SSB_MAXSLOPE;
if(txssb.slope < SSB_MINSLOPE)txssb.slope=SSB_MINSLOPE;
if(txssb.bass > SSB_MAXBASS)txssb.bass=SSB_MAXBASS;
if(txssb.bass < SSB_MINBASS)txssb.bass=SSB_MINBASS;
if(txssb.treble > SSB_MAXTREBLE)txssb.treble=SSB_MAXTREBLE;
if(txssb.treble < SSB_MINTREBLE)txssb.treble=SSB_MINTREBLE;
if(txssb.mic_f_threshold < 0)txssb.mic_f_threshold=0;
if(txssb.mic_f_threshold > SSB_MAX_MICF)txssb.mic_f_threshold=SSB_MAX_MICF;
if(txssb.mic_t_threshold < 0)txssb.mic_t_threshold=0;
if(txssb.mic_t_threshold > SSB_MAX_MICT)txssb.mic_t_threshold=SSB_MAX_MICT;
if(txssb.mic_gain < 0)txssb.mic_gain=0;
if(txssb.mic_gain > SSB_MAX_MICGAIN)txssb.mic_gain=SSB_MAX_MICGAIN;
if(txssb.mic_agc_time < 0)txssb.mic_agc_time=0;
if(txssb.mic_agc_time > SSB_MAX_MICAGC_TIME)
                                   txssb.mic_agc_time=SSB_MAX_MICAGC_TIME;
if(txssb.rf1_gain < SSB_MIN_RF1_GAIN)txssb.rf1_gain=SSB_MIN_RF1_GAIN;
if(txssb.rf1_gain > SSB_MAX_RF1_GAIN)txssb.rf1_gain=SSB_MAX_RF1_GAIN;
tg_basebfreq_hz=0;
tx_hware_fqshift=txssb.frequency_shift;
init_txmem_spproc();
if(kill_all_flag)return;
//make_tx_phstep();
tx_show_siz=micsize;
if(tx_show_siz < alc_fftsize)tx_show_siz=alc_fftsize;
if(tx_show_siz < txout_fftsize)tx_show_siz=txout_fftsize;
txtrace=malloc(MAX_DISPLAY_TYPES*tx_show_siz*sizeof(short int));
if(txtrace == NULL)
  {
  lirerr(778543);
  return;
  }
for(i=0; i<MAX_DISPLAY_TYPES*tx_show_siz; i++)txtrace[i]=screen_height-1;
txtrace_height=screen_height-text_height*(TXPAR_INPUT_LINE+1);
linrad_thread_create(THREAD_TX_INPUT);
old_display_ptr=0;
for(i=0; i<DISPLAY_HOLD_SIZE; i++)
  {
  old_admax[i]=0;
  old_micagc[i]=1;
  old_rf1clip[i]=0;
  old_prat1a[i]=90;
  old_prat1b[i]=90;
  old_pmax1[i]=0;
  old_prat2[i]=90;
  old_pmax2[i]=0;
  old_prat3[i]=90;
  old_pmax3[i]=0;
  }
clear_screen();
tx_ad_maxamp=0;
ad_pix1=0;
ad_pix2=0;
micagc_pix1=0;
micagc_pix2=0;
mute_pix1=0;
mute_pix2=0;
rf1clip_pix1=0;
rf1clip_pix2=0;
alc_pix1=0;
alc_pix2=0;
sprintf(s,"FFT:%.1f ms  ADC:%.1f ms  DAC:%.1f ms ph %d",
                      1000.F*(float)mictimf_block/(float)ui.tx_ad_speed,
                      1000.F*(float)snd[TXAD].block_frames/(float)ui.tx_ad_speed,
                      1000.F*(float)snd[TXDA].block_frames/(float)ui.tx_da_speed,
                      tx_fft_phase_flag);
lir_text(40,screen_last_line,s);
settextcolor(14);
make_txproc_filename();
sprintf(s,"Press 'S' to save %s, 'R' to read.  '+' or '-' to change file no.",
                                                 txproc_filename);
lir_text(0,TX_BAR_AD_LINE-3,s);
lir_text(0,TX_BAR_AD_LINE-2,
  "Arrow up/down to change spectrum scale.  F3 to toggle output");
if(tx_onoff_flag == 0)
  {
  settextcolor(12);
  }
else
  {
  settextcolor(10);
  }
lir_text(61,TX_BAR_AD_LINE-2,"ON");
settextcolor(7);
lir_text(63,TX_BAR_AD_LINE-2,"/");
if(tx_onoff_flag == 0)
  {
  settextcolor(10);
  }
else
  {
  settextcolor(12);
  }
lir_text(64,TX_BAR_AD_LINE-2,"OFF");
settextcolor(7);
lir_text(0,TX_BAR_AD_LINE,"'M' =");
lir_text(0,TX_BAR_MUTE_LINE,"'Q' =");
lir_text(0,TX_BAR_MICAGC_LINE,"'A' =");
lir_text(0,TX_BAR_RF1CLIP_LINE,"'C' =");
lir_text(0,TX_BAR_ALC_LINE,"'P' =");
if(tx_setup_mode == TX_SETUP_AD)settextcolor(15); else settextcolor(7);
lir_text(6,TX_BAR_AD_LINE,"Soundcard");
tx_bar(16,TX_BAR_AD_LINE,-1,-1);
if(tx_setup_mode == TX_SETUP_MUTE)settextcolor(15); else settextcolor(7);
lir_text(6,TX_BAR_MUTE_LINE,"Mute");
tx_bar(16,TX_BAR_MUTE_LINE,-1,-1);
if(tx_setup_mode == TX_SETUP_MICAGC)settextcolor(15); else settextcolor(7);
lir_text(6,TX_BAR_MICAGC_LINE,"AGC");
tx_bar(16,TX_BAR_MICAGC_LINE,-1,-1);
if(tx_setup_mode == TX_SETUP_RF1CLIP)settextcolor(15); else settextcolor(7);
lir_text(6,TX_BAR_RF1CLIP_LINE,"RF clip");
tx_bar(16,TX_BAR_RF1CLIP_LINE,-1,-1);
if(tx_setup_mode == TX_SETUP_ALC)settextcolor(15); else settextcolor(7);
lir_text(6,TX_BAR_ALC_LINE,"ALC");
tx_bar(16,TX_BAR_ALC_LINE,-1,-1);
settextcolor(7);
lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_AD_LINE-1,"level");
lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_MUTE_LINE-1,"muted");
lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_MICAGC_LINE-1,"agc act");
lir_text(28+TX_BAR_SIZE/text_width,TX_BAR_MICAGC_LINE-1,"crest");
lir_text(38+TX_BAR_SIZE/text_width,TX_BAR_MICAGC_LINE-1,"peak");
lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_RF1CLIP_LINE-1,"clip act");
lir_text(28+TX_BAR_SIZE/text_width,TX_BAR_RF1CLIP_LINE-1,"crest");
lir_text(38+TX_BAR_SIZE/text_width,TX_BAR_RF1CLIP_LINE-1,"re-peak");
lir_text(48+TX_BAR_SIZE/text_width,TX_BAR_RF1CLIP_LINE-1,"crest");
lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_ALC_LINE-1,"crest");
lir_text(28+TX_BAR_SIZE/text_width,TX_BAR_ALC_LINE-1,"peak");
switch (tx_setup_mode)
  {
  case TX_SETUP_AD:
  lir_text(0,0,
            "Verify AF clip level. (Some device drivers give too few bits)");
  lir_text(0,1,"Use mixer program to set volume for the AF to never reach");
  lir_text(0,2,"its maximum during normal operation.");
  lir_text(0,3,
              "Press 'L' or 'H' to change cut-off frequencies,'F','B' or 'T'");
  lir_text(0,4,"for slope, bass or treble to change frequency response.");
  sprintf(s,
    "Low %.0fHz  High %.0fHz  Slope %ddB/kHz  Bass %ddB  Treble %ddB",
       tx_lowest_bin*txad_hz_per_bin, tx_highest_bin*txad_hz_per_bin,
                                            txssb.slope, txssb.bass, txssb.treble);
  lir_text(0,5,s);
  settextcolor(15);
  lir_text(70,1,"1=Mic raw time funct.");
  settextcolor(12);
  lir_text(70,2,"2=Mic raw spectrum.");
  settextcolor(10);
  lir_text(70,3,"3=Mic filtered spectrum.");
  settextcolor(14);
  lir_text(70,4,"4=Mic filtered wide.");
  settextcolor(11);
  lir_text(70,5,"5=Filter.");
  settextcolor(7);
  break;

  case TX_SETUP_MUTE:
  lir_text(0,0,"Press 'F' to set frequency domain threshold or");
  lir_text(0,1,"Press 'T' to set time domain threshold for signal");
  
  lir_text(0,2,"to become muted when only background noise is present.");
  sprintf(s,"Thresholds:  F=%d dB   T=%d dB",
                                 txssb.mic_f_threshold, txssb.mic_t_threshold);
  lir_text(0,3,s);
  settextcolor(14);
  lir_text(70,0,"1=F muted");
  settextcolor(12);
  lir_text(80,0,"time func.");
  settextcolor(11);
  lir_text(70,1,"2=F muted spectrum.");
  settextcolor(10);
  lir_text(70,2,"3=Mic filtered spectrum.");
  lir_text(70,4,"5=F muted");
  settextcolor(14);
  lir_text(70,3,"4=Mic filtered wide.");
  settextcolor(12);
  lir_text(80,4,"time func AGC.");
  settextcolor(15);
  lir_text(70,5,"6=T,F muted");
  settextcolor(12);
  lir_text(82,5,"AGC clipped.");
  settextcolor(7);
  break;
  
  case TX_SETUP_MICAGC:
  lir_text(0,0,"Press 'V' to set mic volume for suitable AGC action");
  lir_text(0,1,"or 'T' to set decay time constant");
  sprintf(s,"Vol=%d dB   T=%.2f sek",txssb.mic_gain, 0.01*txssb.mic_agc_time);
  lir_text(0,2,s);
  settextcolor(10);
  lir_text(70,4,"5=F muted");
  settextcolor(12);
  lir_text(80,4,"time funct AGC.");
  settextcolor(7);
  settextcolor(15);
  lir_text(70,5,"6=T,F muted");
  settextcolor(12);
  lir_text(82,5,"AGC clipped.");
  settextcolor(7);
  break;
  
  case TX_SETUP_RF1CLIP:
  lir_text(0,0,"Press 'B' to set RF1 gain for desired clip level.");
  lir_text(0,1,"Bar range is 0 to 30 dB");
  sprintf(s,"RF1 gain=%d dB",txssb.rf1_gain);
  lir_text(0,2,s);
  settextcolor(10);
  lir_text(70,0,"1=Clipped spectrum wide.");
  settextcolor(11);
  lir_text(70,1,"2=Clipped spectrum narrow.");
   settextcolor(13);
  lir_text(70,2,"3=Filtered clipped spectrum wide.");
  settextcolor(14);
  lir_text(70,3,"4=Filtered clipped spectrum narrow.");
  settextcolor(10);
  lir_text(70,4,"5=F muted");
  settextcolor(12);
  lir_text(80,4,"time funct AGC.");
  settextcolor(7);
  settextcolor(15);
  lir_text(70,5,"6=T,F muted");
  settextcolor(12);
  lir_text(82,5,"AGC clipped.");
  settextcolor(7);
  break;

  case TX_SETUP_ALC:
  sprintf(s,"Press 'T' to change ALC time constant. Current: %.1f ms.",
                                                    0.1F*txssb.alc_time);
  lir_text(0,0,s);
  sprintf(s,"Press 'U' to change FFT time span. Current: %d ms.",
                                                    txssb.micblock_time);
  lir_text(0,1,s);
  sprintf(s,"Press 'D' to change output delay margin. Current: %d ms.",
                                                    txssb.delay_margin);
  lir_text(0,2,s);
  sprintf(s,"Press 'E' to change frequency shift. Current: %d Hz.",
                                                    txssb.frequency_shift);
  lir_text(0,3,s);
  settextcolor(15);
  lir_text(70,0,"1=Filtered time funct");
  settextcolor(12);
  lir_text(91,0,"before ALC.");
  settextcolor(14);
  lir_text(70,1,"2=Filtered time funct");
  settextcolor(11);
  lir_text(91,1,"with ALC.");
  settextcolor(10);
  lir_text(70,2,"3=ALC factor");
  settextcolor(13);
  lir_text(70,3,"4=Spectrum after ALC.");
  settextcolor(13);
  lir_text(70,4,"5=Re-sampled");
  settextcolor(10);
  lir_text(83,4,"time funct.");
  settextcolor(15);
  lir_text(70,5,"6=Tx out. Resampled spectrum.");
  settextcolor(12);
  lir_text(70,6,"7=Tx out");
  settextcolor(10);
  lir_text(79,6,"time funct.");
  settextcolor(7);
  break;
  }
if(!kill_all_flag)lir_await_event(EVENT_TX_INPUT);
for(i=0; i<10; i++)
  {
  micfft_px=micfft_pa;
  lir_sched_yield();
  while(micfft_px == micfft_pa && !kill_all_flag)
    {
    lir_await_event(EVENT_TX_INPUT);
    t1=0;
    for(k=0;k<100;k++)t1+=sin(0.0001*k);
    lir_sched_yield();
    }
  }
micfft_px=micfft_pa;
while(!kill_all_flag)
  {
  old_display_ptr++;
  if(old_display_ptr>=DISPLAY_HOLD_SIZE)old_display_ptr=0;
// Show the microphone input level. 
  ad_pix2=tx_ad_maxamp;
  if(ui.tx_ad_bytes != 2)
    {
    ad_pix2/=0x10000;
    }  
  old_admax[old_display_ptr]=ad_pix2;
  t1=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1<old_admax[i])t1=old_admax[i];
    }
  t1=20*log10(t1/0x8000);  
  sprintf(s,"%.1fdB  ",t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_AD_LINE, s);
  ad_pix2=(ad_pix2*TX_BAR_SIZE)/0x8000;    
  tx_bar(16,TX_BAR_AD_LINE,ad_pix1,ad_pix2);
  ad_pix1=ad_pix2;
  tx_ad_maxamp=0.95*tx_ad_maxamp;
// **************************************************************
// The setup routine contains the same processing steps as the
// transmit routine, but it has varoius display functions added
// into it. The processing steps are numbered and described below.
// **************************************************************
//
// ************* step1. *************
// Wait for a new block of data from the mic input routine.
// Note that the data arrives as fourier transforms (micfft) and
// that the mic input routine tx_input() already has applied
// the filtering of the microphone signal.
//tx_ssb_buftim('a');  
  lir_sched_yield();
  while(!kill_all_flag && micfft_pa ==  micfft_px)
    {
    if(tx_output_flag == 2 && tx_min_output_samples < 1000000)
      {
      sprintf(s,"D/A margin %.1f ms  ",
             1000.F*(float)(tx_min_output_samples-snd[TXDA].block_frames)/
                                                     (float)ui.tx_da_speed);
      lir_text(0,screen_last_line,s);
      }
    lir_await_event(EVENT_TX_INPUT);
    lir_sched_yield();
    }
  if(tx_agc_factor < 0.1)tx_agc_factor=0.1;
  if(tx_fft_phase_flag)tx_ph1*=-1;
  old_micagc[old_display_ptr]=tx_agc_factor;
  tx_agc_factor=tx_agc_decay*tx_agc_factor+(1-tx_agc_decay);
  micfft_bin_minpower=micfft_bin_minpower_ref*tx_agc_factor*tx_agc_factor;
  micfft_minpower=micfft_minpower_ref;//*tx_agc_factor*tx_agc_factor;
  valid_bins=tx_ssb_step2(&t1);
  if(tx_setup_mode == TX_SETUP_MUTE)
    {
    if(current_time()-tx_oscilloscope_time >0.2F)
      {
      if(tx_display_mode == TX_DISPLAY_MUTE_F)
        {
        contract_tx_spectrum(&micfft[micfft_px]); 
        show_txfft(tx_oscilloscope_tmp,0,1,micdisplay_fftsize);
        tx_oscilloscope_time=recent_time;
        }
      }
    }  
  mute_pix2=(valid_bins*TX_BAR_SIZE)/
                                (tx_highest_bin-tx_lowest_bin+1);    
  tx_bar(16,TX_BAR_MUTE_LINE,mute_pix1,mute_pix2);
  mute_pix1=mute_pix2;
  settextcolor(7);
  mm=micfft_bufsiz/mic_fftsize;
  n=0;
  for(i=0; i<mm; i++)
    {
    if(cliptimf_mute[i])n++;
    }
  sprintf(s,"%.2f%% ",100.F*(float)n/(float)mm);
  lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_MUTE_LINE,s);
  settextcolor(7);      
// In case AGC has reduced the gain by more than 20 dB, set the
// AGC factor to -20 dB immediately because voice should never
// be as loud. This is to avoid the agc release time constant
// for impulse sounds caused by hitting the microphone etc.       
  t1=BIGFLOAT;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1>old_micagc[i])t1=old_micagc[i];
    }
  t1=log10(t1);  
  sprintf(s,"%.1fdB  ",-20*t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, s);
  micagc_pix2=-t1*TX_BAR_SIZE;
  tx_bar(16,TX_BAR_MICAGC_LINE,micagc_pix1,micagc_pix2);
  micagc_pix1=micagc_pix2;
// ************* Step 4. *************
// Go back to the time domain and store the signal in cliptimf
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
//
// Ideally, the peak amplitude should be 1.0, the audio AGC
// should be active and keep the power level nearly constant.
// For a voice signal the waveform will differ from time to time
// and therefore the peak to average power ratio will vary.
// Compute the peak amplitude for the current transform
// and save it for the next time we arrive here.
// Use the peak amplitude for an AGC in the time domain (complex-valued).
// The Hilbert space AGC is a constant that may vary from one
// FFT block to the next one. It is equivalent with an AM modulator
// with a bandwidth corresponding to a single bin in the FFT so this
// AM modulation will not increase the bandwidth notably, but it
// will bring the RF amplitude near unity always.
//
// Finally, apply an amplitude limiter to the complex time domain signal.
// It will work as an RF limiter on the SSB signal and cause a lot of
// signal outside the passband.
  if(valid_bins!=0)
    {
    fftback(mic_fftsize, mic_fftn, &micfft[micfft_px],
                                   micfft_table, micfft_permute, FALSE);
    lir_sched_yield();
    r2=0;
    for(i=0; i<mic_fftsize; i++)
      {
      t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
         micfft[+2*i+1]*micfft[micfft_px+2*i+1];
      if(t2>r2)r2=t2;
      }
    }          
  else
    {
    r2=0;
    if(tx_setup_mode == TX_SETUP_MUTE)
      {
      if(tx_display_mode == TX_DISPLAY_MUTE_T)
        {
        n=2*mic_fftsize;
        for(i=0; i<n; i++)micfft[micfft_px+i]=0;
        }
      }
    }
  if(tx_setup_mode == TX_SETUP_MUTE)
    {
    if(tx_display_mode == TX_DISPLAY_MUTE_T)
      {
      mm=tx_oscilloscope_tmp_size-1;
      n=(micfft_px-micfft_block+micfft_bufsiz)&micfft_mask;
//tx_ph1=1;
      for(i=0; i<mic_fftsize; i+=2)
        {
        tx_oscilloscope_tmp[tx_oscilloscope_ptr  ]=tx_ph1*
                (tx_oscilloscope_tmp[tx_oscilloscope_ptr  ]+
                                                af_gain*micfft[n+i  ]);
        tx_oscilloscope_tmp[tx_oscilloscope_ptr+1]=tx_ph1*
                (tx_oscilloscope_tmp[tx_oscilloscope_ptr+1]+        
                                                af_gain*micfft[n+i+1]);
        tx_oscilloscope_ptr=(tx_oscilloscope_ptr+2)&mm;
        }
      k=tx_oscilloscope_ptr;
      n=(n+mic_fftsize)&micfft_mask;
      if(tx_fft_phase_flag)
        {
        t1=-af_gain;
        }
      else
        {
        t1=af_gain;
        }
      for(i=0; i<mic_fftsize; i+=2)
        {
        tx_oscilloscope_tmp[k  ]=t1*micfft[n+i  ];
        tx_oscilloscope_tmp[k+1]=t1*micfft[n+i+1];
        k=(k+2)&mm;
        }
      if(current_time()-tx_oscilloscope_time >0.2F)
        {
        k=(tx_oscilloscope_ptr-2*(screen_width+mic_fftsize)+
                                       tx_oscilloscope_tmp_size)&mm;
        for(i=0; i<screen_width; i++)
          {
          lir_setpixel(i,tx_oscilloscope[2*i  ],0);
          lir_setpixel(i,tx_oscilloscope[2*i+1],0);
          
          tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                          tx_oscilloscope_tmp[k  ]+tx_oscilloscope_zero;   
          tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                          tx_oscilloscope_tmp[k+1]+tx_oscilloscope_zero;   
          if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
          if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
          lir_setpixel(i,tx_oscilloscope[2*i  ],14);
          lir_setpixel(i,tx_oscilloscope[2*i+1],12);
          k=(k+2)&mm;
          }
        tx_oscilloscope_time=current_time();
        }
      }
    }
//tx_ssb_buftim('b');
  muted_points=tx_ssb_step4(valid_bins,&pmax,&prat1a, &prat1b);
// The oversampling is mic_fftsize/bandwidth =siz/(siz-(ia1-ib1))
// The number of points in the oversampled signal is mic_fftsize/2
// for a single computation by tx_ssb_step4.
// so the number of un-muted points is mic_fftsize/2-muted_points.
// Mute if we have less than 5 points in the non-oversampled signal.
  if(valid_bins != 0 && mic_fftsize/2-muted_points < 5.F*t1)
    {
    cliptimf_mute[((cliptimf_pa-mic_fftsize+micfft_bufsiz)
                                    &micfft_mask)/mic_fftsize]=TRUE;
    valid_bins=0;
    }
  if(valid_bins==0)
    {
    sprintf(s,"MUTED");
    }
  else
    {
    sprintf(s,"     ");
    }
//tx_ssb_buftim('c');  
  if(tx_setup_mode == TX_SETUP_MUTE ||
     tx_setup_mode == TX_SETUP_MICAGC ||
     tx_setup_mode == TX_SETUP_RF1CLIP)
    {
    if(tx_display_mode == TX_DISPLAY_AGC)
      {
      if(current_time()-tx_oscilloscope_time >0.2F)
        {
        mm=0;
        k=(cliptimf_pa-2*micfft_block-2*screen_width+micfft_bufsiz)&micfft_mask;
        for(i=0; i<screen_width; i++)
          {
          lir_setpixel(i,tx_oscilloscope[2*i  ],0);
          lir_setpixel(i,tx_oscilloscope[2*i+1],0);
          tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                       cliptimf_oscilloscope[k  ]+tx_oscilloscope_zero;   
          tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                       cliptimf_oscilloscope[k+1]+tx_oscilloscope_zero;   
          if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
          if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
          lir_setpixel(i,tx_oscilloscope[2*i  ],10);
          lir_setpixel(i,tx_oscilloscope[2*i+1],12);
          k=(k+2)&micfft_mask;
          }
        tx_oscilloscope_time=current_time();
        }
      } 
    if(tx_display_mode == TX_DISPLAY_CLIP)
      {
      if(current_time()-tx_oscilloscope_time >0.2F)
        {
        k=(cliptimf_pa-2*micfft_block-2*screen_width+micfft_bufsiz)&micfft_mask;
        for(i=0; i<screen_width; i++)
          {
          lir_setpixel(i,tx_oscilloscope[2*i  ],0);
          lir_setpixel(i,tx_oscilloscope[2*i+1],0);
          tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                                            cliptimf[k  ]+tx_oscilloscope_zero;   
          tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                                            cliptimf[k+1]+tx_oscilloscope_zero;   
          if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
          if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
          if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
          lir_setpixel(i,tx_oscilloscope[2*i  ],15);
          lir_setpixel(i,tx_oscilloscope[2*i+1],12);
          k=(k+2)&micfft_mask;
          }
        tx_oscilloscope_time=current_time();
        }
      }
    }
  old_prat1a[old_display_ptr]=prat1a;
  old_prat1b[old_display_ptr]=prat1b;
  old_pmax1[old_display_ptr]=pmax;
  prat1a=0;
  prat1b=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    prat1a+=old_prat1a[i];
    prat1b+=old_prat1b[i];
    if(pmax<old_pmax1[i])pmax=old_pmax1[i];
    }
  prat1a/=DISPLAY_HOLD_SIZE;  
  prat1b/=DISPLAY_HOLD_SIZE;  
  t1=1;
  if(pmax/pow(10.0,0.1*txssb.rf1_gain) < t1)
                                      t1=pmax/pow(10.0,0.1*txssb.rf1_gain);
  prat1a=10*log10(prat1a);  
  if(prat1b>0.000000001 && pmax > prat1b)
    {
    prat1b=10*log10(prat1b);  
    }
  else
    {
    prat1b=99;
    }
  if(sw_cliptimf)
    {  
    sprintf(s,"%.1fdB   ",prat1b);
    sprintf(&s[15],"%.1fdB   ",prat1a);
    }
  else
    {  
    sprintf(s,"------   ");
    sprintf(&s[15],"------   ");
    }
  if(sw_cliptimf && tx_display_mode == TX_DISPLAY_AGC)
    {
    settextcolor(15);
    }
  lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, &s[15]);
  if(sw_cliptimf && tx_display_mode == TX_DISPLAY_CLIP)
    {
    settextcolor(15);
    }
  else 
    {
    settextcolor(7);
    }
  lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
  settextcolor(7);
  old_rf1clip[old_display_ptr]=pmax;
  if(pmax > 1000)pmax=1000;  
  if(pmax > 1)
    {  
    rf1clip_pix2=log10(pmax)*TX_BAR_SIZE/3;
    t2=1;
    }
  else
    {
    rf1clip_pix2=0;
    t2=pmax;
    }
  t1=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1<old_rf1clip[i])t1=old_rf1clip[i];
    }
  if(t1 > 1)
    {
    t1=10*log10(t1);  
    }
  else
    {
    t1=0;
    }  
  sprintf(s,"%.1fdB  ",t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
  if(t2 > 0.000000001)
    {
    sprintf(s,"%.1fdB  ",10*log10(t2));
    lir_text(38+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, s);
    }
  else
    {
    lir_text(38+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, "       ");
    }
  tx_bar(16,TX_BAR_RF1CLIP_LINE,rf1clip_pix1,rf1clip_pix2);
  rf1clip_pix1=rf1clip_pix2;
// ************* Step 5. *************
// At this point we have applied an RF clipper by limiting power on a
// complex-valued time domain function. As a consequence we have 
// produced intermodulation products outside the desired passband.
// Go to the frequency domain and remove undesired frequencies.
  tx_ssb_step5();
// ************* Step 6. *************
// Go back to the time domain and store the signal in alctimf.
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
// Compute power and store the peak power with
// an exponential decay corresponding to a time constant
// of 50 milliseconds.
// We expand the fft size from mic_fftsize to tx_fftsiz2 because
// the fractional resampling that will follow this step needs
// the signal to be oversampled by a factor of four to avoid
// attenuation at high frequencies. The polynomial fitting
// works as a low pass filter and we do not want any attenuation
// at the high side of our passband.
  if(lir_tx_output_samples() < snd[TXDA].tot_frames-2*snd[TXDA].block_frames)
    {    
    while(clipfft_px != clipfft_pa)
      {
      pmax=tx_ssb_step6(&prat);
      lir_sched_yield();
      if(tx_setup_mode == TX_SETUP_ALC)
        {
        if(tx_display_mode == TX_DISPLAY_ALCTIMF_RAW)
          {
          k=(alctimf_pa-alc_fftsize+alc_bufsiz)&alc_mask;
          for(i=0; i<alc_fftsize; i++)
            {
            alctimf_raw[k+i]=alctimf[k+i];
            }
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            k=(alctimf_pa-2*alc_fftsize-2*screen_width+alc_bufsiz)&alc_mask;;
            for(i=0; i<screen_width; i++)
              {
              lir_setpixel(i,tx_oscilloscope[2*i  ],0);
              lir_setpixel(i,tx_oscilloscope[2*i+1],0);
              tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                                      alctimf_raw[k  ]+tx_oscilloscope_zero;   
              tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                                      alctimf_raw[k+1]+tx_oscilloscope_zero;   
              if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
              if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
              if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
              if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
              lir_setpixel(i,tx_oscilloscope[2*i  ],15);
              lir_setpixel(i,tx_oscilloscope[2*i+1],12);
              k=(k+2)&alc_mask;
              }
            tx_oscilloscope_time=current_time();
            }
          }
        }
      prat/=alc_sizhalf;
      old_prat2[old_display_ptr]=prat;
      old_pmax2[old_display_ptr]=pmax;
      prat=0;
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        prat+=old_prat2[i];
        }
      prat/=DISPLAY_HOLD_SIZE;  
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        if(pmax<old_pmax2[i])pmax=old_pmax2[i];
        }
      sprintf(s,"%.1fdB  ",pmax);
      lir_text(38+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
      if(prat>0.000000001 && pmax > prat)
        {
        prat=10*log10(pmax/prat);  
        sprintf(s,"%.1fdB  ",prat);
        if(tx_display_mode == TX_DISPLAY_ALCTIMF_RAW)
          {
          settextcolor(15);
          }
        lir_text(48+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
        if(tx_display_mode == TX_DISPLAY_ALCTIMF_RAW)
          {
          settextcolor(7);
          }
        }
      else
        {
        lir_text(48+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, "      ");
        } 
      if(pmax > 1)
        {  
        pmax=2*log10(pmax);
        if(pmax>1)pmax=1;
        alc_pix2=pmax*TX_BAR_SIZE;
        }
      else
        {
        alc_pix2=0;
        }
      tx_bar(16,TX_BAR_ALC_LINE,alc_pix1,alc_pix2);
      alc_pix1=alc_pix2;
      }
// ************* Step 7. *************
// Use the slowly decaying bi-directional peak power as an ALC
// but make sure we allow at least two data blocks un-processed
// to allow the backwards fall-off to become re-calculated.
// Using the slowly varying function for an ALC (= AM modulation)
// will not increase the bandwidth by more than the bandwidth
// of the modulation signal (50 ms or 20 Hz)
    while( ((alctimf_pa-alctimf_pb+alc_bufsiz)&alc_mask) >= 3*alc_fftsize)
      {
      pmax=tx_ssb_step7(&prat);
      if(tx_setup_mode == TX_SETUP_ALC)
        {
        if(tx_display_mode == TX_DISPLAY_ALCTIMF)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            k=(alctimf_pb-2*alc_fftsize-2*screen_width+alc_bufsiz)&alc_mask;;
            for(i=0; i<screen_width; i++)
              {
              lir_setpixel(i,tx_oscilloscope[2*i  ],0);
              lir_setpixel(i,tx_oscilloscope[2*i+1],0);
              tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                                      alctimf[k  ]+tx_oscilloscope_zero;   
              tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                                      alctimf[k+1]+tx_oscilloscope_zero;   
              if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
              if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
              if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
              if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
              lir_setpixel(i,tx_oscilloscope[2*i  ],14);
              lir_setpixel(i,tx_oscilloscope[2*i+1],11);
              k=(k+2)&alc_mask;
              }
            tx_oscilloscope_time=current_time();
            }
          }
        if(tx_display_mode == TX_DISPLAY_ALC_FACTOR)
          {
          k=(alctimf_pa-alc_fftsize+alc_bufsiz)&alc_mask;
          for(i=0; i<alc_fftsize; i++)
            {
            alctimf_raw[k+i]=alctimf[k+i];
            }
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            k=(alctimf_pb-2*alc_fftsize-2*screen_width+alc_bufsiz)&alc_mask;;
            for(i=0; i<screen_width; i++)
              {
              lir_setpixel(i,tx_oscilloscope[2*i  ],0);
              lir_setpixel(i,tx_oscilloscope[2*i+1],0);
              if(fabs(alctimf_raw[k]) > 0)
                {
                tx_oscilloscope[2*i  ]=-4.F*tx_oscilloscope_ypixels*txtrace_gain*
                           alctimf[k  ]/alctimf_raw[k]+tx_oscilloscope_max;   

                }
              else
                {  
                tx_oscilloscope[2*i  ]=-4.F*tx_oscilloscope_ypixels*txtrace_gain+
                                                tx_oscilloscope_max;
                }
              if(fabs(alctimf_raw[k+1]) > 0)
                {
                tx_oscilloscope[2*i+1]=-4.F*tx_oscilloscope_ypixels*txtrace_gain*
                             alctimf[k+1]/alctimf_raw[k+1]+tx_oscilloscope_max;   
                }
              else
                {  
                tx_oscilloscope[2*i+1]=-4.F*tx_oscilloscope_ypixels*txtrace_gain+
                                                tx_oscilloscope_max;
                }
              if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
              if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
              lir_setpixel(i,tx_oscilloscope[2*i  ],10);
              lir_setpixel(i,tx_oscilloscope[2*i+1],10);
              k=(k+2)&alc_mask;
              }
            tx_oscilloscope_time=current_time();
            }
          }
        if(tx_display_mode == TX_DISPLAY_ALC_SPECTRUM)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            k=tx_analyze_sizhalf;
            m=tx_analyze_sizhalf-1;
            n=(alctimf_pb-2*tx_analyze_size-2*alc_fftsize
                                               +alc_bufsiz)&alc_mask;;
            mm=(n+tx_analyze_size)&alc_mask;
            for(i=0; i<tx_analyze_sizhalf; i++)
              {
              alctimf_raw[2*i  ]=alctimf[n  ]*tx_analyze_win[i]; 
              alctimf_raw[2*i+1]=alctimf[n+1]*tx_analyze_win[i]; 
              alctimf_raw[2*k  ]=alctimf[mm  ]*tx_analyze_win[m];
              alctimf_raw[2*k+1]=alctimf[mm+1]*tx_analyze_win[m];
              m--;
              k++;
              n=(n+2)&alc_mask;
              mm=(mm+2)&alc_mask;
              }
            lir_sched_yield();  
            fftforward(tx_analyze_size, tx_analyze_fftn, alctimf_raw, 
                             tx_analyze_table, tx_analyze_permute, FALSE);
            display_color[0]=13;
            show_txfft(alctimf_raw,0,0,tx_analyze_size);
            tx_oscilloscope_time=recent_time;
            }
          }
        }
      prat/=alc_sizhalf;
      old_prat3[old_display_ptr]=prat;
      old_pmax3[old_display_ptr]=pmax;
      prat=0;
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        prat+=old_prat3[i];
        }
      prat/=DISPLAY_HOLD_SIZE;  
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        if(pmax<old_pmax3[i])pmax=old_pmax3[i];
        }
      if(prat>0.000000001 && pmax > prat)
        {
        prat=10*log10(pmax/prat);  
        sprintf(s,"%.1fdB  ",prat);
        if(tx_setup_mode == TX_SETUP_ALC &&
           tx_display_mode == TX_DISPLAY_ALC_SPECTRUM)
          {
          settextcolor(15);
          }
        lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_ALC_LINE, s);
        if(tx_setup_mode == TX_SETUP_ALC &&
          tx_display_mode == TX_DISPLAY_ALC_SPECTRUM)
          {
          settextcolor(7);
          }
        }
      else
        {
        lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_ALC_LINE, "       ");
        } 
      if(pmax > 0.000000001)
        {
        sprintf(s,"%.1fdB  ",10*log10(pmax));
        lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_ALC_LINE, s);
        }
      else
        {
        lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_ALC_LINE, "       ");
        }
        

      }
// ************* Step 8. *************
// In case we have enough signal in the buffer, start the output.
    tx_ssb_step8();
    if(tx_setup_mode == TX_SETUP_ALC)
      {
      if(tx_display_mode == TX_DISPLAY_TXOUT_SPECTRUM)
        {
        if(current_time()-tx_oscilloscope_time >0.2F)
          {
          k=tx_analyze_sizhalf;
          m=tx_analyze_sizhalf-1;
          n=(txout_pa-2*tx_analyze_size+txout_bufsiz)&txout_mask;;
          mm=(n+tx_analyze_size)&txout_mask;
          for(i=0; i<tx_analyze_sizhalf; i++)
            {
            alctimf_raw[2*i  ]=txout[n  ]*tx_analyze_win[i]; 
            alctimf_raw[2*i+1]=txout[n+1]*tx_analyze_win[i]; 
            alctimf_raw[2*k  ]=txout[mm  ]*tx_analyze_win[m];
            alctimf_raw[2*k+1]=txout[mm+1]*tx_analyze_win[m];
            m--;
            k++;
            n=(n+2)&txout_mask;
            mm=(mm+2)&txout_mask;
            }
          lir_sched_yield();  
          fftforward(tx_analyze_size, tx_analyze_fftn, alctimf_raw, 
                             tx_analyze_table, tx_analyze_permute, FALSE);
          display_color[0]=15;
          show_txfft(alctimf_raw,0,0,tx_analyze_size);
          tx_oscilloscope_time=recent_time;
          }
        }
      if(tx_display_mode == TX_DISPLAY_TXOUT_TIMF)
        {
        if(current_time()-tx_oscilloscope_time >0.2F)
          {
          n=(txout_pa-2*screen_width+txout_bufsiz)&txout_mask;;
          for(i=0; i<screen_width; i++)
            {
            lir_setpixel(i,tx_oscilloscope[2*i  ],0);
            lir_setpixel(i,tx_oscilloscope[2*i+1],0);
          
            tx_oscilloscope[2*i  ]=tx_oscilloscope_ypixels*txtrace_gain*
                                            txout[n  ]+tx_oscilloscope_zero;   
            tx_oscilloscope[2*i+1]=tx_oscilloscope_ypixels*txtrace_gain*
                                            txout[n+1]+tx_oscilloscope_zero;   
            if(tx_oscilloscope[2*i ] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_min;
            if(tx_oscilloscope[2*i+1] < tx_oscilloscope_min)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_min;
            if(tx_oscilloscope[2*i  ] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i  ]=tx_oscilloscope_max;
            if(tx_oscilloscope[2*i+1] > tx_oscilloscope_max)
                                tx_oscilloscope[2*i+1]=tx_oscilloscope_max;
            lir_setpixel(i,tx_oscilloscope[2*i  ],12);
            lir_setpixel(i,tx_oscilloscope[2*i+1],10);
            n=(n+2)&txout_mask;
            }
          tx_oscilloscope_time=recent_time;
          }
        }
      }
    }
  test_keyboard();
  if(lir_inkey != 0)
    {
    process_current_lir_inkey();
    if(lir_inkey=='X')goto end_tx_setup;
    if(lir_inkey==ARROW_UP_KEY)
      {
      txtrace_gain*=1.5;
      goto continue_setup;
      }
    if(lir_inkey==ARROW_DOWN_KEY)
      {
      txtrace_gain/=1.35;
      goto continue_setup;
      }
    linrad_thread_stop_and_join(THREAD_TX_INPUT);
    close_tx_sndout();
    close_tx_sndin();
    if(txmem_handle != NULL)free(txmem_handle);
    txmem_handle=NULL;
    free(txtrace); 
    lir_await_event(EVENT_TX_INPUT);
    lir_close_event(EVENT_TX_INPUT);
    if(lir_inkey=='S')
      {
      clear_screen();
      save_tx_parms(TRUE);
      }
    if(lir_inkey=='R')
      {
      clear_screen();
      read_txpar_file();
      }
    if(lir_inkey == F3_KEY)
      {
      tx_onoff_flag++;
      tx_onoff_flag&=1;
      }  
    switch (tx_setup_mode)
      {
      case TX_SETUP_AD:
      switch (lir_inkey)
        {
        case 'L':
        lir_text(0,TXPAR_INPUT_LINE,"Low:");
        txssb.minfreq=lir_get_integer(5,TXPAR_INPUT_LINE,4,1,SSB_MINFQ_HIGH);
        break;


        case 'H':
        lir_text(0,TXPAR_INPUT_LINE,"High:");
        txssb.maxfreq=lir_get_integer(6,TXPAR_INPUT_LINE,5,
                                            SSB_MAXFQ_LOW,ui.tx_ad_speed/2);
        break;

        case 'F':
        lir_text(0,TXPAR_INPUT_LINE,"Slope:");
        txssb.slope=lir_get_integer(7,TXPAR_INPUT_LINE,4,SSB_MINSLOPE,
                                                               SSB_MAXSLOPE);
        break;

        case 'B':
        lir_text(0,TXPAR_INPUT_LINE,"Bass:");
        txssb.bass=lir_get_integer(6,
                                 TXPAR_INPUT_LINE,4,SSB_MINBASS,SSB_MAXBASS);
        break;

        case 'T':
        lir_text(0,TXPAR_INPUT_LINE,"Treble:");
        txssb.treble=lir_get_integer(8,
                             TXPAR_INPUT_LINE,4,SSB_MINTREBLE,SSB_MAXTREBLE);
        break;

        default:
        if(lir_inkey >= '1' && lir_inkey <= '5')
          {
          tx_display_mode=lir_inkey;
          }
        break;
        }
      if(tx_display_mode > '5')tx_display_mode='5';
      break;  

      case TX_SETUP_MUTE:
      switch(lir_inkey)
        {
        case 'F':
        lir_text(0,TXPAR_INPUT_LINE,"Freq domain:");
        txssb.mic_f_threshold=lir_get_integer(13,
                                          TXPAR_INPUT_LINE,3,0,SSB_MAX_MICF);
        break;

        case 'T':
        lir_text(0,TXPAR_INPUT_LINE,"Time domain:");
        txssb.mic_t_threshold=lir_get_integer(13,
                                          TXPAR_INPUT_LINE,3,0,SSB_MAX_MICT);
        break;

        default:
        if(lir_inkey >= '1' && lir_inkey <= '6')
          {
          tx_display_mode=lir_inkey;
          }
        break;
        }        
      if(tx_display_mode > '6')tx_display_mode='6';
      break;

      case TX_SETUP_MICAGC:
      if(lir_inkey=='V')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Volume:");
        txssb.mic_gain=lir_get_integer(8,
                                      TXPAR_INPUT_LINE,3,0,SSB_MAX_MICGAIN);
        }
      if(lir_inkey=='T')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Time constant:");
        t1=lir_get_float(15,TXPAR_INPUT_LINE,6,0,SSB_MAX_MICAGC_TIME);
        txssb.mic_agc_time=100*t1;  
        }
      if(lir_inkey >= '5' && lir_inkey <= '6')
        {
        tx_display_mode=lir_inkey;
        }
      if(tx_display_mode < '5')tx_display_mode='5';
      if(tx_display_mode > '6')tx_display_mode='6';
      break;

      case TX_SETUP_RF1CLIP:
      if(lir_inkey=='B')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Clipper gain:");
        txssb.rf1_gain=lir_get_integer(15,
                       TXPAR_INPUT_LINE,3,SSB_MIN_RF1_GAIN,SSB_MAX_RF1_GAIN);
        }
      if(lir_inkey >= '1' && lir_inkey <= '6')
        {
        tx_display_mode=lir_inkey;
        }
      if(tx_display_mode > '6')tx_display_mode='6';
      break;

      case TX_SETUP_ALC:
      if(lir_inkey=='T')
        {
        lir_text(0,TXPAR_INPUT_LINE,"ALC time constant:");
        t1=lir_get_float(18,TXPAR_INPUT_LINE,6,0,SSB_MAX_ALC_TIME);
        txssb.alc_time=10*t1;
        }
      if(lir_inkey=='U')
        {
        lir_text(0,TXPAR_INPUT_LINE,"FFT time span:");
        txssb.micblock_time=lir_get_integer(14,TXPAR_INPUT_LINE,6,
                              SSB_MIN_MICBLOCK_TIME, SSB_MAX_MICBLOCK_TIME);
        }
      if(lir_inkey=='D')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Output delay margin:");
        txssb.delay_margin=lir_get_integer(20,TXPAR_INPUT_LINE,6,
                               SSB_MIN_DELAY_MARGIN,SSB_MAX_DELAY_MARGIN);
        }
      if(lir_inkey=='E')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Frequency shift:");
        txssb.frequency_shift=lir_get_integer(16,TXPAR_INPUT_LINE,6,
                               -ui.tx_da_speed/2,ui.tx_da_speed/2);
        }
      if(lir_inkey >= '1' && lir_inkey <= '7')
        {
        tx_display_mode=lir_inkey;
        }
      if(tx_display_mode > '7')tx_display_mode='7';
      break;
      }
// ABCDEFgHijkLMnoPQRSTUVXyz      
    if(lir_inkey=='A')tx_setup_mode=TX_SETUP_MICAGC;
    if(lir_inkey=='P')tx_setup_mode=TX_SETUP_ALC;
    if(lir_inkey=='M')tx_setup_mode=TX_SETUP_AD;
    if(lir_inkey=='Q')tx_setup_mode=TX_SETUP_MUTE;
    if(lir_inkey=='C')tx_setup_mode=TX_SETUP_RF1CLIP;
    if(lir_inkey=='+')
      {
      tg.spproc_no++;
      if(tg.spproc_no > MAX_SSBPROC_FILES)tg.spproc_no=MAX_SSBPROC_FILES;
      }
    if(lir_inkey=='-')
      {
      tg.spproc_no--;
      if(tg.spproc_no < 0)tg.spproc_no=0;
      }
    goto restart;
    }
continue_setup:;  
  }
end_tx_setup:;
linrad_thread_stop_and_join(THREAD_TX_INPUT);
free(txtrace);
close_tx_sndout();
close_tx_sndin();
lir_close_event(EVENT_TX_INPUT);
if(txmem_handle != NULL)free(txmem_handle);
txmem_handle=NULL;
tg.spproc_no=default_spproc_no;
tx_onoff_flag=0;
}

