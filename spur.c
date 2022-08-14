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
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "thrdef.h"
int update_spur_pol(void);


void eliminate_spurs(void)
{
int ni, nx, iter;
int ind, knd, diffind;
int i, j, k;
float *z;
int *uind;
float *zsig;
float *spt;
float t1, t2, t3, t4, r1, r2, r3, r4;
float ampl, phase, phase_slope, phase_curv;
float freq, rot;
short int *sptmmx, *zmmx;
// Give the pointers a value. 
// Otherwise the compiler will complain.
spt=spur_table;
sptmmx=spur_table_mmx;
nx=(ffts_na+1)&fftxn_mask;
for(spurno=0; spurno<no_of_spurs; spurno++)
  {
  if( (spurno & 11111)==0)lir_sched_yield();
  spurp0=spurno*spur_block;
  i=3*spurno;
  sp_c1=spur_pol[i  ]; 
  sp_c2=spur_pol[i+1];
  sp_c3=spur_pol[i+2];
  uind=&spur_ind[spurno*max_fftxn];
  zsig=&spur_signal[twice_rxchan*max_fftxn*spurno];
  if(swmmx_fft2)
    {
    sptmmx=&spur_table_mmx[spurp0+ffts_na*SPUR_WIDTH*twice_rxchan];
    }
  else
    {
    spt=&spur_table[spurp0+ffts_na*SPUR_WIDTH*twice_rxchan];
    }
  if(spur_flag[spurno] == 1)
    {
    j=(int)(spur_freq[spurno])+2-spur_location[spurno]-SPUR_SIZE/2; 
    if(j < 0 || j > 1)
      {
      if(j < -1 )j=-1;
      if(j > 2)j=2;
      shift_spur_table(j);
      }
    }
  if(spur_flag[spurno] != 0)
    {
    if(sw_onechan)
      {
      if(swmmx_fft2)
        {
        zmmx=&fft2_short_int[2*(ffts_na*fftx_size+spur_location[spurno])];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sptmmx[2*i  ]=zmmx[2*i  ];
          sptmmx[2*i+1]=zmmx[2*i+1];
          }
        }
      else
        {
        z=&fftx[2*(ffts_na*fftx_size+spur_location[spurno])];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          spt[2*i  ]=z[2*i  ];
          spt[2*i+1]=z[2*i+1];
          }
        }
      }
    else
      {
      if(swmmx_fft2)
        {
        zmmx=&fft2_short_int[4*(ffts_na*fftx_size+spur_location[spurno])];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sptmmx[4*i  ]=zmmx[4*i  ];
          sptmmx[4*i+1]=zmmx[4*i+1];
          sptmmx[4*i+2]=zmmx[4*i+2];
          sptmmx[4*i+3]=zmmx[4*i+3];
          }
        }
      else
        {
        z=&fftx[4*(ffts_na*fftx_size+spur_location[spurno])];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          spt[4*i  ]=z[4*i  ];
          spt[4*i+1]=z[4*i+1];
          spt[4*i+2]=z[4*i+2];
          spt[4*i+3]=z[4*i+3];
          }
        }
      }
    k=spur_flag[spurno];
    if(k > 5)k*=10;
    k*=2;
    spur_flag[spurno]++;
    if(spur_flag[spurno]>1000000)
      {
      spur_flag[spurno]-=2*3*5*7*spur_speknum;
      } 
    if(k%spur_speknum ==0)
      {
// If we just lost locking or      
// if we have been unlocked for a while, try to lock again.
      i=spur_relock();
      if(genparm[AFC_ENABLE] == 2 && i == FALSE)
        {
        no_of_spurs--;
        if(no_of_spurs==0)return;
        if(spurno != no_of_spurs)
          {
          remove_spur(spurno);
          }
        spurno--;
        }
      }
    goto lock_fail;
    }
// Use old PLL data to get the expected phase slope for the new transform
// Calculate spur_signal and store at zsig.
  phase_slope=spur_d1pha[spurno]+spur_d2pha[spurno];
  rot=-0.5*phase_slope/PI_L;
  i=spur_freq[spurno]*spur_freq_factor-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  spur_freq[spurno]=freq;
gtpos1:;    
  j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
  if(j < 0 || j > 1)
    {
    if(j < -1 || j > 2)
      {
      spur_flag[spurno]=1;
      goto lock_fail;
      }
    shift_spur_table(j);
    goto gtpos1;
    }
  j=1-j;
  ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
  if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
  ind=ind*SPUR_SIZE+j;
  uind[ffts_na]=ind;  
  if(sw_onechan)
    {
    r1=0;
    r2=0;
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[2*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        sptmmx[2*i  ]=zmmx[2*i  ];
        r1+=zmmx[2*i  ]*spur_spectra[ind+i];
        sptmmx[2*i+1]=zmmx[2*i+1];
        r2+=zmmx[2*i+1]*spur_spectra[ind+i];
        }
      }
    else
      {
      z=&fftx[2*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spt[2*i  ]=z[2*i  ];
        r1+=z[2*i  ]*spur_spectra[ind+i];
        spt[2*i+1]=z[2*i+1];
        r2+=z[2*i+1]*spur_spectra[ind+i];
        }
      }
    }
  else
    {
    t1=0;
    t2=0;
    t3=0;
    t4=0;
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[4*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        sptmmx[4*i  ]=zmmx[4*i  ];
        t1+=zmmx[4*i  ]*spur_spectra[ind+i];
        sptmmx[4*i+1]=zmmx[4*i+1];
        t2+=zmmx[4*i+1]*spur_spectra[ind+i];
        sptmmx[4*i+2]=zmmx[4*i+2];
        t3+=zmmx[4*i+2]*spur_spectra[ind+i];
        sptmmx[4*i+3]=zmmx[4*i+3];
        t4+=zmmx[4*i+3]*spur_spectra[ind+i];
        }
      }
    else
      {
      z=&fftx[4*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spt[4*i  ]=z[4*i  ];
        t1+=z[4*i  ]*spur_spectra[ind+i];
        spt[4*i+1]=z[4*i+1];
        t2+=z[4*i+1]*spur_spectra[ind+i];
        spt[4*i+2]=z[4*i+2];
        t3+=z[4*i+2]*spur_spectra[ind+i];
        spt[4*i+3]=z[4*i+3];
        t4+=z[4*i+3]*spur_spectra[ind+i];
        }
      }
    r1=sp_c1*t1+sp_c2*t3+sp_c3*t4;
    r2=sp_c1*t2+sp_c2*t4-sp_c3*t3;
    r3=sp_c1*t3-sp_c2*t1+sp_c3*t2;
    r4=sp_c1*t4-sp_c2*t2-sp_c3*t1;
    if((j^(spur_location[spurno]&1))==1)
      {
      r3=-r3;
      r4=-r4;
      }
    zsig[4*ffts_na+2]=r3;
    zsig[4*ffts_na+3]=r4;
    }
  if((j^(spur_location[spurno]&1))==1)
    {
    r1=-r1;
    r2=-r2;
    }
  zsig[twice_rxchan*ffts_na  ]=r1;
  zsig[twice_rxchan*ffts_na+1]=r2;
  iter=0;
  freq=spur_freq[spurno];
