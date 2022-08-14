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
#include "sigdef.h"
#include "screendef.h"
#include "seldef.h"
#include "thrdef.h"

extern int wg_old_y1;
extern int wg_old_y2;
extern int wg_old_x1;
extern int wg_old_x2;



void txtest_init(void)
{
int i, j, k;
float t1;
int pnts_per_seg;
txtest_decayfac1=fft1_blocktime;
txtest_decayfac2=1-txtest_decayfac1;
txtest_no_of_segs=(int)((fft1_last_point-fft1_first_point)*
                                                   fft1_hz_per_point/600);
if(txtest_no_of_segs >= screen_width)
  {
  lirerr(1159);
  return;
  }
for(i=0; i<screen_width; i++)
  {
  txtest_ypeak[i]=wg.ybottom-1;
  txtest_ypeak_decay[i]=wg.ybottom-1;
  txtest_yavg[i]=wg.ybottom-1;
  fft1_spectrum[i]=wg.ybottom-1;
  }
mix1_selfreq[0]=2400;
// Set up a rectangular filter with 2400Hz bandwidth
pnts_per_seg=2400/fft1_hz_per_point;
j=(mix1.size-pnts_per_seg)/2;
k=mix1.size/2;
i=0;
for(i=0; i<j; i++)
  {
  mix1_fqwin[i]=0;
  }
for(i=j; i<=(int)mix1.size/2; i++)
  {
  mix1_fqwin[i]=1;
  }  
j=(mix1.size-mix1.new_points)/2;
k=j;
for(i=0; i<(int)mix1.crossover_points; i++)
  {
  mix1.cos2win[i]=mix1.window[k];
  mix1.sin2win[i]=mix1.window[j];
  k--;
  j++;
  }
if(txtest_no_of_segs <=0)txtest_no_of_segs=1;
txtest_pntinc=(fft1_last_point-fft1_first_point)/txtest_no_of_segs;
t1=pnts_per_seg/2+1;
txtest_first_point=fft1_first_point+t1;
txtest_pixinc=txtest_pntinc;
if(wg.xpoints_per_pixel == 0)
  {
  txtest_pixinc=txtest_pntinc*wg.pixels_per_xpoint;
  t1*=wg.pixels_per_xpoint;
  }
else
  {
  txtest_pixinc=(float)(txtest_pntinc)/wg.xpoints_per_pixel;
  t1/=wg.xpoints_per_pixel;
  if(txtest_pixinc < 1)
    {
    lirerr(1160);
    return;
    }
  }  
t1+=wg_first_xpixel;
txtest_spek_no=0;
txtest_spek_p0=0;
while(txtest_first_point-(int)mix1.size/2 < fft1_first_point)
  {
  txtest_first_point+=txtest_pntinc;
  t1+=txtest_pixinc;
  }
txtest_first_xpix=t1+0.5;  
while(txtest_no_of_segs*txtest_pntinc+(int)mix1.size/2+
               txtest_first_point > fft1_last_point) txtest_no_of_segs--;
if(txtest_no_of_segs < 0)
  {
  txtest_no_of_segs=0;
  }
else
  {
  if( txtest_first_xpix+(txtest_no_of_segs-1)*txtest_pixinc >= wg.xright)
    {
    txtest_no_of_segs=0;
    }  
  }
txtest_yfac=wg_yfac_power*wg.spek_avgnum;
if(genparm[FIRST_FFT_SINPOW] != 0)
  {
  t1=0;
  for(i=0; i<fft1_size/2; i++)t1+=fft1_window[i];
  t1/=fft1_size;
  txtest_yfac/=t1*t1;
  }
txtest_peak_redraw=0;
txtest_saturated=(float)BIGFLOAT;
}

void make_txtest_wide_spectra(void)
{
int i, j, k;
float t1;
// Make the average spectrum.
for(i=0; i<txtest_no_of_segs; i++)
  {
  t1=0;
  k=0;
  for(j=0; j<wg.spek_avgnum; j++)
    {
    t1+=txtest_power[k+i];
    k+=txtest_no_of_segs;
    }
  txtest_powersum[i]=t1/wg.spek_avgnum;  
  }
}


void txtest(void)
{
int i, j, k;
int ia, ib;
int mix1_flag;
int local_workload_reset;
int old_fft1_nx, old_fft1_px;
float pwr1, pwr2;
float t1;
restart:;
local_workload_reset=workload_reset_flag;
for(i=0; i<txtest_no_of_segs*wg.spek_avgnum; i++)txtest_power[i]=0;
mix1_flag=0;
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_TXTEST] == THRFLAG_KILL)goto txtest_exit;
    lir_sleep(10000);
    }
  }  
