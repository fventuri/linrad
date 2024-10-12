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
#include <fcntl.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "hwaredef.h"
#include "thrdef.h"
#include "options.h"
#include "blnkdef.h"
#include "txdef.h"
#include "vernr.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define RG_MIN_WIDTH (10*text_width)
#define RADAR_SCALE_ZERO 1

void make_radar_graph(int clear_old);

void make_radar_cfac(void)
{
rg_cfac=10*rg.gain;
rg_czer=0.1*(rg.zero+RADAR_SCALE_ZERO);
}


void make_radar_timeconstant(void)
{
float t1, t2;
// The time for one transform is fft1_blocktime or fft2_blocktime
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  t1=fft1_blocktime;
  }
else
  {
  t1=fft2_blocktime;
  }
// We want the amplitude to fall by 1/e = 0.368 in rg.time
if(rg.time < 5)
  {
  t2=rg.time;
  }
else
  {
  t2=5;
  }  
radar_redraw_count=1+0.1*t2/(t1*pulse_sep);
radar_decayfac=pow(0.368,pulse_sep*t1/rg.time);
}

void update_radar_average(int ptr)
{
int i, j, m;
int ia, ib, nmask;
float t1;
nmask=fft1_sumsq_bufsize/fft1_size-1;
ia=(ptr-4+nmask+1)&nmask;  
ib=(ptr+4)&nmask;  
t1=0;
i=ia;
while(ia != ib)
  {
  if(t1 < fft1_sumsq[ia*fft1_size+pulse_bin])
    {
    t1=fft1_sumsq[ia*fft1_size+pulse_bin];
    i=ia;
    }
  ia=(ia+1)&nmask;
  }
ia=i;
while(0.01*t1<fft1_sumsq[ia*fft1_size+pulse_bin])ia=(ia+1)&nmask;
ia=((ia-10)*fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;  
m=0;
for(i=0; i<radar_lines; i++)
  {
  for(j=radar_first_bin; j<radar_last_bin; j++)
    {
    radar_average[m]=radar_average[m]*radar_decayfac+fft1_sumsq[ia+j];
    m++;
    }
  ia=(ia+fft1_size)&fft1_sumsq_mask;  
  }  
radar_update_cnt++;;
}

void run_radar(void)
{
int i, j, k, ia, ib, n, na, nx, ptr;
int fftx_flag, nmask;
int pulsefreq_flag;
float t1, ref_ston, tx_noise_floor;
double dt1, disksync_time;
double total_time1;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_RADAR);
#endif
total_time1=current_time();
dt1=total_time1;  
disksync_time=total_time1;
fftx_flag=0;
while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
 {
 if(thread_command_flag[THREAD_RADAR] == 
                                           THRFLAG_KILL)goto wcw_error_exit;
 lir_sleep(10000);
 }
nmask=fft1_sumsq_bufsize/fft1_size-1;
radar_fft1_sumsq_pa=fft1_sumsq_pa;
na=radar_fft1_sumsq_pa/fft1_size;
na=(na+nmask)&nmask;
nx=na;
fft1_nx=fft1_nb;
fft1_px=fft1_pb;
ptr=0;
pulsefreq_flag=0;
ref_ston=BIGFLOAT;
tx_noise_floor=0;
thread_status_flag[THREAD_RADAR]=THRFLAG_ACTIVE;
while(!kill_all_flag && thread_command_flag[THREAD_RADAR] == THRFLAG_ACTIVE)
  {
  dt1=current_time();
  if(diskwrite_flag)
    {
    if(fabs(disksync_time-dt1) > 0.2)
      {
      lir_sync();
      dt1=current_time();
      disksync_time = dt1;
      }
    }
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  thread_status_flag[THREAD_RADAR]=THRFLAG_SEM_WAIT;
  lir_await_event(EVENT_FFT1_READY);
  if(kill_all_flag) goto wcw_error_exit;
  if(thread_command_flag[THREAD_RADAR] != THRFLAG_ACTIVE)goto narend;
  thread_status_flag[THREAD_RADAR]=THRFLAG_ACTIVE;
more_fftx:;
  fftx_flag=0;
  if( genparm[SECOND_FFT_ENABLE] == 0 )
    {
    if( fft1_nb != fft1_nx)
      {
      if(diskread_flag == 2)
        {
// We use a recording for input. Presumably it was recorded
// in radar mode, but we can not be sure.
// Try to find a repetitive pattern under the assumption that
// the operator has some idea what he is looking for.        
// The key parameter is the fft1 bandwidth. We should assume that
// the transmitted pulse length is the same order of magnitude
// as the inverted bandwidth.
//
// During transmit, the noise floor should be significantly lower
// as compared to the noice floor during receive. The transmitted
// pulse may be strong or weak, but it should be well above the
// noise in the transmit period.
//
// Locate the strongest peak and compute its S/N
        k=0;
        t1=0;
        for(i=0; i<fft1_size; i++)
          {
          if(t1 < fft1_sumsq[radar_fft1_sumsq_pa+i])
            {
            t1=fft1_sumsq[radar_fft1_sumsq_pa+i];
            k=i;
            }
          }
        ia=k;
        ib=k;
        while(ia>1 && fft1_sumsq[radar_fft1_sumsq_pa+ia] >
                      fft1_sumsq[radar_fft1_sumsq_pa+ia-1] &&
                      fft1_sumsq[radar_fft1_sumsq_pa+ia] >
                      fft1_sumsq[radar_fft1_sumsq_pa+ia-2])ia--;
        while(ib<fft1_size-2 && fft1_sumsq[radar_fft1_sumsq_pa+ib] >
                                fft1_sumsq[radar_fft1_sumsq_pa+ib+1] &&
                                fft1_sumsq[radar_fft1_sumsq_pa+ib] >
                                fft1_sumsq[radar_fft1_sumsq_pa+ib+2])ib++;
        ib++;
        t1=0;
// Assuming this is our transmit pulse, there should not be any other
// strong signals present. Assume that everything outside the transmitted
// pulse is our noise floor.
        for(i=0; i<ia; i++)t1+=fft1_sumsq[radar_fft1_sumsq_pa+i];
        for(i=ib; i<fft1_size; i++)t1+=fft1_sumsq[radar_fft1_sumsq_pa+i];
        t1/=fft1_size-(ib-ia);
        radar_pulse_noise_floor[na]=t1;
        radar_pulse_ston[na]=fft1_sumsq[radar_fft1_sumsq_pa+k]/t1;
        radar_pulse_bin[na]=k;
        if(pulsefreq_flag == 0)
          {
          ptr=0;
          pulse_bin=0;
          k=(na-nx+nmask+1)&nmask;
          if(k>nmask/2 || k>500)
            {
// We have collected a lot of S/N data so it should be possible to
// locate our transmitted pulses.
// First locate the transform with the largest S/N.
            k=nx;
            t1=0;
            while(k != na)
              {
              if(t1 < radar_pulse_ston[k])t1=radar_pulse_ston[k];
              k=(k+1)&nmask;
              }
// Set a threshold 25 dB below the best S/N and regard everything
// above it as a transmitted pulse.
            ref_ston=t1*0.003;
            tx_noise_floor=0;
            ia=nx;
gt_pulse:;          
            while(ia != na && ref_ston > radar_pulse_ston[ia])ia=(ia+1)&nmask;
            ib=ia;
            while(ib != na && ref_ston < radar_pulse_ston[ib])ib=(ib+1)&nmask;
            if(ib != na)
              {
              k=((ib-ia+nmask+1)&nmask)/2;
              k=(ia+k)&nmask;
// Now, k is the transform at the center of our transmitted pulse.
              i=(k-nx+nmask+1)&nmask;
              if( i < nmask/2) 
                {
                if(i > 32 && pulsefreq_flag == 0)
                  {
                  nx=(k-16+nmask)&nmask;
                  goto skip_begin;
                  }    
                pulsefreq_flag=1;
                radar_pulse_pointer[ptr]=k;
                pulse_bin+=radar_pulse_bin[k];
                tx_noise_floor+=radar_pulse_noise_floor[k];
                ptr++;
                }
              ia=ib;
              goto gt_pulse;
              }
            }
          if(ptr < 10)
            {
            pulsefreq_flag=0;
            goto skip_begin;
            }    
          tx_noise_floor/=ptr;  
          i=nx;
          t1=0;
          while(i != na)
            {
            t1+=radar_pulse_noise_floor[i];
            i=(i+1)&nmask;
            }
          t1/=(na-nx+nmask+1)&nmask;  
          t1=tx_noise_floor/t1;
          nx=(ia+5)&nmask;
          if(t1 > 0.1)
            {
            pulsefreq_flag=0;
            goto skip_begin;
            }
          ib=ptr/2;
          ia=0;
          t1=0;
          for(i=1; i<ptr; i++)
            {
            radar_average[i-1]=radar_pulse_pointer[i]-radar_pulse_pointer[i-1];
            t1+=radar_average[i];
            }
// look for the median value of pulse separations.
          k=ptr-1;
          n=k/2;
          for(i=0; i<n; i++)
            {
            for(j=i; j<k; j++)
              {
              if(radar_average[i]>radar_average[j])
                {
                t1=radar_average[j];
                radar_average[j]=radar_average[i];
                radar_average[i]=t1;
                }
              }
            }  
          pulse_sep=radar_average[n-1];
          pulse_bin=(pulse_bin+ptr/2)/ptr;
          for(i=0; i<radar_bins*radar_maxlines; i++)radar_average[i]=0;
          radar_update_cnt=0;
          radar_lines=pulse_sep+20;
          if(radar_lines>radar_maxlines)radar_lines=radar_maxlines;
          make_radar_timeconstant();
          k=fft1_size;
          if(k>radar_bins)k=radar_bins;
          k/=2;
          radar_first_bin=pulse_bin-k;
          radar_last_bin=pulse_bin+k;
          if(radar_first_bin<0)
            {
            radar_last_bin-=radar_first_bin;
            radar_first_bin=0;
            }
          if(radar_last_bin>fft1_size)
            {
            radar_first_bin+=fft1_size-radar_last_bin;
            radar_last_bin=fft1_size;
            }
          radar_show_bins=radar_last_bin-radar_first_bin;
          ptr-=2;
          for(i=0; i<ptr-1; i++)
            {
            k=radar_pulse_pointer[i];
            if(abs(pulse_bin-radar_pulse_bin[k]) < 2)
              {
              update_radar_average(k);
              }
            }
skip_begin:;
          }  
        else
          {
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
gt_pulse2:;          
          k=(na-nx+nmask+1)&nmask;
          if(k>pulse_sep+40 && k< nmask/2)
            {
            ia=nx;
            while(ia != na && ref_ston > radar_pulse_ston[ia])ia=(ia+1)&nmask;
            ib=ia;
            while(ib != na && ref_ston < radar_pulse_ston[ib])ib=(ib+1)&nmask;
            if(ib != na)
              {
              k=((ib-ia+nmask+1)&nmask)/2;
              k=(ia+k)&nmask;
              i=(k-nx+nmask+1)&nmask;
              if( i < nmask/2) 
                {
                j=radar_pulse_pointer[(ptr+nmask)&nmask];
                if(abs(pulse_bin-radar_pulse_bin[j]) < 2)
                  {
                  update_radar_average(j);
                  }
                ptr=(ptr+1)&nmask;
                radar_pulse_pointer[ptr]=k;
                nx=(k+pulse_sep/2)&nmask;
                goto gt_pulse2;
                }
              else
                lirerr(76121);  
              }
            else
              {
              nx=ia;
              }

            }
          }
        }
      else
        {
// We are actually transmitting so the tx frequency is known.
        }          
      if(radar_update_cnt > radar_redraw_count)
        {
        radar_update_cnt=0;
        sc[SC_RG_REDRAW]++;
        }
      radar_fft1_sumsq_pa=(radar_fft1_sumsq_pa+fft1_size)&fft1_sumsq_mask;
      na=(na+1)&nmask;
      fft1_nx=(fft1_nx+1)&fft1n_mask;  
      fft1_px=(fft1_px+fft1_block)&fft1_mask;  
      if(fft1_nb != fft1_nx)fftx_flag=1;
      }
    }
  else
    {
    if(new_baseb_flag >= 0)
      {
      if(fft2_na != fft2_nx &&
                ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
        {        
        fft2_mix1_fixed();
        }
      if(fft2_na != fft2_nx)fftx_flag=1;
      }  
    else
      {
      timf3_px=timf3_pa;
      timf3_py=timf3_pa;
      fft3_px=fft3_pa;
      baseb_fx=(baseb_pa-2+baseband_size)&baseband_mask;
      baseb_py=baseb_pa;
      }
    }
  if(fftx_flag != 0) 
    {
    lir_sched_yield();
    if(!kill_all_flag)goto more_fftx;
    }
narend:;
  }    
wcw_error_exit:;
thread_status_flag[THREAD_RADAR]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RADAR] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}