refine:;
  iter++;  
  refine_pll_parameters();
// **************************************************************
// Now, that the LO is changed, check if we have to recalculate
// spur_signal. In the spur_signal evaluation we need to know
// the LO frequency (phase_slope) in order to weigh the
// individual spectral lines properly.
  phase_slope=spur_d1pha[spurno];
  phase_curv=spur_d2pha[spurno];
  phase_slope+=phase_curv;
  nx=(ffts_na-spur_speknum+fftxn_mask)&fftxn_mask;
  diffind=0;
  ni=ffts_na;
  while(ni != nx)
    {
    rot=-0.5*phase_slope/PI_L;
    i=freq*spur_freq_factor-rot+0.5;
    rot+=i;
    freq=rot/spur_freq_factor;
    j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
    if(j < 0 || j > 1)goto iterx;
    j=1-j;
    ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
    if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
    ind=ind*SPUR_SIZE+j;
    k=(uind[ni]-ind+NO_OF_SPUR_SPECTRA*SPUR_SIZE)&
                                         (NO_OF_SPUR_SPECTRA*SPUR_SIZE-1);  
    if(k > NO_OF_SPUR_SPECTRA*SPUR_SIZE/2)k=NO_OF_SPUR_SPECTRA*SPUR_SIZE-k;
    if(k > diffind)diffind=k;
    uind[ni]=ind;
    if(k != 0)
      {
      knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
      if(sw_onechan)
        {
        r1=0;
        r2=0;
        if(swmmx_fft2)
          {
          for(i=0; i<SPUR_WIDTH; i++)
            {
            r1+=spur_table_mmx[2*i+knd  ]*spur_spectra[ind+i];
            r2+=spur_table_mmx[2*i+knd+1]*spur_spectra[ind+i];
            }
          }  
        else
          {
          for(i=0; i<SPUR_WIDTH; i++)
            {
            r1+=spur_table[2*i+knd  ]*spur_spectra[ind+i];
            r2+=spur_table[2*i+knd+1]*spur_spectra[ind+i];
            }
          }  
        }
      else  
        {
        t1=0;
        t2=0;
        t3=0;
        t4=0;
        if(swmmx_fft2)
          {
          for(i=0; i<SPUR_WIDTH; i++)
            {
            t1+=spur_table_mmx[4*i+knd  ]*spur_spectra[ind+i];
            t2+=spur_table_mmx[4*i+knd+1]*spur_spectra[ind+i];
            t3+=spur_table_mmx[4*i+knd+2]*spur_spectra[ind+i];
            t4+=spur_table_mmx[4*i+knd+3]*spur_spectra[ind+i];
            }
          }  
        else
          {
          for(i=0; i<SPUR_WIDTH; i++)
            {
            t1+=spur_table[4*i+knd  ]*spur_spectra[ind+i];
            t2+=spur_table[4*i+knd+1]*spur_spectra[ind+i];
            t3+=spur_table[4*i+knd+2]*spur_spectra[ind+i];
            t4+=spur_table[4*i+knd+3]*spur_spectra[ind+i];
            }
          }  
        r1=sp_c1*t1+sp_c2*t3+sp_c3*t4;
        r2=sp_c1*t2+sp_c2*t4-sp_c3*t3;
        r3=sp_c1*t3-sp_c2*t1+sp_c3*t2;
        r4=sp_c1*t4-sp_c2*t2-sp_c3*t1;
        if((j^(spur_location[spurno]&1))==1)
          {
          r3=-r3;
          r4=-r4;
          }
        zsig[4*ni+2]=r3;
        zsig[4*ni+3]=r4;
        }
      if((j^(spur_location[spurno]&1))==1)
        {
        r1=-r1;
        r2=-r2;
        }
      zsig[twice_rxchan*ni  ]=r1;
      zsig[twice_rxchan*ni+1]=r2;
      }
    phase_slope-=phase_curv;
    ni=(ni+fftxn_mask)&fftxn_mask;
    }
  if(diffind > 2.5*SPUR_SIZE && iter < 5)goto refine;
iterx:;
  if(diffind!=0)refine_pll_parameters();
  if(fabs(spur_ampl[spurno])< spur_minston*spur_noise[spurno])
    {
    spur_flag[spurno]=1;
    goto lock_fail;
    }
// If we have reached this far, the spur phase is well established.
// If we have two rx channels, update the polarization.
// The polarization is assumed to vary slowly with time so there
// is no reason to remake spur_signal
  if(ui.rx_rf_channels == 2)
    {
    if(update_spur_pol() != 0) refine_pll_parameters();
    }
// Subtract the spur from the new transform. 
  phase=spur_d0pha[spurno];
  phase_slope=spur_d1pha[spurno];
  phase_curv=spur_d2pha[spurno];
  ampl=spur_ampl[spurno];
  phase_slope+=phase_curv;
  phase+=phase_slope;
  spur_d0pha[spurno]=phase;
  spur_d1pha[spurno]=phase_slope;
  if(spur_d0pha[spurno] > PI_L)spur_d0pha[spurno]-=2*PI_L;
  if(spur_d0pha[spurno] <-PI_L)spur_d0pha[spurno]+=2*PI_L;
  if(spur_d1pha[spurno] > PI_L)spur_d1pha[spurno]-=2*PI_L;
  if(spur_d1pha[spurno] <-PI_L)spur_d1pha[spurno]+=2*PI_L;
  if(spur_d2pha[spurno] > PI_L)spur_d2pha[spurno]-=2*PI_L;
  if(spur_d2pha[spurno] <-PI_L)spur_d2pha[spurno]+=2*PI_L;
  spur_avgd2[spurno]=spur_weiold*spur_avgd2[spurno]+spur_weinew*phase_curv;
  rot=-0.5*phase_slope/PI_L;
  i=spur_freq[spurno]*spur_freq_factor-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  spur_freq[spurno]=freq;