thread_status_flag[THREAD_TXTEST]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_TXTEST] == THRFLAG_ACTIVE)
  {
  if(local_workload_reset!=workload_reset_flag)
    {
    local_workload_reset=workload_reset_flag;
    if(txtest_saturated < 0.1)
      {
      txtest_saturated=BIGFLOAT;
      }
    for(i=0; i<txtest_no_of_segs; i++)
      {
      txtest_peak_power_decay[i]=txtest_peak_power[i];
      txtest_peak_power[i]=0;
      }      
    for(i=wg_first_xpixel; i<wg_last_xpixel; i++)
      {
      txtest_ypeak[i]=wg.ybottom-1;
      }
// Just clear the entire window and make a new graph.
// Quick fix when going from single-threaded to multi-threaded structure. 
    wg_old_x1=wg.xleft;
    wg_old_x2=wg.xright;
    wg_old_y1=wg.ytop;
    wg_old_y2=wg.ybottom;
    make_wide_graph(TRUE);
    memcheck(197,fft1mem,&fft1_handle);
    if(kill_all_flag) goto txtest_exit;
    }
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// mix1_flag signals to us that there is work to do on previous
// transforms so do not wait for a new fft1/fft2 if it is nonzero.
  if(mix1_flag == 0) lir_await_event(EVENT_FFT1_READY);
  if(txtest_no_of_segs < 1)fft1_px=(fft1_px+1)&fft1_mask;
  mix1_flag=0;
// ###########################################################
// Third processing block.
// evaluate peak voltage in 2.4kHz bandwidth.
// ###########################################################  
  i=(fft1_pb-fft1_px+fft1_mask+1)&fft1_mask;
  if(i>2*(int)mix1.size)
    {
    mix1_flag=1;
    old_fft1_nx=fft1_nx;
    old_fft1_px=fft1_px; 
    mix1_point[0]=txtest_first_point;
    for(i=0; i<txtest_no_of_segs; i++)
      {
      fft1_nx=old_fft1_nx;
      fft1_px=old_fft1_px;
      for(j=0; j<2*timf3_block; j++)
        {
        timf3_float[j]=0;     
        }
      timf3_pa=0;  
      fft1_mix1_fixed();
// In case the window is sine squared, we have to divide by it.
// because the standard routine we use does not divide for us.
      ia=(mix1.interleave_points+mix1.crossover_points)/2;
      if(mix1.interleave_points == mix1.new_points)
        {
        ia=mix1.size/4;
        ib=3*mix1.size/4;
        j=mix1.size/2+1;
        k=mix1.size/2-1; 
        while(k >= (int)mix1.size/5)
          {
          timf3_float[2*k]*=mix1.window[k];     
          timf3_float[2*k+1]*=mix1.window[k];     
          timf3_float[2*j]*=mix1.window[k];     
          timf3_float[2*j+1]*=mix1.window[k];     
          j++;
          k--;
          }
        }
      else
        {
        if(mix1.interleave_points == 0)
          {
          ia=0;
          ib=mix1.size;
          }
        else
          {
          ia=0;
          ib=mix1.new_points+1;
          }
        }
      pwr2=0;
      pwr1=0;
      for(j=ia; j<ib; j++)
        {
        t1=timf3_float[2*j  ]*timf3_float[2*j  ]+
           timf3_float[2*j+1]*timf3_float[2*j+1];
        pwr1+=t1;
        if(pwr2 < t1)pwr2=t1;
        }
      pwr1*=txtest_yfac;
      pwr2*=txtest_yfac;
      txtest_power[txtest_spek_p0+i]=pwr1/(ib-ia);
      txtest_peak_power[i]=pwr2;
      txtest_peak_power_decay[i]=txtest_peak_power_decay[i]*txtest_decayfac2+
                                        txtest_peak_power[i]*txtest_decayfac1;
      mix1_point[0]+=txtest_pntinc;
      lir_sched_yield();
      }
    txtest_show_p0=txtest_spek_p0;  
    txtest_spek_no++;
    txtest_spek_p0+=txtest_no_of_segs;
    if(txtest_spek_no >=wg.spek_avgnum)
      {
      txtest_spek_no=0;
      txtest_spek_p0=0;
      }
    make_txtest_wide_spectra();
    }
// Here we do the mouse actions that affect the wide graph.
// Currently only the waterfall memory area may become re-allocated
  if(mouse_task!=-1)
    {
    k=mouse_task&GRAPH_MASK;
    if( k > MAX_WIDEBAND_GRAPHS)
      {
      set_button_states();
      if(mouse_active_flag == 0)
        {
        switch (k)
          {
          case FREQ_GRAPH:
          mouse_on_freq_graph();
          break;
          }
        if(mouse_active_flag == 0)lirerr(18877);
        }
      if(numinput_flag==0)current_mouse_activity();
      if( (numinput_flag&DATA_READY_PARM) != 0)
        {
        par_from_keyboard_routine();
        par_from_keyboard_routine=NULL;
        mouse_active_flag=0;
        numinput_flag=0;
        leftpressed=BUTTON_IDLE;  
        }
      if(mouse_active_flag == 0)
        {
        mouse_task=-1;
        }
      }  
    }
  }
if(thread_command_flag[THREAD_TXTEST]==THRFLAG_IDLE)
  {
// The wideband dsp thread is running and it puts out
// the waterfall graph. Stop it now.
  thread_command_flag[THREAD_WIDEBAND_DSP]=THRFLAG_IDLE;
  while(thread_status_flag[THREAD_WIDEBAND_DSP]!=THRFLAG_IDLE)
    {
    lir_sleep(25000);
    }
  thread_status_flag[THREAD_TXTEST]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_TXTEST] == THRFLAG_IDLE)
    {
    lir_sleep(10000);
    fft1_px = fft1_pb;
    }
  thread_command_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
  lir_sleep(1000);
  wg_old_x1=wg.xleft;
  wg_old_x2=wg.xright;
  wg_old_y1=wg.ytop;
  wg_old_y2=wg.ybottom;
  make_wide_graph(TRUE);
  if(kill_all_flag) goto txtest_exit;
  goto restart;
  }  
txtest_exit:; 
lir_sleep(10000);
thread_status_flag[THREAD_TXTEST]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_TXTEST] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
