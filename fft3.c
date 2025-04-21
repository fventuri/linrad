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
#include "seldef.h"
#include "fft3def.h"
#include "fft1def.h"
#include "screendef.h"
#include "sigdef.h"
#include "thrdef.h"

int squelch [65536];

void do_fft3(void)
{
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_FFT3);
#endif
resume:;
thread_status_flag[THREAD_FFT3]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
         thread_command_flag[THREAD_FFT3]==THRFLAG_ACTIVE)
  {
// *******************************************************
wait:;
  if(kill_all_flag || 
     thread_command_flag[THREAD_FFT3] == THRFLAG_KILL)goto kill;
  thread_status_flag[THREAD_FFT3]=THRFLAG_SEM_WAIT;
  lir_sched_yield();
  lir_await_event(EVENT_FFT3);
  thread_status_flag[THREAD_FFT3]=THRFLAG_ACTIVE;
  lir_sched_yield();
  while(((timf3_pa-timf3_px+timf3_size)&timf3_mask) >= twice_rxchan*fft3_size &&
          ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) < fft3_totsiz-2*fft3_block)
    {
    if(mix1_selfreq[0] != old_mix1_selfreq)
      {
      lir_sleep(3000);
      goto wait;;
      }
    if(kill_all_flag ||
       thread_command_flag[THREAD_FFT3] == THRFLAG_KILL)goto kill;
    make_fft3_all();
    lir_sched_yield();
    }
  }
if(thread_command_flag[THREAD_FFT3] == THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_FFT3]=THRFLAG_IDLE;
  while(!kill_all_flag && thread_command_flag[THREAD_FFT3] == THRFLAG_IDLE)
    {
    lir_sleep(1000);
    }
  if(thread_command_flag[THREAD_FFT3] == THRFLAG_ACTIVE)goto resume;
  }
kill:;  
thread_status_flag[THREAD_FFT3]=THRFLAG_RETURNED;
while(!kill_all_flag)
  {
  if(thread_command_flag[THREAD_FFT3] == THRFLAG_KILL)return;
  lir_sleep(1000);
  }
}


void update_squelch(void)
{
int i, j, k, ia, ib;
float t1, t2, t3;
// To decide whether there is a signal in the passband or
// whether there is only noise we look at the statistics of
// fft3_slowsum within the passband as well as at the power
// within the passband in relation to the estimated noise floor
// outside the passband if the passband is a small enough fraction 
// of the baseband bandwidth.
ia=fft3_size/2-bg_filter_points-bg_first_xpoint;
ib=fft3_size/2+bg_filter_points-bg_first_xpoint;
k=0;
if(sw_onechan)
  {
  for(i=ia; i<=ib; i++)
    {
    squelch_info[k]=fft3_slowsum[i]; //ööÖÖ *bg_filterfunc[i] ??
    k++;
    }
  }
else
  {
  ia*=2;
  ib*=2;
  if(bg_twopol == 0)
    {
    for(i=ia; i<=ib; i+=2)
      {
      squelch_info[k]=fft3_slowsum[i];
      k++;
      }
    }  
  else
    {
    for(i=ia; i<=ib; i+=2)
      {
      squelch_info[k]=fft3_slowsum[i]+fft3_slowsum[i+1];
      k++;
      }
    }                      
  }
k--;  
for(i=0; i<k; i++)
  {
  for(j=i+1; j<=k; j++)
    {
    if(squelch_info[i] < squelch_info[j])
      {
      t1=squelch_info[i];
      squelch_info[i]=squelch_info[j];
      squelch_info[j]=t1;
      }
    }
  }      
j=1+0.8*k;
// compute the noise level from the 20% smallest data points.
t2=0;
for(i=j; i<=k; i++)
  {
  t2+=squelch_info[i];
  }
t2/=k-j+1;
if(t2==0)return;
// use a point between 0 and 50% of the full range.
j=bg.squelch_point*k/200;
t3=0.2*(squelch_info[j]-t2)*sqrt((float)(fft3_slowsum_cnt-0.75))/t2;  
if(t3 > pow(10,0.1*bg.squelch_level))
  {
  squelch_turnon_time=recent_time;
  }
}

