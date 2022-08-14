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
#include "fft3def.h"
#include "seldef.h"
#include "sigdef.h"
#include "screendef.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

void clear_wide_maxamps(void)
{
int i;
for(i=0; i<2*MAX_RX_CHANNELS; i++)
  {
  fft1_maxamp[i]=0;
  timf2_maxamp[i]=0;
  }
for(i=0; i<MAX_RX_CHANNELS; i++)fft2_maxamp[i]=0;
}

void make_ad_wttim(void)
{
if(diskread_flag >= 2)
  {
  ad_wts=0;
  }
else
  {  
  ad_wts=(timf1p_pa-timf1p_px+timf1_bytes)&timf1_bytemask;
  ad_wts/=snd[RXAD].framesize;
  }
ad_wttim=(float)(ad_wts)/ui.rx_ad_speed;
}

void make_fft2_wttim(void)
{
// ***********************************************
// In case second fft is enabled we have:
// timf2_wtb back transformed fft1 blocks waiting in timf2 and
// fft2_wtb completed fft2 transforms waiting to be back transformed
// fft2_wtb should be small or zero unless AFC is operating on delayed signal.
// Back transformed fft1 transforms waiting in timf2.
timf2_wtb=(float)((timf2_pa-timf2_px+timf2_mask+1)&timf2_mask)
                                                        /timf2_input_block;
timf2_wttim=timf2_wtb*fft1_blocktime;   
// fft2 transforms waiting for back transformation.
fft2_wtb=(fft2_na-fft2_nx+max_fft2n)&fft2n_mask;  
fft2_wttim=fft2_wtb*fft2_blocktime;    
}

void make_timing_info(void)
{
int db_wts;
make_ad_wttim();
total_wttim=ad_wttim;
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
// ***********************************************
// fft1_wtb completed fft1 transforms waiting to be back transformed
// to the time domain.
// This number should be small or zero unless AFC is operating without
// second fft on delayed signal.
  fft1_wtb=(float)((fft1_pa-fft1_px+fft1_mask+1)&fft1_mask)/fft1_block;
  fft1_wttim=fft1_wtb*fft1_blocktime;
  total_wttim+=fft1_wttim;
  if(mix1_selfreq[0] >=0 )
    {
    make_fft2_wttim();
    total_wttim+=timf2_wttim;
    total_wttim+=fft2_wttim;
    }
  else
    {
    timf2_wttim=0;
    fft2_wttim=0;
    }  
  }
else
  {
  if(mix1_selfreq[0] >=0 )
    {
// ***********************************************
// fft1_wtb completed fft1 transforms waiting to be 
// transformed to the baseband by mix1.
    if(genparm[AFC_ENABLE] == 0 || genparm[AFC_LOCK_RANGE] == 0)
      {
      fft1_wtb=(float)((fft1_pa-fft1_px+fft1_mask+1)&fft1_mask)/fft1_block;
      }
    else
      {
      fft1_wtb=(fft1_na-fft1_nx+max_fft1n)&fft1n_mask;
      }
    fft1_wttim=fft1_wtb*fft1_blocktime;
    total_wttim+=fft1_wttim;
    }
  else
    {
    fft1_wttim=0;
    }  
  }  
// ************************************************
// If a frequency is selected for audio output: 
if(mix1_selfreq[0] >=0 )
  {
// Samples waiting in timf3.
  timf3_wts=((timf3_pa-timf3_px+timf3_size)&timf3_mask)/twice_rxchan;
  timf3_wttim=timf3_wts/timf3_sampling_speed;
  total_wttim+=timf3_wttim;
// Blocks waiting as finished transforms. 
  fft3_wtb=(fft3_pa-fft3_px+fft3_totsiz)&fft3_mask;
  fft3_wtb/=fft3_block;
// We get fft3_new_points points from each block
  fft3_wttim=fft3_wtb*fft3_new_points/timf3_sampling_speed;
  total_wttim+=fft3_wttim;
// Samples waiting in the baseband 
  baseb_wts=baseb_pa-baseb_py;
  if(baseb_wts<0)baseb_wts+=baseband_size;
  baseb_wttim=baseb_wts/baseband_sampling_speed;
  total_wttim+=baseb_wttim;
// Data waiting for output 
  db_wts=(daout_pa-daout_px+daout_size)&daout_bufmask;
  db_wts/=(rx_daout_channels*rx_daout_bytes);
  db_wttim=(float)(db_wts)/genparm[DA_OUTPUT_SPEED];
  total_wttim+=db_wttim;
  if( thread_status_flag[THREAD_RX_OUTPUT] == THRFLAG_ACTIVE)
    {
    da_wttim=(float)(snd[RXDA].valid_frames)/genparm[DA_OUTPUT_SPEED];
    }
  else
    {  
    da_wttim=0;
    snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
    }
  total_wttim+=da_wttim;
  }