void help_on_radar_graph(void)
{
int msg_no;
int event_no;
// Set msg to select a frequency in case it is not button or border
msg_no=13;
for(event_no=0; event_no<MAX_RGBUTT; event_no++)
  {
  if( rgbutt[event_no].x1 <= mouse_x && 
      rgbutt[event_no].x2 >= mouse_x &&      
      rgbutt[event_no].y1 <= mouse_y && 
      rgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case RG_TOP:
      case RG_BOTTOM:
      case RG_LEFT:
      case RG_RIGHT:
      msg_no=100;
      break;

      }
    }  
  }
help_message(msg_no);
}  

void change_rg_time(void)
{
rg.time=numinput_float_data;
if(rg.time < 0.01)rg.time=0.01;
if(rg.time > 999)rg.time=999;
make_radar_timeconstant();
make_modepar_file(GRAPHTYPE_RG);
sc[SC_RG_REDRAW]++;
}
 
void change_rg_zero(void)
{
rg.zero=numinput_float_data;
if(rg.zero < -99)rg.zero=-99;
if(rg.zero > 99)rg.zero=99;
make_radar_cfac();
make_modepar_file(GRAPHTYPE_RG);
sc[SC_RG_REDRAW]++;
}
 void change_rg_gain(void)
{
rg.gain=numinput_float_data;
if(rg.gain < 0.01)rg.gain=0.01;
if(rg.gain > 99)rg.gain=99;
make_radar_cfac();
make_modepar_file(GRAPHTYPE_RG);
sc[SC_RG_REDRAW]++;
}
 