gtpos3:;
  j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
  if(j < 0 || j > 1)
    {
    if(j < -1 || j > 2)
      {
      spur_flag[spurno]=1;
      goto lock_fail;
      }
    shift_spur_table(j);
    goto gtpos3;
    }
  j=1-j;
  ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
  if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
  ind=ind*SPUR_SIZE+j;
  if(sw_onechan)
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-cos(phase)*ampl;
      t2=-sin(phase)*ampl;
      }
    else
      {  
      t1=cos(phase)*ampl;
      t2=sin(phase)*ampl;
      }
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[2*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        zmmx[2*i  ]-=spur_spectra[ind+i]*t1;
        zmmx[2*i+1]-=spur_spectra[ind+i]*t2;
        }
      }
    else
      {
      z=&fftx[2*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[2*i  ]-=spur_spectra[ind+i]*t1;
        z[2*i+1]-=spur_spectra[ind+i]*t2;
        }
      }
    }
  else  
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-sp_c1*cos(phase)*ampl;
      t2=-sp_c1*sin(phase)*ampl;
      t3=-(sp_c2*cos(phase)-sp_c3*sin(phase))*ampl;
      t4=-(sp_c2*sin(phase)+sp_c3*cos(phase))*ampl;
      }
    else
      {  
      t1=sp_c1*cos(phase)*ampl;
      t2=sp_c1*sin(phase)*ampl;
      t3=(sp_c2*cos(phase)-sp_c3*sin(phase))*ampl;
      t4=(sp_c2*sin(phase)+sp_c3*cos(phase))*ampl;
      }
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[4*(ffts_na*fft2_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        zmmx[4*i  ]-=spur_spectra[ind+i]*t1;
        zmmx[4*i+1]-=spur_spectra[ind+i]*t2;
        zmmx[4*i+2]-=spur_spectra[ind+i]*t3;
        zmmx[4*i+3]-=spur_spectra[ind+i]*t4;
        }
      }
    else
      {
      z=&fftx[4*(ffts_na*fftx_size+spur_location[spurno])];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[4*i  ]-=spur_spectra[ind+i]*t1;
        z[4*i+1]-=spur_spectra[ind+i]*t2;
        z[4*i+2]-=spur_spectra[ind+i]*t3;
        z[4*i+3]-=spur_spectra[ind+i]*t4;
        }
      }
    }
  lock_fail:;
  }
}

int update_spur_pol(void)
{
int i,ni;
float *z;
float c1,c2,c3,sina;
float a2,b2,re,im;
float t1,t2,t3,t4,r1,r2,noi2,a2s,b2s;
// The data in spur signal is combined by use of the current
// polarization coefficient for the first channel to contain
// the spur and for the second channel to contain noise only.
// Get the correlation between the first and the second channel.
// If it is nonzero, there is some contribution from the
// spur in the second channel.
// Use the correlation to improve the polarization coefficients.
// See fft3_mix2() in mix2.c for explanation.
z=&spur_signal[4*max_fftxn*spurno];
a2=0;
b2=0;
re=0;
im=0;
ni=ffts_na;
for(i=0; i<spur_speknum; i++)
  {
  a2+=z[4*ni  ]*z[4*ni  ]+z[4*ni+1]*z[4*ni+1];
  b2+=z[4*ni+2]*z[4*ni+2]+z[4*ni+3]*z[4*ni+3];
  re+=z[4*ni  ]*z[4*ni+2]+z[4*ni+1]*z[4*ni+3];
  im+=z[4*ni+1]*z[4*ni+2]-z[4*ni  ]*z[4*ni+3];
  ni=(ni+fftxn_mask)&fftxn_mask;
  }
t1=a2+b2;
a2/=t1;
b2/=t1;
re/=t1;
im/=t1;
t2=re*re+im*im;
noi2=a2*b2-t2;
a2s=a2-noi2;
b2s=b2-noi2;
if(b2s <=0.0001 || t2 == 0)return 0;
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
t1=c1*sp_c1-c2*sp_c2-c3*sp_c3;
t2=c3*sp_c2-c2*sp_c3;
t3=c1*sp_c2+c2*sp_c1;
t4=c1*sp_c3+c3*sp_c1;
// We want t2 to be zero so we adjust the phase.
c1=sqrt(t1*t1+t2*t2);
r2=sqrt(t3*t3+t4*t4);
r1=atan2(t3,t4)+atan2(t1,t2);
c2=-r2*cos(r1);
c3=r2*sin(r1);
t1=sqrt(c1*c1+c2*c2+c3*c3);
t2=c1*sp_c1+c2*sp_c2+c3*sp_c3;
if(t2 < 0)t1=-t1;
c1/=t1;
c2/=t1;
c3/=t1;
t2=sqrt((c1-sp_c1)*(c1-sp_c1)+(c2-sp_c2)*(c2-sp_c2)+(c3-sp_c3)*(c3-sp_c3));
t2/=spur_speknum;
t1=spur_speknum-1;
sp_c1=(t1*sp_c1+c1)/spur_speknum;
sp_c2=(t1*sp_c2+c2)/spur_speknum;
sp_c3=(t1*sp_c3+c3)/spur_speknum;
t1=sqrt(sp_c1*sp_c1+sp_c2*sp_c2+sp_c3*sp_c3);
if(sp_c2 < 0)t1=-t1;
sp_c1/=t1;
sp_c2/=t1;
sp_c3/=t1;
i=3*spurno;
spur_pol[i  ]=sp_c1; 
spur_pol[i+1]=sp_c2;
spur_pol[i+2]=sp_c3;
if(t2 > 0.05)return 1; 
return 0;
}

void remove_spur(int ia)
{
int i;
spur_flag[ia]=spur_flag[no_of_spurs];
spur_location[ia]=spur_location[no_of_spurs];
spur_d0pha[ia]=spur_d0pha[no_of_spurs];
spur_d1pha[ia]=spur_d1pha[no_of_spurs];
spur_d2pha[ia]=spur_d2pha[no_of_spurs];
spur_ampl[ia]=spur_ampl[no_of_spurs];
spur_noise[ia]=spur_noise[no_of_spurs];
spur_avgd2[ia]=spur_avgd2[no_of_spurs];
spur_pol[ia]=spur_pol[no_of_spurs];
spur_freq[ia]=spur_freq[no_of_spurs];
if(genparm[SECOND_FFT_ENABLE] !=0 && fft_cntrl[FFT2_CURMODE].mmx != 0)
  {
  for(i=0; i<spur_block; i++)
    {
    spur_table_mmx[ia*spur_block+i]=spur_table_mmx[no_of_spurs*spur_block+i];
    }
  }
else
  {
  for(i=0; i<spur_block; i++)
    {
    spur_table[ia*spur_block+i]=spur_table[no_of_spurs*spur_block+i];
    }
  }  
for(i=0; i<twice_rxchan*max_fftxn; i++)
  {
  spur_signal[twice_rxchan*max_fftxn*ia+i]=
                          spur_signal[twice_rxchan*max_fftxn*no_of_spurs+i];
  }        
for(i=0; i<max_fftxn; i++)
  {
  spur_ind[max_fftxn*ia+i]=spur_ind[max_fftxn*no_of_spurs+i];
  }
}