else
  {
  db_wttim=0;
  da_wttim=0;
  }
}



void show_amp_info(void)
{
char s[80];
int line, i, k, n;
float t1;
k=(screen_last_line+1)*text_height;
n=k-8*text_height;
hide_mouse(0,28*text_width,n,k);
line=screen_last_line;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  sprintf(s,"%6.2f  ",sqrt(fft1_noise_floor));
  lir_text(20,line,s);
  }
line--;
k=5;
for(i=0; i<ui.rx_ad_channels; i++)
  {
  t1=rx_ad_maxamp[i];
  if(t1<1)t1=1;
  t1/=0x8000;
  t1=-20*log10(t1);
  sprintf(s,"%2.2f ",t1);
  lir_text(k,line,s);
  k+=6;
  }
if(rx_mode == MODE_TXTEST)return;
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
  if(swmmx_fft2)
    {
    line--;  
    k=10;
    for(i=0; i<ui.rx_rf_channels; i++)
      {
      t1=fft2_maxamp[i];
      if(t1<1)t1=1;
      t1/=0x8000;
      t1=-20*log10(t1);
      sprintf(s,"%2.2f ",t1);
      lir_text(k,line,s);
      k+=6;
      }
    }  
  if(swmmx_fft1)
    {
    line--;  
    k=10;
    for(i=0; i<ui.rx_rf_channels; i++)
      {
      t1=timf2_maxamp[i];
      if(t1<1)t1=1;
      t1/=0x8000;
      t1=-20*log10(t1);
      sprintf(s,"%2.2f ",t1);
      lir_text(k,line,s);
      k+=6;
      }
    line--;  
    k=10;
    for(i=0; i<ui.rx_rf_channels; i++)
      {
      t1=timf2_maxamp[MAX_RX_CHANNELS+i];
      if(t1<1)t1=1;
      t1/=0x8000;
      t1=-20*log10(t1);
      sprintf(s,"%2.2f ",t1);
      lir_text(k,line,s);
      k+=6;
      }
    line-=2;  
    k=10;
    for(i=0; i<ui.rx_rf_channels; i++)
      {
      t1=fft1_maxamp[2*i];
      if(t1<1)t1=1;
      t1/=0x8000;
      t1=-20*log10(t1);
      sprintf(s,"%2.2f ",t1);
      lir_text(k,line+1,s);
      t1=fft1_maxamp[2*i+1];
      if(t1<1)t1=1;
      t1/=0x8000;
      t1=-20*log10(t1);
      sprintf(s,"%2.2f ",t1);
      lir_text(k,line,s);
      k+=6;
      }
    }
  }
}

void clear_timinfo_lines(char color)
{
int i, k, m, n, col, line;
k=(screen_last_line+1)*text_height;
m=8*text_height;
n=k-m;
hide_mouse(0,26*text_width,n,k);
lir_fillbox(0,n,26*text_width,m,color);
lir_fillbox(0,n-text_height,40*text_width,text_height,color);
col=30;
line=screen_last_line;
for(i=0; i<THREAD_MAX; i++)
  {
  if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
    {
    if(col+11 > screen_last_col)
      {
      col=30;
      line --;
      }
    lir_text(col,line,"          ");
    col+=11;
    }
  }
}

void clear_ampinfo_lines(char color)
{
int k, m, n;
k=(screen_last_line+1)*text_height;
m=8*text_height;
n=k-m;
hide_mouse(0,28*text_width,n,k);
lir_fillbox(0,n,28*text_width,m,color);
}

void amp_info_texts(void)
{
char color;
int line;
if(ampinfo_flag == 0)
  {
  color=0;
  }
else
  {
  color=38;
  }  
if(timinfo_flag)clear_timinfo_lines(0);
clear_ampinfo_lines(color);
timinfo_flag=0;
if(ampinfo_flag==0)return;
line=screen_last_line;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  lir_text(14,line,"Floor");
  }
line--;
lir_text(0,line,"A/D");
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
  if(swmmx_fft2)
    {
    line--;
    lir_text(4,line,"fft2");
    }
  if(swmmx_fft1)
    {
    line--;
    lir_text(0,line,"timf2 Wk");
    line--;
    lir_text(0,line,"timf2 St");
    line--;
    lir_text(2,line,"fft1 Wk");
    line--;
    lir_text(2,line,"fft1 St");
    }
  } 
line--;
lir_text(0,line,"Amplitude margins (dB)");
}