void update_bg_waterf(void)
{
int i,k,m,ix;
float der,t1,y,yval;
// A new summed power spectrum has arrived.
// Convert it from linear power scale to log in units of 0.1dB.
// Expand or contract so we get the number of pixels that
// will fit on the screen.
// Store at the pos of our current line in waterfall
if(bg.pixels_per_point == 1)
  {
  for(ix=0; ix < bg_xpixels; ix++)
    {
    y=100*log10(bg_waterf_sum[ix]*bg_waterf_yfac);
    if(y < -32767)y=-32767;
    if(y>32767)y=32767;
    bg_waterf[bg_waterf_ptr+ix]=y;
    }
  }   
else
  {
// There are more pixels than data points so we must interpolate.
  y=100*log10(bg_waterf_sum[0]*bg_waterf_yfac);
  yval=y;
  if(y < -32767)y=-32767;
  if(y>32767)y=32767;
  bg_waterf[bg_waterf_ptr]=y;
  i=1;
  m=bg_xpixels-bg.pixels_per_point;
  for(ix=0; ix<m; ix+=bg.pixels_per_point)
    {
    t1=100*log10(bg_waterf_sum[i]*bg_waterf_yfac);
    der=(t1-yval)/bg.pixels_per_point;
    for(k=ix+1; k<=ix+bg.pixels_per_point; k++)
      {
      yval=yval+der;
      y=yval;
      if(y < -32767)y=32767;
      if(y>32767)y=32767;
      bg_waterf[bg_waterf_ptr+k]=y;
      }  
    yval=t1;
    i++;
    }      
  }
for(i=0; i < bg_xpoints; i++)
  {
  bg_waterf_sum[i]=0;
  }
bg_waterf_ptr-=bg_xpixels;
if(bg_waterf_ptr < 0)bg_waterf_ptr+=bg_waterf_size;
bg_waterf_sum_counter=0;
if( bg_waterf_lines != -1)awake_screen();
}