void refine_pll_parameters(void)
{
int i, ni;
float r1;
float a1, a2, b1, b2, d1, d2;
float phase, phase_slope, phase_curv;
float *zsig;
// We have at least one new data point in spur_signal.
// The current set of pll parameters give a good description
// of the spur signal for the old points.
// Split spur_signal in one component that is in phase with the pll LO
// and one component that is orthogonal.
zsig=&spur_signal[twice_rxchan*max_fftxn*spurno];
ni=ffts_na;
phase=spur_d0pha[spurno];
phase_slope=spur_d1pha[spurno];
phase_curv=spur_d2pha[spurno];
phase_slope+=phase_curv;
phase+=phase_slope;
a1=cos(phase);
a2=sin(phase);
b1=cos(phase_slope);
b2=sin(phase_slope);
d1=cos(phase_curv);
d2=sin(phase_curv);
for(i=spur_speknum-1; i>=0; i--)
  {
  sp_sig[2*i  ]=a1*zsig[twice_rxchan*ni  ]+a2*zsig[twice_rxchan*ni+1];
  sp_sig[2*i+1]=a1*zsig[twice_rxchan*ni+1]-a2*zsig[twice_rxchan*ni  ];
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r1=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r1;
  ni=(ni+fftxn_mask)&fftxn_mask;
  }
spur_phase_parameters();
phase+=sp_d0;
phase_slope+=sp_d1;
phase_curv+=sp_d2;
phase-=phase_slope;
phase_slope-=phase_curv;
spur_d0pha[spurno]=phase;
spur_d1pha[spurno]=phase_slope;
spur_d2pha[spurno]=phase_curv;
}

int spur_relock(void)
{
int i, j, k, np, nx, ni, pa, izz, ind, pnt, maxrem;
float t1,t2,t3,t4,r1,r2,r3,r4,p1,p2;
float a1,a2,b1,b2,d1,d2;
float rot;
float *z, *pwr, *sumsq;
float freq, phase, phase_slope, phase_curv;
short int *zmmx;
TWOCHAN_POWER *pxy, *xysum;
int *uind;
// Store old transforms in spur table for the current spur and 
// accumulate power spectrum.
spurp0=spurno*spur_block;
np=(ffts_na-spur_speknum+max_fftxn)&fftxn_mask;
nx=(ffts_na+1)&fftxn_mask;
if(swmmx_fft2)
  {
  if(sw_onechan)
    {
// With one channel only, just use the power spectrum.  
    for(i=0; i<SPUR_WIDTH; i++)spur_power[i]=0;
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1=spur_table_mmx[2*i+k  ];
        t2=spur_table_mmx[2*i+k+1];
        spur_power[i]+=t1*t1+t2*t2;
        }
      np=(np+1)&fftxn_mask;
      }  
    }
  else  //ui.rx_rf_channels = 2
    {
// We have two channels and polarization is unknown.
// First make an average of channel powers and correlations.
    for(i=0; i<SPUR_WIDTH; i++)
      {
      spur_pxy[i].x2=0;
      spur_pxy[i].y2=0;
      spur_pxy[i].re_xy=0;
      spur_pxy[i].im_xy=0;
      }
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1=spur_table_mmx[4*i+k  ];
        t2=spur_table_mmx[4*i+k+1];
        t3=spur_table_mmx[4*i+k+2];
        t4=spur_table_mmx[4*i+k+3];
        spur_pxy[i].x2+=t1*t1+t2*t2;
        spur_pxy[i].y2+=t3*t3+t4*t4;
        spur_pxy[i].re_xy+=t1*t3+t2*t4;
        spur_pxy[i].im_xy+=t2*t3-t1*t4;
        }
      np=(np+1)&fftxn_mask;
      }
    }
  }
else
  {  
  if(sw_onechan)
    {
// With one channel only, just use the power spectrum.  
    for(i=0; i<SPUR_WIDTH; i++)spur_power[i]=0;
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1=spur_table[2*i+k  ];
        t2=spur_table[2*i+k+1];
        spur_power[i]+=t1*t1+t2*t2;
        }
      np=(np+1)&fftxn_mask;
      }  
    }
  else  //ui.rx_rf_channels = 2
    {
// We have two channels and polarization is unknown.
// First make an average of channel powers and correlations.
    for(i=0; i<SPUR_WIDTH; i++)
      {
      spur_pxy[i].x2=0;
      spur_pxy[i].y2=0;
      spur_pxy[i].re_xy=0;
      spur_pxy[i].im_xy=0;
      }
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1=spur_table[4*i+k  ];
        t2=spur_table[4*i+k+1];
        t3=spur_table[4*i+k+2];
        t4=spur_table[4*i+k+3];
        spur_pxy[i].x2+=t1*t1+t2*t2;
        spur_pxy[i].y2+=t3*t3+t4*t4;
        spur_pxy[i].re_xy+=t1*t3+t2*t4;
        spur_pxy[i].im_xy+=t2*t3-t1*t4;
        }
      np=(np+1)&fftxn_mask;
      }
    }
  }
if(sw_onechan)
  {
  spurspek_norm();  
  }
else
  {
// Two channels. Extract the power for a signal using the channel 
// correlation (Slightly better than just adding x2 and y2)
  for(i=0; i<SPUR_WIDTH; i++)
    {
    t1=spur_pxy[i].x2+spur_pxy[i].y2;
    spur_power[i]=t1+2*(spur_pxy[i].re_xy*spur_pxy[i].re_xy+
                        spur_pxy[i].im_xy*spur_pxy[i].im_xy
                       -spur_pxy[i].x2*spur_pxy[i].y2 ) / t1;
    }
  spurspek_norm();  
  make_spur_pol();
  }


spur_freq[spurno]=spur_location[spurno]+(float)(SPUR_SIZE)*0.5;  
if(make_spur_freq() != 0)
  {
  return FALSE;
  }
if(spur_freq[spurno]+2-spur_location[spurno]-SPUR_SIZE/2 > 
                                               SPUR_WIDTH/2-0.5)return FALSE;
// The power spectrum has a maximum within range.
// Calculate spur_signal using the power spectrum.
if(spur_phase_lock(nx)!=0)return FALSE;
// Locking was sucessful.
// Remove the spur backwards in time.
// First find out how many old transforms we still did not use completely.
// Make k the number of transforms we can change to correct the latest
// and still unwritten line of the waterfall graph.
// Make j the number of transforms that will affect the output.
j=0;
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  maxrem=1+fft1_sumsq_counter; 
  maxrem+=((fft1_sumsq_pa-fft1_sumsq_pwg+fft1_sumsq_bufsize)&
                                                fft1_sumsq_mask)/fft1_size; 
  maxrem+=wg_waterf_sum_counter;
  if(new_baseb_flag >= 0)
    {
    j=(fft1_nb-fft1_nx+max_fft1n)&fft1n_mask;
    if(j<fft1n_mask)j++;
    }
  }
else
  {
  maxrem=wg_waterf_sum_counter;
  if(maxrem < hg.spek_avgnum)maxrem=hg.spek_avgnum;
  if(new_baseb_flag >= 0)
    {
    j=(fft2_na-fft2_nx+max_fft2n)&fft2n_mask;
    if(j<fft2n_mask)j++;
    }
  }  
if(j>maxrem)maxrem=j;
if(maxrem > fftxn_mask)maxrem=fftxn_mask;
if(maxrem >= spur_flag[spurno])
  {
  maxrem=spur_flag[spurno];
  }
spur_flag[spurno]=0;
if(maxrem > spur_speknum)
  {
  maxrem-=spur_speknum;
  nx=(ffts_na-spur_speknum+max_fftxn)&fftxn_mask;
  }
else
  {
  nx=(ffts_na-maxrem+max_fftxn)&fftxn_mask;
  maxrem=0;
  }  