void show_timing_info(void)
{
char s[80];
int k, n, line;
float t1;
k=(screen_last_line+1)*text_height;
n=k-7*text_height;
if(!audio_dump_flag && ui.rx_addev_no != NETWORK_DEVICE_CODE)
  {
  if( measured_ad_speed > ui.rx_ad_speed*1.01 || 
      measured_ad_speed < ui.rx_ad_speed*0.99)
    {
    if( measured_ad_speed > 0)
      {
      sprintf(s,"RX A/D SPEED ERROR: %dHz (nominal %dHz)",
                                    (int)(measured_ad_speed),ui.rx_ad_speed);
      wg_error(s,WGERR_ADSPEED);
      }
    }  
  }
if(!timinfo_flag)return;  
hide_mouse(0,26*text_width,n,k);
make_timing_info();
//
// *******  1  **********
line=screen_last_line;
sprintf(s,"%1.3f ",total_wttim);
lir_text(19,line,s);
//
// *******  2  **********
line--;
sprintf(s,"%1.3f ",ad_wttim);
s[5]=0;
lir_text(6,line,s);
sprintf(s,"%1.3f",fft1_wttim);
s[5]=0;
lir_text(19,line,s);
//
// *******  3  **********
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
  line--;
  sprintf(s,"%1.3f ",timf2_wttim);
  s[5]=0;
  lir_text(6,line,s);
  sprintf(s,"%1.3f ",fft2_wttim);
  lir_text(19,line,s);
  }
//
// *******  4  **********
line--;
sprintf(s,"%1.3f ",timf3_wttim);
s[5]=0;
lir_text(6,line,s);
sprintf(s,"%1.3f ",fft3_wttim);
s[5]=0;
lir_text(19,line,s);
//
// *******  5  **********
line--;
sprintf(s,"%1.3f ",baseb_wttim);
s[5]=0;
lir_text(6,line,s);
// Write buf, how much time the data in daout[] corresponds to
sprintf(s,"%1.3f ",db_wttim);
s[5]=0;
lir_text(19,line,s);
//
// *******  6  **********
line--;
// Write D/A, time of data in output buffer (already written to device)
sprintf(s,"%1.3f ",da_wttim);
s[5]=0;
lir_text(6,line,s);
// Write MIN, the minimum time margin in the output buffer.
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  t1=min_daout_samps/(rx_daout_channels*rx_daout_bytes);
  }
else
  {
  t1=snd[RXDA].min_valid_frames;
  }
if(t1<0)t1=0;	
t1/=genparm[DA_OUTPUT_SPEED];
sprintf(s,"%1.3f ",t1);
s[5]=0;
lir_text(19,line,s);
if(ui.network_flag & NET_RX_OUTPUT)
  {
  sprintf(s,"NetWT  %1.3f",max_netwait_time);
  lir_text(28,line,s);
  sprintf(s,"NetERR %1.3f",net_error_time);
  lir_text(28,line+1,s);
  }
if(measured_da_speed > 0)
  {
  sprintf(s,"D/A:%dHz ",(int)(measured_da_speed));
  lir_text(14,screen_last_line-8,s);
  }
sprintf(s,"A/D:%dHz ",(int)(measured_ad_speed));
lir_text(0,screen_last_line-8,s);
sprintf(s,"DMA:  A/D:%.0f Hz   D/A:%.0f Hz     ",snd[RXAD].interrupt_rate,
                                           snd[RXDA].interrupt_rate);
lir_text(0,screen_last_line-7,s);
}

void timing_info_texts(void)
{
char color;
int line;
if(timinfo_flag == 0)
  {
  color=0;
  }
else
  {
  color=38;
  }  
if(ampinfo_flag)clear_ampinfo_lines(0);
clear_timinfo_lines(color);
ampinfo_flag=0;
if(timinfo_flag == 0)return;
//
// *******  1  **********
line=screen_last_line;
lir_text(14,line,"Tot");
//
// *******  2  **********
line--;
lir_text(2,line,"Raw");
lir_text(13,line,"fft1");
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
//
// *******  3  **********
  line--;
  lir_text(0,line,"timf2");
  lir_text(13,line,"fft2");
  }
//
// *******  4  **********
line--;
lir_text(0,line,"timf3");
lir_text(13,line,"fft3");
//
// *******  5  **********
line--;
lir_text(0,line,"baseb");
lir_text(14,line,"buf");
//
// *******  6  **********
line--;
lir_text(0,line,"D/A");
lir_text(14,line,"MIN");
//
// *******  7  **********
line-=1;
lir_text(6,line,"Delay times");
}

void deb_timing_info(char *txt)
{
#define FCT 1000
if(dmp == NULL)return;
make_timing_info();
DEB"\n%.0f%s|%dAD %.0f ft1 %.0f tf2 %.0f ft2 %.0f",
          FCT*total_wttim,txt,new_baseb_flag,
      FCT*ad_wttim,FCT*fft1_wttim,FCT*timf2_wttim,FCT*fft2_wttim);
DEB" tf3 %.0f ft3 %.0f bas %.0f db %.0f da %.0f",
      FCT*timf3_wttim,FCT*fft3_wttim,FCT*baseb_wttim,
                                        FCT*db_wttim,FCT*da_wttim);
}