void make_fft3_all(void)
{
int ja,jb,im;
int i,iw,j,k,m,p0,ss,poffs;
int mm, ia, ib, ic, jr, pa, pb;
float t1,t2,t3,t4,x1,x2;
float r1,r2,r3,r4;
float amp, pos;
double dt1,dt2,dt3,dt4,dx1,dx2;
double ds1,ds2,ds3,ds4;
double dr1,dr2,dr3,dr4;
double *d_z;
float *z;
mm=twice_rxchan;
for(ss=0; ss<genparm[MIX1_NO_OF_CHANNELS]; ss++)
  {
  poffs=ss*mm*timf3_size;        
  z=&fft3[fft3_pa+ss*mm*fft3_size];
  d_z=&d_fft3[fft3_pa+ss*mm*fft3_size];
  if(mix1_selfreq[ss] >= 0)
    {
// Frequency no ss is selected.
// **********************************************************
// **************  ONE CHANNEL ******************************
    if(sw_onechan)
      {  
      pa=timf3_px;
      pb=(pa+fft3_size)&timf3_mask;
      ib=fft3_size-2;
      for( ia=0; ia<fft3_size/2; ia++)
        {
        ib+=2;
        t1=timf3_float[poffs+pa  ]*fft3_window[2*ia];
        t2=timf3_float[poffs+pa+1]*fft3_window[2*ia];      
        t3=timf3_float[poffs+pb  ]*fft3_window[2*ia+1];
        t4=timf3_float[poffs+pb+1]*fft3_window[2*ia+1];   
        x1=t1-t3;
        fft3_tmp[2*ia  ]=t1+t3;
        x2=t4-t2;
        fft3_tmp[2*ia+1]=t2+t4;
        pa=(pa+2)&timf3_mask;
        pb=(pb+2)&timf3_mask;
        fft3_tmp[ib  ]=fft3_tab[ia].cos*x1+fft3_tab[ia].sin*x2;
        fft3_tmp[ib+1]=fft3_tab[ia].sin*x1-fft3_tab[ia].cos*x2;
        } 
#if IA64 == 0 && CPU == CPU_INTEL
      asmbulk_of_dif(fft3_size, fft3_n, 
                                     fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#else
      bulk_of_dif(fft3_size, fft3_n, 
                                     fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#endif
      for(ia=0; ia < fft3_size; ia+=2)
        {
        ib=2*fft3_permute[ia];                               
        ic=2*fft3_permute[ia+1];                             
        z[ib  ]=fft3_tmp[2*ia  ]+fft3_tmp[2*ia+2];
        z[ic  ]=fft3_tmp[2*ia  ]-fft3_tmp[2*ia+2];
        z[ib+1]=fft3_tmp[2*ia+1]+fft3_tmp[2*ia+3];
        z[ic+1]=fft3_tmp[2*ia+1]-fft3_tmp[2*ia+3];
        }
      }
    else
      {  
// **************************************************************
// *************************  TWO CHANNELS *********************
      if(fft1_correlation_flag > 1)
        {
        pa=timf3_px;
        pb=(pa+2*fft3_size)&timf3_mask;
        ib=fft3_size-2;
        pa=timf3_px;
        pb=(pa+2*fft3_size)&timf3_mask;
        for( ia=0; ia<fft3_size/2; ia++)
          {
          ib+=2;
          dt1=d_timf3_float[poffs+pa  ]*d_fft3_window[2*ia];
          dt2=d_timf3_float[poffs+pa+1]*d_fft3_window[2*ia];      
          dt3=d_timf3_float[poffs+pb  ]*d_fft3_window[2*ia+1];
          dt4=d_timf3_float[poffs+pb+1]*d_fft3_window[2*ia+1];   
          dx1=dt1-dt3;
          d_timf3_tmp[4*ia  ]=dt1+dt3;
          dx2=dt4-dt2;
          d_timf3_tmp[4*ia+1]=dt2+dt4;
          d_timf3_tmp[2*ib  ]=d_fft3_tab[ia].cos*dx1+d_fft3_tab[ia].sin*dx2;
          d_timf3_tmp[2*ib+1]=d_fft3_tab[ia].sin*dx1-d_fft3_tab[ia].cos*dx2;
          dr1=d_timf3_float[poffs+pa+2]*d_fft3_window[2*ia];
          dr2=d_timf3_float[poffs+pa+3]*d_fft3_window[2*ia];      
          dr3=d_timf3_float[poffs+pb+2]*d_fft3_window[2*ia+1];
          dr4=d_timf3_float[poffs+pb+3]*d_fft3_window[2*ia+1];   
          dx1=dr1-dr3;
          d_timf3_tmp[4*ia+2]=dr1+dr3;
          dx2=dr4-dr2;
          d_timf3_tmp[4*ia+3]=dr2+dr4;
          pa=(pa+4)&timf3_mask;
          pb=(pb+4)&timf3_mask;
          d_timf3_tmp[2*ib+2]=d_fft3_tab[ia].cos*dx1+d_fft3_tab[ia].sin*dx2;
          d_timf3_tmp[2*ib+3]=d_fft3_tab[ia].sin*dx1-d_fft3_tab[ia].cos*dx2;
          } 

        d_bulk_of_dual_dif(fft3_size, fft3_n, 
                                   d_timf3_tmp, d_fft3_tab, yieldflag_ndsp_fft3);

        for(ia=0; ia < fft3_size; ia+=2)
          {
          ib=4*fft3_bigpermute[ia  ];                               
          ic=4*fft3_bigpermute[ia+1];                             
          d_z[ib  ]=d_timf3_tmp[4*ia  ]+d_timf3_tmp[4*ia+4];
          z[ib  ]=d_timf3_tmp[4*ia  ]+d_timf3_tmp[4*ia+4];
          d_z[ic  ]=d_timf3_tmp[4*ia  ]-d_timf3_tmp[4*ia+4];
          z[ic  ]=d_timf3_tmp[4*ia  ]-d_timf3_tmp[4*ia+4];
          d_z[ib+1]=d_timf3_tmp[4*ia+1]+d_timf3_tmp[4*ia+5];
          z[ib+1]=d_timf3_tmp[4*ia+1]+d_timf3_tmp[4*ia+5];
          d_z[ic+1]=d_timf3_tmp[4*ia+1]-d_timf3_tmp[4*ia+5];
          z[ic+1]=d_timf3_tmp[4*ia+1]-d_timf3_tmp[4*ia+5];
          d_z[ib+2]=d_timf3_tmp[4*ia+2]+d_timf3_tmp[4*ia+6];
          z[ib+2]=d_timf3_tmp[4*ia+2]+d_timf3_tmp[4*ia+6];
          d_z[ic+2]=d_timf3_tmp[4*ia+2]-d_timf3_tmp[4*ia+6];
          z[ic+2]=d_timf3_tmp[4*ia+2]-d_timf3_tmp[4*ia+6];
          d_z[ib+3]=d_timf3_tmp[4*ia+3]+d_timf3_tmp[4*ia+7];
          z[ib+3]=d_timf3_tmp[4*ia+3]+d_timf3_tmp[4*ia+7];
          d_z[ic+3]=d_timf3_tmp[4*ia+3]-d_timf3_tmp[4*ia+7];
          z[ic+3]=d_timf3_tmp[4*ia+3]-d_timf3_tmp[4*ia+7];
          }
// Find the strongest signal within the baseband bandwidth.
        if(fft1_correlation_flag == 2)
          {
          if(corr_afc_count < MAX_CORR_AFC_COUNT)
            {
// increment MAX_CORR_AFC_COUNT times before trying to step the 
// frequency (again.)
            if(corr_afc_count<100)corr_afc_count++;
            }
          else
            { 
            ib=20*bg_carr_20db_points;
            if(ib > fft3_size/8)ib=fft3_size/8;
            ia=fft3_size/2-ib;
            ib+=fft3_size/2;
            dt1=0;
            ic=fft3_size/2;
            for(i=ia; i<=ib; i++)
              {
              dt2=d_z[4*i  ]*d_z[4*i  ]+d_z[4*i+1]*d_z[4*i+1]+
                  d_z[4*i+2]*d_z[4*i+2]+d_z[4*i+3]*d_z[4*i+3];
              if(dt2 > dt1)
                {
                ic=i;
                dt1=dt2;
                }
              }
            if(abs(ic-fft3_size/2) < (ib-ia)/2)
              {
              dt1=d_z[4*ic-4]*d_z[4*ic-4]+d_z[4*ic-3]*d_z[4*ic-3]+
                 d_z[4*ic-2]*d_z[4*ic-2]+d_z[4*ic-1]*d_z[4*ic-1];
              dt2=d_z[4*ic  ]*d_z[4*ic  ]+d_z[4*ic+1]*d_z[4*ic+1]+
                 d_z[4*ic+2]*d_z[4*ic+2]+d_z[4*ic+3]*d_z[4*ic+3];
              dt3=d_z[4*ic+4]*d_z[4*ic+4]+d_z[4*ic+5]*d_z[4*ic+5]+
                 d_z[4*ic+6]*d_z[4*ic+6]+d_z[4*ic+7]*d_z[4*ic+7];
              t1=sqrt(dt1);
              t2=sqrt(dt2);
              t3=sqrt(dt3);
              parabolic_fit(&amp, &pos, t1,t2,t3);
              pos+=ic;
              t1=pos-fft3_size/2;
// t1 is now the desired frequency step in bins.
              t2=0.25*(float)bg_carr_20db_points;;
// t2 is the smallest step we want to make.
// Never step less than 0.5 bins
              if(t2 < 0.5F)t2=0.5F;
              if(fabs(t1) > t2)
                {
                t1*=(float)timf3_sampling_speed/fft3_size;
                if(mix1_selfreq[0]+t1 <= mix1_lowest_fq ||
                   mix1_selfreq[0]+t1 >= mix1_highest_fq)
// AFC failed, skip operation
                  {
                  corr_afc_count=0;
                  return;
                  }                   
                mix1_selfreq[0]+=t1;
                add_mix1_cursor(0);
                corr_afc_count=0;
                sc[SC_SHOW_CENTER_FQ]++;  
                corrpow_cnt=0;
                return;
                }
              }
            }
          }  
        }
      else
        {
        pa=timf3_px;
        pb=(pa+2*fft3_size)&timf3_mask;
        ib=fft3_size-2;
        for( ia=0; ia<fft3_size/2; ia++)
          {
          ib+=2;
          t1=timf3_float[poffs+pa  ]*fft3_window[2*ia];
          t2=timf3_float[poffs+pa+1]*fft3_window[2*ia];      
          t3=timf3_float[poffs+pb  ]*fft3_window[2*ia+1];
          t4=timf3_float[poffs+pb+1]*fft3_window[2*ia+1];   
          x1=t1-t3;
          fft3_tmp[4*ia  ]=t1+t3;
          x2=t4-t2;
          fft3_tmp[4*ia+1]=t2+t4;
          fft3_tmp[2*ib  ]=fft3_tab[ia].cos*x1+fft3_tab[ia].sin*x2;
          fft3_tmp[2*ib+1]=fft3_tab[ia].sin*x1-fft3_tab[ia].cos*x2;
          r1=timf3_float[poffs+pa+2]*fft3_window[2*ia];
          r2=timf3_float[poffs+pa+3]*fft3_window[2*ia];      
          r3=timf3_float[poffs+pb+2]*fft3_window[2*ia+1];
          r4=timf3_float[poffs+pb+3]*fft3_window[2*ia+1];   
          x1=r1-r3;
          fft3_tmp[4*ia+2]=r1+r3;
          x2=r4-r2;
          fft3_tmp[4*ia+3]=r2+r4;
          pa=(pa+4)&timf3_mask;
          pb=(pb+4)&timf3_mask;
          fft3_tmp[2*ib+2]=fft3_tab[ia].cos*x1+fft3_tab[ia].sin*x2;
          fft3_tmp[2*ib+3]=fft3_tab[ia].sin*x1-fft3_tab[ia].cos*x2;
          } 
#if IA64 == 0 && CPU == CPU_INTEL
      asmbulk_of_dual_dif(fft3_size, fft3_n, 
                                     fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#else
      bulk_of_dual_dif(fft3_size, fft3_n, 
                                     fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#endif
        for(ia=0; ia < fft3_size; ia+=2)
          {
          ib=4*fft3_permute[ia  ];                               
          ic=4*fft3_permute[ia+1];                             
          z[ib  ]=fft3_tmp[4*ia  ]+fft3_tmp[4*ia+4];
          z[ic  ]=fft3_tmp[4*ia  ]-fft3_tmp[4*ia+4];
          z[ib+1]=fft3_tmp[4*ia+1]+fft3_tmp[4*ia+5];
          z[ic+1]=fft3_tmp[4*ia+1]-fft3_tmp[4*ia+5];
          z[ib+2]=fft3_tmp[4*ia+2]+fft3_tmp[4*ia+6];
          z[ic+2]=fft3_tmp[4*ia+2]-fft3_tmp[4*ia+6];
          z[ib+3]=fft3_tmp[4*ia+3]+fft3_tmp[4*ia+7];
          z[ic+3]=fft3_tmp[4*ia+3]-fft3_tmp[4*ia+7];
          }
        }
      }
    }
  }
if(thread_command_flag[THREAD_FFT3] != THRFLAG_ACTIVE ||
                         mix1_selfreq[0] != old_mix1_selfreq)return;
// Now fft3_float contains transforms for all enabled channels.
// In case the main channel is enabled, calculate power spectra
// and rx channel correlations.
// Before computing powers, use the current polarization parameters
// to transform polarization.
if(mix1_selfreq[0] < 0)goto mix0_absent;
#define ZZ 0.00000000000001
z=&fft3[fft3_pa];
fft3_slowsum_cnt++;
if(fft3_slowsum_cnt > bg.fft_avgnum)fft3_slowsum_cnt=bg.fft_avgnum;
if(sw_onechan)
  {
  k=bg_show_pa;
  iw=0;
  i=bg_first_xpoint<<1;
  for(j=0; j<fft3_slowsum_recalc; j++)
    {
    t1=fft3_slowsum[j]-fft3_power[k];
    fft3_power[k]=(z[i]*z[i]+z[i+1]*z[i+1])*fft3_fqwin_inv[i>>1];
    i+=2;
    bg_waterf_sum[iw]+=fft3_power[k];
    iw++;
    t1+=fft3_power[k];
    if(t1<0.0000001)t1=0.0000001;
    fft3_slowsum[j]=t1;
    k++;
    }
  jr=fft3_slowsum_recalc+bg_xpoints/bg.fft_avgnum+1;
  if(jr > bg_xpoints)jr=bg_xpoints;
  for(j=fft3_slowsum_recalc; j<jr; j++)
    {
    fft3_power[k]=(z[i]*z[i]+z[i+1]*z[i+1])*fft3_fqwin_inv[i>>1];
    i+=2;
    bg_waterf_sum[iw]+=fft3_power[k];
    iw++;
    p0=k-bg_show_pa;
    t1=fft3_power[p0];
    for(m=1; m<bg.fft_avgnum; m++)
      {
      p0+=bg_xpoints;
      t1+=fft3_power[p0];
      }
    fft3_slowsum[j]=t1;  
    k++;
    }
  for(j=jr; j<bg_xpoints; j++)
    {
    t1=fft3_slowsum[j]-fft3_power[k];
    fft3_power[k]=(z[i]*z[i]+z[i+1]*z[i+1])*fft3_fqwin_inv[i>>1];
    i+=2;
    bg_waterf_sum[iw]+=fft3_power[k];
    iw++;
    t1+=fft3_power[k];
    if(t1<0.0000001)t1=0.0000001;
    fft3_slowsum[j]=t1;
    k++;
    }
  }
else
  {
  if(fft1_correlation_flag == 2)
    {
    if(fft1corr_reset_flag != corrpow_reset)
      {
      corrpow_reset=fft1corr_reset_flag;
      corrpow_cnt=0;
      } 
    d_z=&d_fft3[fft3_pa];
    if(corrpow_cnt < 100)corrpow_cnt++;
    ia=fft3_size/8;
    if(ia > 100*bg_carr_20db_points)ia=100*bg_carr_20db_points;
    ia=fft3_size/2-ia;
    ib=fft3_size/2-bg_carr_20db_points;
// Compute power spectrum within 25% of the baseband or a factor 100
// larger than the carrier bandwidth whichever smallest while
// excluding points within the carrier filter. Store in fft3_tmp
    j=0;
    for(i=ia; i<ib; i++)
      {
      im=fft3_size-i;
      d_fft3_tmp[2*j  ]=d_z[4*i  ]*d_z[4*i  ]+d_z[4*i+1]*d_z[4*i+1]+
                        d_z[4*i+2]*d_z[4*i+2]+d_z[4*i+3]*d_z[4*i+3];
      d_fft3_tmp[2*j+1]=d_z[4*im  ]*d_z[4*im  ]+d_z[4*im+1]*d_z[4*im+1]+
                        d_z[4*im+2]*d_z[4*im+2]+d_z[4*im+3]*d_z[4*im+3];
      j++;
      }
    ib=j;
    if(corrpow_cnt == 1)
      {
// Move the first data block to fft3_cleansum 
      for(j=0; j<ib; j++)
        {
        fft3_cleansum[2*j  ]=d_fft3_tmp[2*j  ];
        fft3_cleansum[2*j+1]=d_fft3_tmp[2*j+1];
        }
      goto corrpow_x;
      }
    if(corrpow_cnt <= MAX_CORRPOW_CNT)
      {
// Accumulate MAX_CORRPOW_CNT spectra in fft3_cleansum.      
      for(j=0; j<ib; j++)
        {
        fft3_cleansum[2*j  ]+=d_fft3_tmp[2*j  ];
        fft3_cleansum[2*j+1]+=d_fft3_tmp[2*j+1];
        }
      goto corrpow_x;
      }
// We want to inhibit when there has been impulse noise due to
// vibrations etc. Find the noise floor of fft_cleansum by first finding
// the lowest power.
    ds1=BIGDOUBLE;
    ds2=BIGDOUBLE;
    for(j=0; j<ib; j++)
      {
      if(ds1>fft3_cleansum[2*j  ])ds1=fft3_cleansum[2*j  ];
      if(ds2>fft3_cleansum[2*j+1])ds2=fft3_cleansum[2*j+1];
      }
// Compute the average noise floor as the average of all points
// within 15 dB above the lowest level
    ds1*=31.6F/MAX_CORRPOW_CNT;
    ds2*=31.6F/MAX_CORRPOW_CNT;
    ds3=0;
    ds4=0;
    i=0;
    for(j=0; j<ib; j++)
      {
      if(fft3_cleansum[2*j  ] < ds1 && fft3_cleansum[2*j+1] < ds2)
        {
        ds3+=fft3_cleansum[2*j  ];
        ds4+=fft3_cleansum[2*j+1];
        i++;
        }
      } 
    ds3/=i;
    ds4/=i;
// Compare the latest spectrum with the average in fft3_cleansum while
// excluding points (spurs) that are 10 dB or more above the noise
// power
    ds3*=10;
    ds4*=10;
    dt1=0;
    dt2=0;
    dr1=0;
    dr2=0;
    i=0;
    for(j=0; j<ib; j++)
      {
      if(fft3_cleansum[2*j  ]<ds3 && fft3_cleansum[2*j+1]<ds4)
        {
        i++; 
        dr1+=fft3_cleansum[2*j  ]/MAX_CORRPOW_CNT;
        dt1+=d_fft3_tmp[2*j  ]-fft3_cleansum[2*j  ]/MAX_CORRPOW_CNT;
        dr2+=fft3_cleansum[2*j+1]/MAX_CORRPOW_CNT;
        dt2+=d_fft3_tmp[2*j+1]-fft3_cleansum[2*j+1]/MAX_CORRPOW_CNT;
        }
      }
    if(dt1 < 0)
      {
      dt1=1.e-5;
      }
    else
      {    
      dt1/=dr1*i;
      }
    if(dt2 < 0)
      {
      dt2=1.e-5;
      }
    else
      {    
      dt2/=dr2*i;
      }
    sprintf(fft3_level, "fft3 %6.2f",10*log10(1000*(dt1+dt2)));
//fprintf(stderr,"\n%s   %f   %f",fft3_level,dt1, dt2);
// ö When fft3_size is large the entire transform is not much affected
// when a glitch has occured. A better strategy would be to remove spurs
// by zeroing the transform on all spurs besed on cleansum and then
// go back to the time domain and look for peaks there.
    if(dt1 < 0.005 && dt2 < 0.005)
      {
      dt1=(float)(MAX_CORRPOW_CNT)/(float)(MAX_CORRPOW_CNT+1);
      for(j=0; j<ib; j++)
        {
        fft3_cleansum[2*j  ]=dt1*(fft3_cleansum[2*j  ]+d_fft3_tmp[2*j  ]);
        fft3_cleansum[2*j+1]=dt1*(fft3_cleansum[2*j+1]+d_fft3_tmp[2*j+1]);
        }
      }
    else
      {
      sprintf(fft3_skip, "fft3 skip %6.2f",10*log10(1000*fabs(dt1+dt2)));
      while(thread_status_flag[THREAD_MIX2] != THRFLAG_SEM_WAIT)
        {
        lir_sleep(2000);
        if(thread_command_flag[THREAD_FFT3] != THRFLAG_ACTIVE)return;  
        }
// the siganal step size is sg_interleave_points at baseband_sampling_speed
// each step is thus sg_interleave_points/baseband_sampling_speed seconds.
// The current fft3 transform spans fft3_size/timf3_sampling_speed seconds
// skip signal analysis for 1.5 times the fft3 transform time.
      t1=1.5*(float)fft3_size/timf3_sampling_speed;
      t1/=(float)sg_interleave_points/baseband_sampling_speed;
      skip_siganal=ceil(t1);
      }
    }
corrpow_x:;
//fprintf(stderr,"\ncorrpow_cnt %d",corrpow_cnt);  
  if(fft1_correlation_flag == 2)
    {
    sc[SC_SHOW_SIGANAL_INFO ]++;
    fft3_show_time=recent_time;
    sc[SC_SHOW_FFT3]++;
    }
  if(mix1_selfreq[0] != old_mix1_selfreq)return;
  k=bg_show_pa;
  iw=0;
  i=4*bg_first_xpoint;
  jb=2*fft3_slowsum_recalc;
  for(j=0; j<jb; j+=2)
    {
    x1=fft3_slowsum[j  ]-fft3_power[k  ];
    x2=fft3_slowsum[j+1]-fft3_power[k+1];
    t1=pg.c1*z[i  ]+pg.c2*z[i+2]+pg.c3*z[i+3];
    t2=pg.c1*z[i+1]+pg.c2*z[i+3]-pg.c3*z[i+2];
    t3=pg.c1*z[i+2]-pg.c2*z[i  ]+pg.c3*z[i+1];
    t4=pg.c1*z[i+3]-pg.c2*z[i+1]-pg.c3*z[i  ];
    fft3_power[k  ]=(t1*t1+t2*t2)*fft3_fqwin_inv[i>>2];
    fft3_power[k+1]=(t3*t3+t4*t4)*fft3_fqwin_inv[i>>2];
    i+=4;
    bg_waterf_sum[iw]+=fft3_power[k];
    if(bg_twopol !=0 || fft1_correlation_flag >=2)
                                      bg_waterf_sum[iw]+=fft3_power[k+1];
    iw++;
    x1+=fft3_power[k  ];
    x2+=fft3_power[k+1];
    if(x1<0.0000001)x1=0.0000001;
    if(x2<0.0000001)x2=0.0000001;
    fft3_slowsum[j  ]=x1;
    fft3_slowsum[j+1]=x2;
    k+=2;
    }
  jr=fft3_slowsum_recalc+bg_xpoints/bg.fft_avgnum+1;
  if(jr > bg_xpoints)jr=bg_xpoints;
  ja=jb;
  jb=2*jr;
  for(j=ja; j<jb; j+=2)
    {
    t1=pg.c1*z[i  ]+pg.c2*z[i+2]+pg.c3*z[i+3];
    t2=pg.c1*z[i+1]+pg.c2*z[i+3]-pg.c3*z[i+2];
    t3=pg.c1*z[i+2]-pg.c2*z[i  ]+pg.c3*z[i+1];
    t4=pg.c1*z[i+3]-pg.c2*z[i+1]-pg.c3*z[i  ];
    fft3_power[k  ]=(t1*t1+t2*t2)*fft3_fqwin_inv[i>>2];
    fft3_power[k+1]=(t3*t3+t4*t4)*fft3_fqwin_inv[i>>2];
    i+=4;
    bg_waterf_sum[iw]+=fft3_power[k];
    if(bg_twopol !=0 || fft1_correlation_flag >=2)
                                      bg_waterf_sum[iw]+=fft3_power[k+1];
    iw++;
    p0=k-bg_show_pa;
    t1=fft3_power[p0  ];
    t2=fft3_power[p0+1];
    for(m=1; m<bg.fft_avgnum; m++)
      {
      p0+=2*bg_xpoints;
      t1+=fft3_power[p0  ];
      t2+=fft3_power[p0+1];
      }
    fft3_slowsum[j  ]=t1;
    fft3_slowsum[j+1]=t2;
    k+=2;
    }
  ja=jb;
  jb=2*bg_xpoints;
  for(j=ja; j<jb; j+=2)
    {
    x1=fft3_slowsum[j  ]-fft3_power[k  ];
    x2=fft3_slowsum[j+1]-fft3_power[k+1];
    t1=pg.c1*z[i  ]+pg.c2*z[i+2]+pg.c3*z[i+3];
    t2=pg.c1*z[i+1]+pg.c2*z[i+3]-pg.c3*z[i+2];
    t3=pg.c1*z[i+2]-pg.c2*z[i  ]+pg.c3*z[i+1];
    t4=pg.c1*z[i+3]-pg.c2*z[i+1]-pg.c3*z[i  ];
    fft3_power[k  ]=(t1*t1+t2*t2)*fft3_fqwin_inv[i>>2];
    fft3_power[k+1]=(t3*t3+t4*t4)*fft3_fqwin_inv[i>>2];
    i+=4;
    bg_waterf_sum[iw]+=fft3_power[k];
    if(bg_twopol !=0 || fft1_correlation_flag >=2)
                                         bg_waterf_sum[iw]+=fft3_power[k+1];
    iw++;
    x1+=fft3_power[k  ];
    x2+=fft3_power[k+1];
    if(x1<0.0000001)x1=0.0000001;
    if(x2<0.0000001)x2=0.0000001;
    fft3_slowsum[j  ]=x1;
    fft3_slowsum[j+1]=x2;
    k+=2;
    }
  }
if(jr != bg_xpoints)
  {
  fft3_slowsum_recalc=jr;
  }
else
  {  
  fft3_slowsum_recalc=0;
  }
if(thread_command_flag[THREAD_FFT3] != THRFLAG_ACTIVE)return;  
timf3_px=(timf3_px+2*fft3_new_points*ui.rx_rf_channels)&timf3_mask;
bg_show_pa+=ui.rx_rf_channels*bg_xpoints;
if(bg_show_pa >= fft3_show_size)bg_show_pa=0;
bg_avg_counter++;
//  avgnum     refresh interval
//  1-7              1
//  8-23             2
//  24-39            3
//  40-55            4
//  Large      when 12.5% of the data is new.
if(bg_avg_counter > ((bg.fft_avgnum+8)>>4)+1)
  {
  if(bg_filter_points > 5 && 
                     2*bg_filter_points < bg_xpoints)update_squelch();
  bg_avg_counter=0;
  if(recent_time-fft3_show_time > 0.05 || fft1_correlation_flag >= 2)
    {
    fft3_show_time=recent_time;
    sc[SC_SHOW_FFT3]++;
    awake_screen();
    }
  }
bg_waterf_sum_counter++;
if(bg_waterf_sum_counter >= bg.waterfall_avgnum)
  {
  update_bg_waterf();
  }
mix0_absent:;
if(mix1_selfreq[0] != old_mix1_selfreq)return;
fft3_pa=(fft3_pa+fft3_block)&fft3_mask;
}