// First remove the spur from the transforms we have used to get the 
// parameters from.
phase=spur_d0pha[spurno];
phase_slope=spur_d1pha[spurno];
phase_curv=spur_d2pha[spurno];
phase_slope+=phase_curv;
phase+=phase_slope;
freq=spur_freq[spurno];
a1=spur_ampl[spurno]*cos(phase);
a2=spur_ampl[spurno]*sin(phase);
b1=cos(phase_slope);
b2=sin(phase_slope);
d1=cos(spur_d2pha[spurno]);
d2=sin(spur_d2pha[spurno]);
pnt=spur_location[spurno];
izz=0;
ni=ffts_na;
uind=&spur_ind[spurno*max_fftxn];
while(ni != nx)
  {
  rot=-0.5*phase_slope/PI_L;
  i=freq*spur_freq_factor-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
  if(j < -1 || j > 2)return FALSE;
  if(j<0)j=0;
  j=1-j;
  if(j<0)j=0;
  ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
  if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
  ind=ind*SPUR_SIZE+j;
  uind[ni]=ind;
  if(sw_onechan)
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-a1;
      t2=-a2;
      }
    else
      {  
      t1=a1;
      t2=a2;
      }
    pwr=&fftx_pwr[ni*fftx_size+pnt];
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[2*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        r1=zmmx[2*i  ]-spur_spectra[ind+i]*t1;
        r2=zmmx[2*i+1]-spur_spectra[ind+i]*t2;
        zmmx[2*i  ]=r1;
        zmmx[2*i+1]=r2;
        pwr[i]=r1*r1+r2*r2;
        }
      }
    else
      {
      z=&fftx[2*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[2*i  ]-=spur_spectra[ind+i]*t1;
        z[2*i+1]-=spur_spectra[ind+i]*t2;
        pwr[i]=z[2*i]*z[2*i]+z[2*i+1]*z[2*i+1];
        }
      }
    }
  else
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-sp_c1*a1;
      t2=-sp_c1*a2;
      t3=-sp_c2*a1+sp_c3*a2;
      t4=-sp_c2*a2-sp_c3*a1;
      }
    else
      {
      t1=sp_c1*a1;
      t2=sp_c1*a2;
      t3=sp_c2*a1-sp_c3*a2;
      t4=sp_c2*a2+sp_c3*a1;
      }
    pxy=(TWOCHAN_POWER*)(&fftx_xypower[np*fftx_size+pnt].x2);
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[4*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        r1=zmmx[4*i  ]-spur_spectra[ind+i]*t1;
        r2=zmmx[4*i+1]-spur_spectra[ind+i]*t2;
        r3=zmmx[4*i+2]-spur_spectra[ind+i]*t3;
        r4=zmmx[4*i+3]-spur_spectra[ind+i]*t4;
        zmmx[4*i  ]=r1;
        zmmx[4*i+1]=r2;
        zmmx[4*i+2]=r3;
        zmmx[4*i+3]=r4;
        pxy[i].x2=r1*r1+r2*r2;
        pxy[i].y2=r3*r3+r4*r4;
        pxy[i].re_xy=r1*r3+r2*r4;
        pxy[i].im_xy=r2*r3-r1*r4;
        }
      }
    else
      {
      z=&fftx[4*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[4*i  ]-=spur_spectra[ind+i]*t1;
        z[4*i+1]-=spur_spectra[ind+i]*t2;
        z[4*i+2]-=spur_spectra[ind+i]*t3;
        z[4*i+3]-=spur_spectra[ind+i]*t4;
        pxy[i].x2=z[2*i  ]*z[2*i  ]+z[2*i+1]*z[2*i+1];
        pxy[i].y2=z[2*i+2]*z[2*i+2]+z[2*i+3]*z[2*i+3];
        pxy[i].re_xy=z[2*i  ]*z[2*i+2]+z[2*i+1]*z[2*i+3];
        pxy[i].im_xy=z[2*i+1]*z[2*i+2]-z[2*i  ]*z[2*i+3];
        }
      }
    }
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r2=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r2;
  phase_slope-=phase_curv;
  ni=(ni+fftxn_mask)&fftxn_mask;
  izz++;
  }