void mouse_continue_radar_graph(void)
{
int j;
switch (mouse_active_flag-1)
  {
  case RG_TOP:
  if(rg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&rg,0);
    rg.ytop=mouse_y;
    j=rg.ybottom-4*text_height;
    if(rg.ytop > j)rg.ytop=j;      
    if(rg_old_y1 > rg.ytop)rg_old_y1=rg.ytop;
    graph_borders((void*)&rg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case RG_BOTTOM:
  if(rg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&rg,0);
    rg.ybottom=mouse_y;
    j=rg.ytop+4*text_height;
    if(rg.ybottom < j)rg.ybottom=j;      
    if(rg_old_y2 < rg.ybottom)rg_old_y2=rg.ybottom;
    graph_borders((void*)&rg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case RG_LEFT:
  if(rg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&rg,0);
    rg.xleft=mouse_x;
    j=rg.xright-RG_MIN_WIDTH;
    if(rg.xleft > j)rg.xleft=j;      
    if(rg_old_x1 > rg.xleft)rg_old_x1=rg.xleft;
    graph_borders((void*)&rg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case RG_RIGHT:
  if(rg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&rg,0);
    rg.xright=mouse_x;
    j=rg.xleft+RG_MIN_WIDTH;
    if(rg.xright < j)rg.xright=j;      
    if(rg_old_x2 < rg.xright)rg_old_x2=rg.xright;
    graph_borders((void*)&rg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case RG_TIME:
  mouse_active_flag=1;
  numinput_xpix=rgbutt[RG_TIME].x1+text_width/2-1;
  numinput_ypix=rgbutt[RG_TIME].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_rg_time;
  return;

  case RG_ZERO:
  mouse_active_flag=1;
  numinput_xpix=rgbutt[RG_ZERO].x1+text_width/2-1;
  numinput_ypix=rgbutt[RG_ZERO].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_rg_zero;
  return;

  case RG_GAIN:
  mouse_active_flag=1;
  numinput_xpix=rgbutt[RG_GAIN].x1+text_width/2-1;
  numinput_ypix=rgbutt[RG_GAIN].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_rg_gain;
  return;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_radar_graph(TRUE);
if(kill_all_flag) return;
}

void mouse_on_radar_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_RGBUTT; event_no++)
  {
  if( rgbutt[event_no].x1 <= mouse_x && 
      rgbutt[event_no].x2 >= mouse_x &&      
      rgbutt[event_no].y1 <= mouse_y && 
      rgbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_radar_graph;
    return;
    }
  }
// Not button or border.
mouse_active_flag=1;
current_mouse_activity=mouse_nothing;
}


void make_radar_graph(int clear_old)
{
char s[80];
int mem1,mem2;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(rg_old_x1,rg_old_x2,rg_old_y1,rg_old_y2);
  lir_fillbox(rg_old_x1,rg_old_y1,rg_old_x2-rg_old_x1+1,
                                                    rg_old_y2-rg_old_y1+1,0);
  }
current_graph_minh=4*text_height;
current_graph_minw=RG_MIN_WIDTH;
check_graph_placement((void*)(&rg));
clear_button(rgbutt, MAX_RGBUTT);
hide_mouse(rg.xleft,rg.xright,rg.ytop,rg.ybottom);  
if(radar_handle != NULL)
  {
  if(kill_all_flag)return;
  radar_handle=chk_free(radar_handle);
  }
init_memalloc(radarmem, MAX_RADAR_ARRAYS);
mem1=fft1_sumsq_bufsize/fft1_size;
radar_bins=rg.xright-rg.xleft-2;
radar_maxlines=rg.ybottom-rg.ytop-2;
mem2=radar_bins*radar_maxlines;
mem(1,&radar_pulse_ston,mem1*sizeof(float),0);
mem(2,&radar_pulse_bin,mem1*sizeof(int),0);
mem(3,&radar_pulse_pointer,mem1*sizeof(int),0);
mem(4,&radar_pulse_noise_floor,mem1*sizeof(float),0);

mem(4,&radar_average,mem2*sizeof(float),0);
radar_totmem=memalloc(&radar_handle,"rad");
if(radar_totmem == 0)
  {
  lirerr(1314);
  return;
  }
scro[radar_graph_scro].no=RADAR_GRAPH;
scro[radar_graph_scro].x1=rg.xleft;
scro[radar_graph_scro].x2=rg.xright;
scro[radar_graph_scro].y1=rg.ytop;
scro[radar_graph_scro].y2=rg.ybottom;
rgbutt[RG_LEFT].x1=rg.xleft;
rgbutt[RG_LEFT].x2=rg.xleft+2;
rgbutt[RG_LEFT].y1=rg.ytop;
rgbutt[RG_LEFT].y2=rg.ybottom;
rgbutt[RG_RIGHT].x1=rg.xright-2;
rgbutt[RG_RIGHT].x2=rg.xright;
rgbutt[RG_RIGHT].y1=rg.ytop;
rgbutt[RG_RIGHT].y2=rg.ybottom;
rgbutt[RG_TOP].x1=rg.xleft;
rgbutt[RG_TOP].x2=rg.xright;
rgbutt[RG_TOP].y1=rg.ytop;
rgbutt[RG_TOP].y2=rg.ytop+2;
rgbutt[RG_BOTTOM].x1=rg.xleft;
rgbutt[RG_BOTTOM].x2=rg.xright;
rgbutt[RG_BOTTOM].y1=rg.ybottom-2;
rgbutt[RG_BOTTOM].y2=rg.ybottom;
// Draw the border lines
graph_borders((void*)&rg,7);
rgbutt[RG_TIME].x1=rg.xleft+2;
rgbutt[RG_TIME].x2=rgbutt[RG_TIME].x1+4.5*text_width;
rgbutt[RG_TIME].y2=rg.ybottom-2;
rgbutt[RG_TIME].y1=rgbutt[RG_TIME].y2-text_height;
sprintf(s,"%f",rg.time);
s[4]=0;
pulse_sep=100;
make_radar_timeconstant();
show_button(&rgbutt[RG_TIME],s);
rgbutt[RG_ZERO].x2=rg.xright-2;
rgbutt[RG_ZERO].x1=rgbutt[RG_ZERO].x2-4.5*text_width;;
rgbutt[RG_ZERO].y2=rg.ybottom-2;
rgbutt[RG_ZERO].y1=rgbutt[RG_ZERO].y2-text_height;
sprintf(s,"%f",rg.zero);
s[4]=0;
show_button(&rgbutt[RG_ZERO],s);
rgbutt[RG_GAIN].x1=rgbutt[RG_ZERO].x1;
rgbutt[RG_GAIN].x2=rgbutt[RG_ZERO].x2;
rgbutt[RG_GAIN].y2=rgbutt[RG_ZERO].y1-2;
rgbutt[RG_GAIN].y1=rgbutt[RG_GAIN].y2-text_height;
sprintf(s,"%f",rg.gain);
s[4]=0;
show_button(&rgbutt[RG_GAIN],s);
rg_flag=1;
rg_old_x1=rg.xleft;
rg_old_x2=rg.xright;
rg_old_y1=rg.ytop;
rg_old_y2=rg.ybottom;
rg_old_time=rg.time;
rg_old_gain=rg.gain;
rg_old_zero=rg.zero;
make_radar_cfac();
make_modepar_file(GRAPHTYPE_RG);
resume_thread(THREAD_SCREEN);
}

void init_radar_graph(void)
{
int errcnt;
errcnt=0;
if (read_modepar_file(GRAPHTYPE_RG) == 0)
  {
rg_default:;  
// Make the default window for the radar graph.
  rg.xleft=0.4*screen_width+2;
  rg.xright=screen_width-1;
  rg.ytop=wg.ybottom+2;
  rg.ybottom=rg.ytop+0.3*screen_height;
  if(rg.ybottom>screen_height-text_height-1)
    {
    rg.ybottom=screen_height-text_height-1;
    rg.ytop=rg.ybottom-6*text_height;
    }
  rg.time=3.;
  rg.gain=1.;
  rg.zero=10;
  }
errcnt++; 
if(errcnt < 2)
  {
  if(rg.xleft < 0 || rg.xleft > screen_last_xpixel)goto rg_default;
  if(rg.xright < 0 || rg.xright > screen_last_xpixel)goto rg_default;
  if(rg.xright-rg.xleft < RG_MIN_WIDTH)goto rg_default;
  if(rg.ytop < 0 || rg.ytop > screen_height-1)goto rg_default;
  if(rg.ybottom < 0 || rg.ybottom > screen_height-1)goto rg_default;
  if(rg.ybottom-rg.ytop < 6*text_height)goto rg_default;
  }
radar_graph_scro=no_of_scro;
make_radar_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