// Step further backwards while checking that the total power in the
// transform is actually reduced.
while(maxrem >0)
  {
  maxrem--;
  rot=-0.5*phase_slope/PI_L;
  i=freq*spur_freq_factor-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
  if(j < -1 || j > 2)return FALSE;
  if(j<0)j=0;
  j=1-j;
  if(j<0)j=0;
  ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
  if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
  ind=ind*SPUR_SIZE+j;
  p1=0;
  p2=0;
  if(sw_onechan)
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-a1;
      t2=-a2;
      }
    else
      {  
      t1=a1;
      t2=a2;
      }
    pwr=&fftx_pwr[ni*fftx_size+pnt];
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[2*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        p1+=pwr[i];
        r1=zmmx[2*i  ]-spur_spectra[ind+i]*t1;
        r2=zmmx[2*i+1]-spur_spectra[ind+i]*t2;
        zmmx[2*i  ]=t1;
        zmmx[2*i+1]=t2;
        pwr[i]=r1*r1+r2*r2;
        p2+=pwr[i];
        }
      if(p2 > p1)
        {
        for(i=0; i<SPUR_WIDTH; i++)
          {
          r1=zmmx[2*i  ]-spur_spectra[ind+i]*t1;
          r2=zmmx[2*i+1]-spur_spectra[ind+i]*t2;
          zmmx[2*i  ]=r1;
          zmmx[2*i+1]=r2;
          pwr[i]=r1*r1+r2*r2;
          }
        }
      }
    else
      {
      z=&fftx[2*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        p1+=pwr[i];
        z[2*i  ]-=spur_spectra[ind+i]*t1;
        z[2*i+1]-=spur_spectra[ind+i]*t2;
        pwr[i]=z[2*i]*z[2*i]+z[2*i+1]*z[2*i+1];
        p2+=pwr[i];
        }
      if(p2 > p1)
        {
        for(i=0; i<SPUR_WIDTH; i++)
          {
          z[2*i  ]+=spur_spectra[ind+i]*t1;
          z[2*i+1]+=spur_spectra[ind+i]*t2;
          pwr[i]=z[2*i]*z[2*i]+z[2*i+1]*z[2*i+1];
          }
        }
      }
    }
  else
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-sp_c1*a1;
      t2=-sp_c1*a2;
      t3=-sp_c2*a1+sp_c3*a2;
      t4=-sp_c2*a2-sp_c3*a1;
      }
    else
      {
      t1=sp_c1*a1;
      t2=sp_c1*a2;
      t3=sp_c2*a1-sp_c3*a2;
      t4=sp_c2*a2+sp_c3*a1;
      }
    pxy=(TWOCHAN_POWER*)(&fftx_xypower[np*fftx_size+pnt].x2);
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[4*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        r1=zmmx[4*i  ]-spur_spectra[ind+i]*t1;
        r2=zmmx[4*i+1]-spur_spectra[ind+i]*t2;
        r3=zmmx[4*i+2]-spur_spectra[ind+i]*t3;
        r4=zmmx[4*i+3]-spur_spectra[ind+i]*t4;
        zmmx[4*i  ]=r1;
        zmmx[4*i+1]=r2;
        zmmx[4*i+2]=r3;
        zmmx[4*i+3]=r4;
        p1+=pxy[i].x2+pxy[i].y2;
        pxy[i].x2=r1*r1+r2*r2;
        pxy[i].y2=r3*r3+r4*r4;
        p2+=pxy[i].x2+pxy[i].y2;
        pxy[i].re_xy=r1*r3+r2*r4;
        pxy[i].im_xy=r2*r3-r1*r4;
        }
      if(p2 > p1)
        {
        for(i=0; i<SPUR_WIDTH; i++)
          {
          r1=zmmx[4*i  ]+spur_spectra[ind+i]*t1;
          r2=zmmx[4*i+1]+spur_spectra[ind+i]*t2;
          r3=zmmx[4*i+2]+spur_spectra[ind+i]*t3;
          r4=zmmx[4*i+3]+spur_spectra[ind+i]*t4;
          zmmx[4*i  ]=r1;
          zmmx[4*i+1]=r2;
          zmmx[4*i+2]=r3;
          zmmx[4*i+3]=r4;
          pxy[i].x2=r1*r1+r2*r2;
          pxy[i].y2=r3*r3+r4*r4;
          pxy[i].re_xy=r1*r3+r2*r4;
          pxy[i].im_xy=r2*r3-r1*r4;
          }
        }
      }
    else
      {
      z=&fftx[4*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[4*i  ]-=spur_spectra[ind+i]*t1;
        z[4*i+1]-=spur_spectra[ind+i]*t2;
        z[4*i+2]-=spur_spectra[ind+i]*t3;
        z[4*i+3]-=spur_spectra[ind+i]*t4;
        p1+=pxy[i].x2+pxy[i].y2;
        pxy[i].x2=z[2*i  ]*z[2*i  ]+z[2*i+1]*z[2*i+1];
        pxy[i].y2=z[2*i+2]*z[2*i+2]+z[2*i+3]*z[2*i+3];
        p2+=pxy[i].x2+pxy[i].y2;
        pxy[i].re_xy=z[2*i  ]*z[2*i+2]+z[2*i+1]*z[2*i+3];
        pxy[i].im_xy=z[2*i+1]*z[2*i+2]-z[2*i  ]*z[2*i+3];
        }
      if(p2 > p1)
        {
        for(i=0; i<SPUR_WIDTH; i++)
          {
          z[4*i  ]+=spur_spectra[ind+i]*t1;
          z[4*i+1]+=spur_spectra[ind+i]*t2;
          z[4*i+2]+=spur_spectra[ind+i]*t3;
          z[4*i+3]+=spur_spectra[ind+i]*t4;
          pxy[i].x2=z[2*i  ]*z[2*i  ]+z[2*i+1]*z[2*i+1];
          pxy[i].y2=z[2*i+2]*z[2*i+2]+z[2*i+3]*z[2*i+3];
          pxy[i].re_xy=z[2*i  ]*z[2*i+2]+z[2*i+1]*z[2*i+3];
          pxy[i].im_xy=z[2*i+1]*z[2*i+2]-z[2*i  ]*z[2*i+3];
          }
        }
      }
    }
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r2=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r2;
  phase_slope-=phase_curv;
  ni=(ni+fftxn_mask)&fftxn_mask;
  izz++;
  }
if(izz == 0)return FALSE;
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if(sw_onechan)
    {
    // Update fft1_sumsq
    if(fft1_sumsq_counter != 0)
      {
      izz-=fft1_sumsq_counter;
      sumsq=&fft1_sumsq[fft1_sumsq_pa+fft1_first_point+pnt];
      for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;    
      ni=fft1_nb;
      for(k=0; k<fft1_sumsq_counter; k++)
        {
        ni=(ni+fft1n_mask)&fft1n_mask;
        pwr=&fft1_power[ni*fft1_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          fft1_sumsq[i]+=pwr[i];
          }
        }    
      }
    pa=fft1_sumsq_pa;  
    while(izz > 0)
      { 
      izz-=wg.fft_avg1num;
      pa=(pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
      sumsq=&fft1_sumsq[pa+fft1_first_point+pnt];
      for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;    
      for(k=0; k<wg.fft_avg1num; k++)
        {
        ni=(ni+fft1n_mask)&fft1n_mask;
        pwr=&fft1_power[ni*fft1_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sumsq[i]+=pwr[i];
          }  
        }
      }
    }
  else  
    {
    // Update fft1_sumsq
    if(fft1_sumsq_counter != 0)
      {
      izz-=fft1_sumsq_counter;
      sumsq=&fft1_sumsq[fft1_sumsq_pa+fft1_first_point+pnt];
      for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;    
      ni=fft1_nb;
      for(k=0; k<fft1_sumsq_counter; k++)
        {
        ni=(ni+fft1n_mask)&fft1n_mask;
        pxy=&fftx_xypower[ni*fftx_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          fft1_sumsq[i]+=pxy[i].x2+pxy[i].y2;
          }
        }    
      }
    pa=fft1_sumsq_pa;  
    while(izz > 0)
      { 
      izz-=wg.fft_avg1num;
      pa=(pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
      sumsq=&fft1_sumsq[pa+fft1_first_point+pnt];
      for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;    
      for(k=0; k<wg.fft_avg1num; k++)
        {
        ni=(ni+fft1n_mask)&fft1n_mask;
        pxy=&fftx_xypower[ni*fftx_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sumsq[i]+=pxy[i].x2+pxy[i].y2;
          }  
        }
      }
    i=(fft1_sumsq_pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask; 
    new_fft1_averages(i, pnt+1,pnt+SPUR_WIDTH-1);
    pa=fft1_sumsq_pa;  
    if(wg_waterf_sum_counter!=0)
      {
      k=wg_waterf_sum_counter;
      for(i=0; i<SPUR_WIDTH; i++)wg_waterf_sum[pnt+i]=0;    
      while(k>0)
        {
        pa=(pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
        sumsq=&fft1_sumsq[pa+fft1_first_point+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          wg_waterf_sum[pnt+i]+=sumsq[i];
          }
        k-=wg.fft_avg1num;
        }
      }  
    }
  i=(fft1_sumsq_pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask; 
  new_fft1_averages(i,pnt+1,pnt+SPUR_WIDTH-1);
  pa=fft1_sumsq_pa;  
  if(wg_waterf_sum_counter!=0)
    {
    k=wg_waterf_sum_counter;
    for(i=0; i<SPUR_WIDTH; i++)wg_waterf_sum[pnt+i]=0;    
    while(k>0)
      {
      pa=(pa-fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
      sumsq=&fft1_sumsq[pa+fft1_first_point+pnt];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        wg_waterf_sum[pnt+i]+=sumsq[i];
        }
      k-=wg.fft_avg1num;
      }
    }  
  }
else
  {
  if(sw_onechan)
    {
// Recalculate fft2_powersum_float which is used for the waterfall display.
// do not worry to correct what is already on screen. 
// Could be done if anyone cares to.
    if(wg_waterf_sum_counter != 0 && wg.waterfall_avgnum !=1)
      {
      ni=fft2_na;
      sumsq=&fft2_powersum_float[pnt];
      for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;
      for(j=0; j<wg_waterf_sum_counter; j++)
        {
        ni=(ni+fft2n_mask)&fft2n_mask;
        pwr=&fftx_pwr[ni*fftx_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sumsq[i]+=pwr[i];
          }
        }
      }  
// Recalculate the high resolution graph in case a frequency is selected.
    if(mix1_selfreq[0]>0)
      {
      if(hg.spek_avgnum > 2)
        {
        if(pnt >= hg_first_point && pnt+SPUR_WIDTH < hg_last_point)
          {
          ni=fft2_nb;
          sumsq=&hg_fft2_pwrsum[pnt-hg_first_point];
          for(i=0; i<SPUR_WIDTH; i++)sumsq[i]=0;
          while(ni != fft2_na)
            {
            pwr=&fftx_pwr[ni*fftx_size+pnt];
            for(i=0; i<SPUR_WIDTH; i++)sumsq[i]+=pwr[i];
            ni=(ni+1)&fft2n_mask;
            }
          }
        }
      }
    }  
  else
    {
// Recalculate fft2_xysum_float which is used for the waterfall display.
// do not worry to correct what is already on screen. 
// Could be done if anyone cares to.
    if(wg_waterf_sum_counter != 0 && wg.waterfall_avgnum !=1)
      {
      ni=fft2_na;
      xysum=&fft2_xysum[pnt];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        xysum[i].x2=0;
        xysum[i].y2=0;
        xysum[i].re_xy=0;
        xysum[i].im_xy=0;
        }
      for(j=0; j<wg_waterf_sum_counter; j++)
        {
        ni=(ni+fft2n_mask)&fft2n_mask;
        pxy=&fftx_xypower[ni*fftx_size+pnt];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          xysum[i].x2+=pxy[i].x2;
          xysum[i].y2+=pxy[i].y2;
          xysum[i].re_xy+=pxy[i].re_xy;
          xysum[i].im_xy+=pxy[i].im_xy;
          }
        }
      }  
// Recalculate the high resolution graph in case a frequency is selected.
    if(mix1_selfreq[0]>0)
      {
      if(hg.spek_avgnum > 2)
        {
        if(pnt >= hg_first_point && pnt+SPUR_WIDTH < hg_last_point)
          {
          ni=fft2_nb;
          sumsq=&hg_fft2_pwrsum[2*(pnt-hg_first_point)];
          for(i=0; i<2*SPUR_WIDTH; i++)sumsq[i]=0;
          if(swmmx_fft2)
            {
            while(ni != fft2_na)
              {
              pwr=&hg_fft2_pwr[2*(ni*hg_size+pnt-hg_first_point)];
              zmmx=&fft2_short_int[4*(ni*fftx_size+pnt)];
              for(i=0; i<SPUR_WIDTH; i++)
                {
                t1=zmmx[4*i  ];
                t2=zmmx[4*i+1];
                t3=zmmx[4*i+2];
                t4=zmmx[4*i+3];
                r1=pg.c1*t1+pg.c2*t3+pg.c3*t4;
                r2=pg.c1*t2+pg.c2*t4-pg.c3*t3;
                pwr[2*i  ]=r1*r1+r2*r2;
                sumsq[2*i  ]+=pwr[2*i  ];
                r1=pg.c1*t3-pg.c2*t1+pg.c3*t2;
                r2=pg.c1*t4-pg.c2*t2-pg.c3*t1;
                pwr[2*i+1]=r1*r1+r2*r2;
                sumsq[2*i+1]+=pwr[2*i+1];
                }
              ni=(ni+1)&fft2n_mask;
              }
            }
          else  
            {  
            while(ni != fft2_na)
              {
              pwr=&hg_fft2_pwr[2*(ni*hg_size+pnt-hg_first_point)];
              z=&fftx[4*(ni*fftx_size+pnt)];
              for(i=0; i<SPUR_WIDTH; i++)
                {
                t1=z[4*i  ];
                t2=z[4*i+1];
                t3=z[4*i+2];
                t4=z[4*i+3];
                r1=pg.c1*t1+pg.c2*t3+pg.c3*t4;
                r2=pg.c1*t2+pg.c2*t4-pg.c3*t3;
                pwr[2*i  ]=r1*r1+r2*r2;
                sumsq[2*i  ]+=pwr[2*i  ];
                r1=pg.c1*t3-pg.c2*t1+pg.c3*t2;
                r2=pg.c1*t4-pg.c2*t2-pg.c3*t1;
                pwr[2*i+1]=r1*r1+r2*r2;
                sumsq[2*i+1]+=pwr[2*i+1];
                }
              ni=(ni+1)&fft2n_mask;
              }
            }
          }
        }
      }
    }  
  }
return TRUE;
}

void spur_phase_parameters(void)
{
int i;
int ia,na;
float t1,t2,t3,r1,r2;
float a1, a2, b1, b2, d1, d2;
// We want to extract phase and it's derivatives from the
// complex amplitude.
// We can improve S/N by averaging!
// The phase may rotate a few turns over the time interval 
// so averaging is somewhat complicated.    
// First get the derivative as the cross product.
// Assuming the spur is present all the time we divide out
// the amplitude to have the phase only as complex numbers.
for(i=1; i<spur_speknum; i++) 
  {
  t1=sp_sig[2*i  ]*sp_sig[2*i-2]+sp_sig[2*i+1]*sp_sig[2*i-1];
  t2=sp_sig[2*i+1]*sp_sig[2*i-2]-sp_sig[2*i  ]*sp_sig[2*i-1];
  r1=sqrt(t1*t1+t2*t2);
  if(r1>0.000000001)
    {
    sp_der[2*i-2]=t1/r1;
    sp_der[2*i-1]=t2/r1;
    }
  else
    {
    sp_der[2*i-2]=0;
    sp_der[2*i-1]=0;
    }
  }
// The derivative is amplitude*frequency offset plus random noise.
// The frequency offset will not vary fast with time so we can average
// out the noise.
complex_lowpass(sp_der, sp_tmp, sp_avgnum, sp_numsub);
if(kill_all_flag)return;
// Get the average phase of the second derivative, 
// also from the cross product.
r1=0;
r2=0;
for(i=1+sp_avgnum/2; i<sp_numsub-sp_avgnum/2; i++) 
  {
  t1=sp_tmp[2*i  ]*sp_tmp[2*i-2]+sp_tmp[2*i+1]*sp_tmp[2*i-1];
  t2=sp_tmp[2*i+1]*sp_tmp[2*i-2]-sp_tmp[2*i  ]*sp_tmp[2*i-1];
  t3=sqrt(t1*t1+t2*t2);
  if(t3 > 0.00001)
    {
    r1+=t1/t3;
    r2+=t2/t3;
    }
  }
sp_d2=atan2(r2,r1);
// If the second derivative is too big, we have an error of some kind.
// Interference or a frequency/phase jump
t1=spur_d2pha[spurno]+sp_d2;
if( fabs(t1) > spur_max_d2 && fabs(sp_d2) > spur_max_d2)
  {
// If the curvature we would get is too large, skip the new value
// and allow the curvature to drift towards zero  
  sp_d2=-spur_d2pha[spurno]/spur_speknum;
  }
else
  {
// If S/N for the spur is poor, do not allow second derivative
// to change rapidly, for a weak signal we would introduce noise.
  t1=spur_weiold*spur_avgd2[spurno]+spur_weinew*t1;
  if(spur_noise[spurno]>0.000001 && fabs(spur_ampl[spurno])>0.000001)
    {
    t2=0.1*fabs(spur_ampl[spurno])/spur_noise[spurno];
    t2=1/(1+t2);
    }
  else
    {
    t2=1;
    }
  sp_d2=t2*(t1-spur_d2pha[spurno])+(1-t2)*sp_d2;
  }
// Extract the phase part and skip the amplitude part of the derivative.
for(i=0; i<sp_numsub; i++)
  {
  sp_pha[i]=atan2(sp_tmp[2*i+1],sp_tmp[2*i]);
  }
remove_phasejumps(sp_pha, sp_numsub);
// Now we integrate the phase derivative to get the S/N improved phase of
// the spur. 
sp_pha[sp_numsub]=0;
for(i=sp_numsub; i>0; i--) 
  {
  sp_pha[i-1]=sp_pha[i]-sp_pha[i-1];
  }
// The phase function has an unknown integration constant but
// we can use it to get derivatives.
// sp_avgnum/2 points at each end come from averages, avoid them, they degrade
// the statistics in the derivative calculations.
// We want the derivatives to be valid for the latest 
// data point, spur_speknum-1
// It is safe to assume that d2pha is a constant, but
// the fact that it is non-zero means that d1pha varies
// with the transform number.
// phase(n) = phase(0) - n*(d1pha+d2pha/2) + n*n*d2pha/2
// where n is transform number counted backwards from ffts_na-1
// The phase shift caused by the second derivative is:
// dph = - n*d2err/2 + n*n*d2err/2 = 0.5*n*(n-1)*d2err
// Adjust sp_pha by adding dph so the phase function will
// become a straight line from which we can get the first derivative.
t1=sp_d2*0.5;
for(i=2; i<spur_speknum; i++)
  {
  sp_pha[sp_numsub-i]-=i*(i-1)*t1;
  }
na=spur_speknum-sp_avgnum;
if(na < 10)na=spur_speknum-sp_avgnum/2;
if(na < 3) na=spur_speknum;
ia=spur_speknum-na;
t1=average_slope(&sp_pha[ia], na);
sp_d1=t1;
//if(mailbox[1]==1)fprintf( stderr,"\nAA %f",sp_d1);
// Now that we have first and second derivatives we can construct
// a signal that accurately follows the frequency of the spur.
// Do not touch the first point. It is the integration
// constant which is still unknown.
// Set up the signal with amplitude 1 as a complex amplitude.
// This is a local oscillator for PLL!
// Twist the phase of the original complex amplitude sp_sig in
// accordance with the phase derivatives we now have.
// Accumulate the average phase angle.
b1=cos(sp_d1);
b2=sin(sp_d1);
a1=b1;
a2=b2;
d1=cos(sp_d2);
d2=sin(sp_d2);
t1=0;
t2=0;
for(i=sp_numsub; i>=0; i--)
  {
  r1=a1*sp_sig[2*i  ]+a2*sp_sig[2*i+1];
  r2=a1*sp_sig[2*i+1]-a2*sp_sig[2*i  ];
  sp_tmp[2*i  ]=r1;
  sp_tmp[2*i+1]=r2;
//if(mailbox[1]==1)fprintf( dmp,"\ntmp[%d]  %f %f  sig %f %f",i,
  //      sp_tmp[2*i],sp_tmp[2*i+1],sp_sig[2*i],sp_sig[2*i+1 ]); 

  t3=sqrt(r1*r1+r2*r2);
  if(t3>0)
    {
    t1+=r1/t3;
    t2+=r2/t3;
    r2+=t3*t3;
    }
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r1=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r1;
  }
sp_d0=atan2(t2,t1);
// Now twist the phase for the average phase to become zero.  
// Fit a straight line to phase to get a correction to sp_d1
// which was not calculated with optimum noise suppression before.
// Under the assumption that the phase angle is small atan(x) = x
// Under the same assumption the real part is the amplitude so
// we can extract the average amplitude now.
//if(mailbox[1]==1)fprintf( dmp,"\navg phase %f %f ",t2,t1);  

t3=sqrt(t1*t1+t2*t2);  
t1/=t3;
t2/=t3;
a1=0;
a2=0;
d1=-0.5*sp_numsub;
for(i=0; i<spur_speknum; i++)
  {
  r1=t1*sp_tmp[2*i  ]+t2*sp_tmp[2*i+1];
  r2=t1*sp_tmp[2*i+1]-t2*sp_tmp[2*i  ];
  sp_tmp[2*i  ]=r1;
  sp_tmp[2*i+1]=r2;
  a1+=r1;
  if(r1 >0 && fabs(r2) < fabs(r1))
    {
    a2+=d1*r2/fabs(r1);
    }
  else
    {
    a2+=d1*atan2(r2,r1);
    }
  d1+=1;
  }
a1/=spur_speknum;  
spur_ampl[spurno]=a1;
a2/=spur_linefit;
sp_d1+=a2;
//if(mailbox[1]==1)fprintf( stderr,"\nBB %f  %f",sp_d1,a2);
// Correct the phase in accordance with the improved first 
// derivative (signal frequency)
// The signal should have it's real part equal to the
// average amplitude and it's imaginary part equal to zero.
// Any deviation is noise (or a modulation on the spur)!
d2=-0.5*sp_numsub*a2;
b1=cos(a2);
b2=-sin(a2);
a1=cos(d2);
a2=sin(d2);
t1=0;
for(i=0; i<spur_speknum; i++)
  {
  r1=a1*sp_tmp[2*i  ]-a2*sp_tmp[2*i+1];
  r2=a1*sp_tmp[2*i+1]+a2*sp_tmp[2*i  ];
  sp_tmp[2*i  ]=r1;
  sp_tmp[2*i+1]=r2;
  t1+=r1;
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  }
t1/=spur_speknum;  
spur_ampl[spurno]=t1;
t2=0;
for(i=0; i<spur_speknum; i++)
  {
  t2+=(sp_tmp[2*i  ]-t1)*(sp_tmp[2*i  ]-t1)+sp_tmp[2*i+1]*sp_tmp[2*i+1];
  }
spur_noise[spurno]=sqrt(t2/spur_speknum);
}

 
