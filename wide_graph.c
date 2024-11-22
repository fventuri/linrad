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


#define YBO 8
#define YWF 4

#include "osnum.h"
#include "globdef.h"
#include <fcntl.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "seldef.h"
#include "graphcal.h"
#include "vernr.h"
#include "sigdef.h"
#include "thrdef.h"
#include "sdrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

typedef struct airspyhf_device airspyhf_device_t;
typedef int (*p_airspyhf_set_optimal_iq_correction_point)
                               (airspyhf_device_t* device, float w);
extern struct airspyhf_device *dev_airspyhf;
extern p_airspyhf_set_optimal_iq_correction_point 
                           airspyhf_set_optimal_iq_correction_point;

int wide_graph_scro;
int wg_old_y1;
int wg_old_y2;
int wg_old_x1;
int wg_old_x2;

void make_wg_fqlimits(void)
{
wg_lowest_freq=0.000001*wg_first_frequency+0.1*frequency_scale_offset;
wg_highest_freq=wg_lowest_freq+0.000001*wg.xpoints*fft1_hz_per_point;
}

float wg_check_mix1(int i0)
{
int i,k;
float t1;
i=mouse_x;
if(i<wg_first_xpixel)i=wg_first_xpixel;
if(i>wg_last_xpixel)i=wg_last_xpixel;
t1=wg_first_frequency+(float)(i-wg_first_xpixel)*wg_hz_per_pixel;
if(t1 <  mix1_lowest_fq)t1=mix1_lowest_fq;
if(t1 > mix1_highest_fq)t1=mix1_highest_fq;
k=0;
for(i=i0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
  {
  if( fabs(t1-mix1_selfreq[i]) < 3*wg_hz_per_pixel)
    {
    new_mix1_curx[i]=-1;
    k++;
    mix1_selfreq[i]=-1;
    mix1_point[i]=-1;
    }
  }
if(k>0 && i0==0)return -BIGFLOAT;
return t1;
}

void wide_graph_selfreq(void)
{
int post;
float t1;
new_mix1_curx[0]=-1;
t1=wg_check_mix1(1);
post=make_new_signal(0, t1);
if(post)
  {
  sc[SC_FREQ_READOUT]++;
  if(genparm[SECOND_FFT_ENABLE] != 0)sc[SC_HG_FQ_SCALE]++;
  if(genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0)
                                                        sc[SC_BG_FQ_SCALE]++;
  }
if(leftpressed != BUTTON_RELEASED)
  {
  if(post)
    {
    awake_screen();
    fg.tune_frequency=mix1_selfreq[0];
    make_modepar_file(GRAPHTYPE_FG);
    }  
  return;
  }
if(post)
  {  
  if(genparm[AFC_ENABLE]!=0 && genparm[AFC_LOCK_RANGE] != 0)
                                                        sc[SC_BG_FQ_SCALE]++;
  awake_screen();
  }
leftpressed=BUTTON_IDLE;  
if(new_baseb_flag == -1)baseb_reset_counter++;
mouse_active_flag=0;
}

void add_mix1_cursor(int num)
{
float t1;
if(mix1_selfreq[num]<wg_first_frequency)
  {
  new_mix1_curx[num]=(int)wg_first_frequency;
  }
else  
  {
  new_mix1_curx[num]=(int)(0.5F+(float)wg_first_xpixel+
                    (mix1_selfreq[num]-wg_first_frequency)/wg_hz_per_pixel);
  if(new_mix1_curx[num] > wg_last_xpixel)
    {
    new_mix1_curx[num]=wg_last_xpixel;
    }
  }
if(ui.rx_addev_no == AIRSPYHF_DEVICE_CODE)
  {
  if(num == 0)
    {    
    if(airspyhf_set_optimal_iq_correction_point)
      {
      t1=mix1_selfreq[0];
      if(fabs(airspyhf_old_point-t1) >2000)
        {
        airspyhf_old_point=t1;
        airspyhf_set_optimal_iq_correction_point(dev_airspyhf, 
                                          (t1-5000.f)/ui.rx_ad_speed-0.5f);
        }
      }
    }
  }  
}


int make_new_signal(int i,float t1)
{
int retcod;
if(mix1_selfreq[i]==t1)
  {
  retcod=FALSE;
  }
else
  {
  retcod=TRUE;
  mix1_selfreq[i]=t1;
  }
mix1_point[i]=-1;
add_mix1_cursor(i);
return retcod;
}

void move_rx_frequency(float step, int mode)
{
float t1;
if(mix1_selfreq[0] < 0 || ag.mode_control != 0)return;
if(genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0)
  {
  t1=mix1_selfreq[0];
  t1+=step;
  if(t1<=mix1_lowest_fq)return;
  if(t1>=mix1_highest_fq)return;
  unconditional_hide_mouse();
  switch (mode)
    {
    case 0:
    mix1_selfreq[0]=t1;
    add_mix1_cursor(0);
    sc[SC_SHOW_CENTER_FQ]++;
    break;
    
    case 1:
    mix1_selfreq[0]=t1;
    add_mix1_cursor(0);
    sc[SC_SHOW_CENTER_FQ]++;
    clear_bfo();
    bg.bfo_freq-=step;
    make_bfo();
    break;
    
    case 2:
    clear_bfo();
    bg.bfo_freq-=step;
    make_bfo();
    break;
    }
  }
}
  
void step_rx_frequency(int direction)
{
int m;
float t1;
m=1<<bg.wheel_stepn;
if(fft1_correlation_flag > 1)
  {
  t1=1.0/1024.;
  }
else
  {
  t1=1.0/256.;
  }  
t1*=(float)m;
move_rx_frequency((float)direction*t1/fftx_points_per_hz,bg.horiz_arrow_mode);
}


void mouse_continue_wide_graph_rightpressed(void)
{
int iymax;
int jw, iy, i, ix,color;
int maxpos, maxval, total;
float t1;
#define MAX_OCCURANCES 3*MAX_COLOR_SCALE
int occurances[MAX_OCCURANCES];
int last_valid;
last_valid=0;
if(rightpressed != BUTTON_RELEASED)return;
switch (mouse_active_flag-1)
  {
  case WG_WATERF_ZERO:
  if(recent_time-wg_waterf_zero_time > 1)
    {
    for(i=0; i<MAX_OCCURANCES; i++)occurances[i]=0;
    wg_waterf_zero_time=current_time();
    iy=wg_waterf_y;
    iymax=wg_waterf_y+wg_waterf_lines-wg_waterf_yinc;
    jw=wg_waterf_ptr2;
    while(iy < iymax)
      {
      i=wg_first_xpixel;
      ix=0;
      while(ix<wg_xpixels && i<screen_width)
        {
        color=(int)(MAX_COLOR_SCALE+wg_waterf_cfac*
                                  ((float)wg_waterf[jw+ix]-(float)wg_waterf_czer));
        ix++;
        if(color<0)
          {
          color=0;
          }
        else
          {  
          if(color >= MAX_OCCURANCES)color=MAX_OCCURANCES-1;
          }
        occurances[color]++;  
        if(color != 0)last_valid=occurances[0];
        i++;
        }
      iy++;
      jw+=wg_xpixels;
      if(jw > wg_waterf_size-wg_xpixels-1)jw=0;
      }  
    if(last_valid == 0)last_valid=1;
    occurances[0]=last_valid;
    maxpos=-1;
    maxval=0;
    total=0;
    for(i=0; i<MAX_OCCURANCES-2; i++)
      {
      if(occurances[i]>maxval)
        {
        maxval=occurances[i];
        maxpos=i;
        }
      total+=occurances[i];  
      }  
    if(maxpos==-1 || maxpos== 3)break;
    if(occurances[MAX_OCCURANCES-1] > 1.5*total)
      {
      maxpos=MAX_COLOR_SCALE-1;
      }
    if(maxpos == 0)
      {
// The waterfall is black.      
     wg.waterfall_db_zero-=0.01F*(float)MAX_COLOR_SCALE/wg_waterf_cfac;
      }
    else
      { 
      if(maxpos == MAX_COLOR_SCALE-1)     
        {
// The waterfall is white.      
        wg.waterfall_db_zero+=0.01F*(float)MAX_COLOR_SCALE/wg_waterf_cfac;
        }
      else
        {
        wg.waterfall_db_zero+=.01F*(float)(maxpos-MAX_COLOR_SCALE-3)/
                                          wg_waterf_cfac;
        }  
      }
    make_modepar_file(GRAPHTYPE_WG);
    sc[SC_WG_WATERF_REDRAW]++;
    }
  break;

  case WG_FREQ_ADJUSTMENT_MODE:
  t1=0.000001F*(float)
               (mouse_x-(wg_last_xpixel+wg_first_xpixel)/2)*wg_hz_per_pixel;
  make_wg_fqlimits();
  wg_lowest_freq+=t1;
  wg_highest_freq+=t1;
  t1=0.1F*(float)frequency_scale_offset;
  if(wg_lowest_freq < t1)
    {
    wg_highest_freq+=t1-wg_lowest_freq;
    wg_lowest_freq+=t1-wg_lowest_freq;;
    }
  t1+=(float)(0.000001F*(float)(fft1_size-1)*(float)fft1_hz_per_point);
  if(wg_highest_freq > t1)
    {
    wg_lowest_freq+=wg_highest_freq-t1;
    wg_highest_freq+=wg_highest_freq-t1;
    }
  wg_freq_adjustment_mode=2;
  wg_old_y1=wg.ytop;
  wg_old_y2=wg.ybottom;
  wg_old_x1=wg.xleft;
  wg_old_x2=wg.xright;
  make_wide_graph(TRUE);
  sc[SC_WG_WATERF_REDRAW]++;
  break;  

  default:
  break;
  }  
rightpressed=BUTTON_IDLE;  
mouse_task=-1;
wg_waterf_zero_time=0;
mouse_active_flag=0;
}

void wide_graph_add_signal(void)
{
float t1;
int i;
int event_no;
// The user made a click on the right mouse button.
for(event_no=0; event_no<MAX_WGBUTT; event_no++)
  {
  if( wgbutt[event_no].x1 <= mouse_x && 
      wgbutt[event_no].x2 >= mouse_x &&      
      wgbutt[event_no].y1 <= mouse_y && 
      wgbutt[event_no].y2 >= mouse_y) 
    {
    wg_old_y1=wg.ytop;
    wg_old_y2=wg.ybottom;
    wg_old_x1=wg.xleft;
    wg_old_x2=wg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_wide_graph_rightpressed;
    return;
    }
  }
// If the mouse is on the frequency scale, shift the 
// range of frequencies that we display.
// Add or remove a receive signal.
// We arrive here when the mouse right button is pressed.
// mix1_selfreq[0] is the frequency we process and send to the loudspeaker.
// The other frequencies are translated from CW to ASCII and placed on
// the screen (hopefully, some time...).
// If we are within +/- 3 pixels from a frequency that is already selected
// we deselect it.  
if(rightpressed==BUTTON_RELEASED)
  {
  t1=wg_check_mix1(0);
  if(t1 <0)
    {
    sc[SC_FREQ_READOUT]++;
    baseb_reset_counter++;
    if(fft1_correlation_flag ==0 &&
               ((unsigned int)ui.network_flag & NET_RX_INPUT) != 0)
      {
      net_send_slaves_freq();
      }
    goto addx; 
    } 
  if(MAX_MIX1 > 1)
    { 
    for(i=1; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
      {
      if(mix1_selfreq[i]<0)
        {
        make_new_signal(i,t1);
        goto addx;
        }
      }  
    }  
addx:;
  if(rightpressed==BUTTON_RELEASED)
    {
    sc[SC_SHOW_FFT1]++;
    rightpressed=BUTTON_IDLE;
    mouse_task=-1;
    }
  }
}

void make_wg_waterf_cfac(void)
{
wg_waterf_cfac=wg.waterfall_db_gain*0.01F;
wg_waterf_czer=100*(wg.waterfall_db_zero+WATERFALL_SCALE_ZERO);
}


void check_wg_fft_avgnum(void)
{
// fft1 average power spectra are used to display the full dynamic
// range spectrum.
// In case fft2 is not enabled they are also used for the waterfall graph
// and for AFC (if enabled)
// Average power spectra are time consuming to calculate so several
// different ways are used depending on the needs.
// wg.fft_avg1num 1 to MAX_FFT1_AVG1
// fft1_sumsq=sum over avg1num spectra.
// wg.spek_avgnum = number of spectra for main spectrum
// wg.spek_avgnum=wg.fft_avg1num*wg_fft_avg2num;
// On entry here wg.fft_avg1num is determined by the user.
// change wg.spek_avgnum if necessary
// calculate the new avg2num
// In case neither AFC nor fft2 is enabled, make sure the averaging
// selected for the waterfall graph is a multiple of wg.fft_avg1num.
if(wg.fft_avg1num<1)wg.fft_avg1num=1;
if(wg.fft_avg1num>MAX_FFT1_AVG1)wg.fft_avg1num=MAX_FFT1_AVG1;
if(wg.spek_avgnum<wg.fft_avg1num)wg.spek_avgnum=wg.fft_avg1num;
wg_fft_avg2num=(wg.spek_avgnum+wg.fft_avg1num/2)/wg.fft_avg1num;
if(wg_fft_avg2num >= max_fft1_sumsq)wg_fft_avg2num=max_fft1_sumsq-1;
wg.spek_avgnum=wg.fft_avg1num*wg_fft_avg2num;
while(wg.spek_avgnum >= 99999)
  {
  wg_fft_avg2num--;
  wg.spek_avgnum=wg.fft_avg1num*wg_fft_avg2num;
  }
if( genparm[SECOND_FFT_ENABLE] == 0 )
  {
  if(wg.waterfall_avgnum<wg.fft_avg1num)wg.waterfall_avgnum=wg.fft_avg1num;
  wg.waterfall_avgnum=(wg.waterfall_avgnum+wg.fft_avg1num/2)/wg.fft_avg1num;
  wg.waterfall_avgnum*=wg.fft_avg1num;
  }
}

void wg_error(char *txt,int line)
{
int i;
i=0;
while(txt[i]!=0)i++;
settextcolor(15);
i=wg.xright/text_width-4-i;
if(i<=wg.xleft/text_width)i=wg.xleft/text_width+1;
lir_text(i,wg.ybottom/text_height-2-line,txt);
settextcolor(7);
}


void decrease_wg_pixels_per_points(void)
{
if(wg_xpixels < 128)return;
if(wg.pixels_per_xpoint != 0)
  {
  wg.pixels_per_xpoint/=2;
  if(wg.pixels_per_xpoint > 0)
    {
    wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
    }
  else
    {
    wg.pixels_per_xpoint=0;
    wg.xpoints_per_pixel=2;
    wg.xpoints=wg_xpixels*2;
    }
  }
else
  {
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    wg.xpoints_per_pixel++;
    wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
    }
  else
    {  
    wg.xpoints_per_pixel*=2;
    wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
    }
  }
}

void help_on_wide_graph(void)
{
int msg_no;
int event_no;
// Set msg to select a frequency in case it is not button or border
msg_no=340;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_WGBUTT; event_no++)
  {
  if( wgbutt[event_no].x1 <= mouse_x && 
      wgbutt[event_no].x2 >= mouse_x &&      
      wgbutt[event_no].y1 <= mouse_y && 
      wgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case WG_TOP:
      case WG_BORDER:
      case WG_BOTTOM:
      case WG_LEFT:
      case WG_RIGHT:
      msg_no=100;
      break;

      case WG_YSCALE_EXPAND:
      msg_no=3;
      break;
     
      case WG_YSCALE_CONTRACT:
      msg_no=4;
      break;
      
      case WG_YZERO_DECREASE:
      msg_no=5;
      break;
            
      case WG_YZERO_INCREASE:
      msg_no=6;
      break;

      case WG_FQMIN_DECREASE:
      msg_no=7;
      break;

      case WG_FQMIN_INCREASE:
      msg_no=8;
      break;

      case WG_FQMAX_DECREASE:
      msg_no=9;
      break;

      case WG_FQMAX_INCREASE:
      msg_no=10;
      break;

      case WG_SPUR_TOGGLE:
      msg_no=316;
      break;

      case WG_AVG1NUM:
      if(genparm[SECOND_FFT_ENABLE]==0)
        {
        msg_no=11;
        }
      else
        {
        msg_no=12;
        }  
      break;

      case WG_FFT1_AVGNUM:
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=58;
        }
      else
        {
        msg_no=59;
        }
      break;

      case WG_WATERF_AVGNUM:
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=60;
        }
      else
        {
        msg_no=61;
        }
      break;

      case WG_WATERF_ZERO:
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=64;
        }
      else
        {
        msg_no=65;
        }
      break;

      case WG_WATERF_GAIN:
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=66;
        }
      else
        {
        msg_no=67;
        }
      break;
      
      case WG_FREQ_ADJUSTMENT_MODE:
      msg_no=323;
      break;

      case WG_LOWEST_FREQ:
      msg_no=324;
      break;

      case WG_HIGHEST_FREQ:
      msg_no=325;
      break;      
      }
    }
  }  
help_message(msg_no);
}

void change_fft1avgnum(void)
{
wg.spek_avgnum=numinput_int_data;
check_wg_fft_avgnum();
change_fft1_flag=TRUE;
make_wg_yfac();
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_BUTTONS]++;
}

void change_waterfall_avgnum(void)
{
wg.waterfall_avgnum=numinput_int_data;
if(wg.waterfall_avgnum < 1)
  {
  wg.waterfall_avgnum=1;
  }  
if(genparm[SECOND_FFT_ENABLE] == 0) check_wg_fft_avgnum();
make_wg_yfac();
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_BUTTONS]++;
}

void change_wg_waterfall_zero(void)
{
wg.waterfall_db_zero=numinput_float_data;
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_WATERF_REDRAW]++;
}

void change_wg_waterfall_gain(void)
{
wg.waterfall_db_gain=numinput_float_data;
if(wg.waterfall_db_gain <0.01F)wg.waterfall_db_gain=0.01F;
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_WATERF_REDRAW]++;
}


void change_wg_lowest_freq(void)
{
float t1, t2;
t1=(float)(0.1*frequency_scale_offset);
t2=t1+(float)(0.000001*(fft1_size-4)*fft1_hz_per_point);
wg_lowest_freq=numinput_float_data;
if(wg_lowest_freq > t2)wg_lowest_freq=t2;
if(wg_lowest_freq < t1)wg_lowest_freq=t1;
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_WATERF_REDRAW]++;
}

void change_wg_highest_freq(void)
{
float t1, t2;
t1=(float)(0.1*frequency_scale_offset+0.000004*fft1_hz_per_point);
t2=t1+0.000001F*(float)(fft1_size-4)*fft1_hz_per_point;
wg_highest_freq=numinput_float_data;
if(wg_highest_freq > t2)wg_highest_freq=t2;
if(wg_highest_freq < t1)wg_highest_freq=t1;
make_modepar_file(GRAPHTYPE_WG);
sc[SC_WG_WATERF_REDRAW]++;
}

void mouse_continue_wide_graph(void)
{
int j, old_xpoints;
switch (mouse_active_flag-1)
  {
  case WG_TOP:
  if(wg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&wg,0);
    wg.ytop=mouse_y;
    j=wg.yborder-text_height-YBO;
    if(wg.ytop > j)wg.ytop=j;      
    if(wg_old_y1 > wg.ytop)wg_old_y1=wg.ytop;
    dual_graph_borders((void*)&wg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case WG_BORDER:
  if(wg.yborder!=mouse_y)
    { 
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&wg,0);
    wg.yborder=mouse_y; 
    j=wg.ytop+text_height+YBO;
    if(wg.yborder < j)wg.yborder=j;      
    j=wg.ybottom-2*text_height;
    if(wg.yborder > j)wg.yborder=j;      
    dual_graph_borders((void*)&wg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case WG_BOTTOM:
  if(wg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&wg,0);
    wg.ybottom=mouse_y;
    j=wg.yborder+2*text_height;
    if(wg.ybottom < j)wg.ybottom=j;      
    if(wg_old_y2 < wg.ybottom)wg_old_y2=wg.ybottom;
    dual_graph_borders((void*)&wg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case WG_LEFT:
  if(wg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&wg,0);
    wg.xleft=mouse_x;
    j=wg.xright-32-6*text_width;
    if(wg.xleft > j)wg.xleft=j;      
    if(wg_old_x1 > wg.xleft)wg_old_x1=wg.xleft;
    dual_graph_borders((void*)&wg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case WG_RIGHT:
  if(wg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&wg,0);
    wg.xright=mouse_x;
    j=wg.xleft+32+6*text_width;
    if(wg.xright < j)wg.xright=j;      
    if(wg_old_x2 < wg.xright)wg_old_x2=wg.xright;
    dual_graph_borders((void*)&wg,15);
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
  case WG_YSCALE_EXPAND:
  wg.yrange=wg.yrange/(float)pow(wg.yrange,.2);
  break;
     
  case WG_YSCALE_CONTRACT:
  wg.yrange=wg.yrange*(float)pow(wg.yrange,.18);
  break;
      
  case WG_YZERO_DECREASE:
  wg.yzero=wg.yzero/(float)pow(wg.yrange,.2);
  break;
            
  case WG_YZERO_INCREASE:
  wg.yzero=(float)(wg.yzero*pow(wg.yrange,.18));
  break;

  case WG_FQMIN_DECREASE:
  old_xpoints=wg.xpoints;
  decrease_wg_pixels_per_points();
  wg.first_xpoint-=wg.xpoints-old_xpoints;
  break;

  case WG_FQMIN_INCREASE:
  old_xpoints=wg.xpoints;
  increase_wg_pixels_per_points();
  wg.first_xpoint-=wg.xpoints-old_xpoints;
  break;

  case WG_FQMAX_DECREASE:
  old_xpoints=wg.xpoints;
  increase_wg_pixels_per_points();
  break;

  case WG_FQMAX_INCREASE:
  old_xpoints=wg.xpoints;
  decrease_wg_pixels_per_points();
  break;

  case WG_AVG1NUM:
  wg.fft_avg1num++;
  if(wg.fft_avg1num > MAX_FFT1_AVG1)wg.fft_avg1num=1;
  check_wg_fft_avgnum();
  sc[SC_WG_BUTTONS]++;
  break;

  case WG_FFT1_AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_FFT1_AVGNUM].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_FFT1_AVGNUM].y1+2;
  numinput_chars=5;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_fft1avgnum;
  return;

  case WG_WATERF_AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_WATERF_AVGNUM].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_WATERF_AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_waterfall_avgnum;
  return;

  case WG_WATERF_ZERO:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_WATERF_ZERO].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_WATERF_ZERO].y1+2;
  numinput_chars=5;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_wg_waterfall_zero;
  return;

  case WG_WATERF_GAIN:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_WATERF_GAIN].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_WATERF_GAIN].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_wg_waterfall_gain;
  return;
  
  case WG_SPUR_TOGGLE:
  wg.spur_inhibit^=1;
  wg.spur_inhibit&=1;
  if(wg.spur_inhibit == 1)spurcancel_flag++;
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  make_modepar_file(GRAPHTYPE_WG);
  sc[SC_WG_WATERF_REDRAW]++;
  return;

  case WG_FREQ_ADJUSTMENT_MODE:
  wg_freq_adjustment_mode++;
  if(wg_freq_adjustment_mode < 2)
    {
    make_wg_fqlimits();
    }
  break;

  case WG_LOWEST_FREQ:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_LOWEST_FREQ].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_LOWEST_FREQ].y1+2;
  numinput_chars=(wgbutt[WG_LOWEST_FREQ].x2 -
                                   wgbutt[WG_LOWEST_FREQ].x1)/text_width-1;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_wg_lowest_freq;
  return;
  
  case WG_HIGHEST_FREQ:
  mouse_active_flag=1;
  numinput_xpix=wgbutt[WG_HIGHEST_FREQ].x1+text_width/2-1;
  numinput_ypix=wgbutt[WG_HIGHEST_FREQ].y1+2;
  numinput_chars=(wgbutt[WG_HIGHEST_FREQ].x2 -
                                   wgbutt[WG_HIGHEST_FREQ].x1)/text_width-1;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_wg_highest_freq;
  return;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_wide_graph(TRUE);
}

void mouse_on_wide_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_WGBUTT; event_no++)
  {
  if( wgbutt[event_no].x1 <= mouse_x && 
      wgbutt[event_no].x2 >= mouse_x &&      
      wgbutt[event_no].y1 <= mouse_y && 
      wgbutt[event_no].y2 >= mouse_y) 
    {
    wg_old_y1=wg.ytop;
    wg_old_y2=wg.ybottom;
    wg_old_x1=wg.xleft;
    wg_old_x2=wg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_wide_graph;
    return;
    }
  }
if(rx_mode != MODE_TXTEST && rx_mode != MODE_RADAR)
  {
// Not button or border.
// Select a frequency.
  current_mouse_activity=wide_graph_selfreq;
  }
else
  {
  current_mouse_activity=mouse_nothing;
  }
mouse_active_flag=1;
}

void make_wg_yfac(void)
{
float t1;
int i;
wg_yfac_power=1/(4*(float)wg.spek_avgnum*wg.yzero*wg.yzero);
wg_corr_yfac_power=1/(4*(float)wg.spek_avgnum*wg.yzero*wg.yzero);
wg_corrtot_yfac_power=1/(4*(float)wg.yzero*wg.yzero);
// If waterfall is generated from fft1 we set the zero point here.
t1=(float)FFT1_WATERFALL_ZERO/(float)wg.waterfall_avgnum;
if(wg.xpoints_per_pixel > 1)
  {
  wg_yfac_power/=(float)wg.xpoints_per_pixel*(float)wg.xpoints_per_pixel;
  wg_corr_yfac_power/=(float)wg.xpoints_per_pixel;
  wg_corrtot_yfac_power/=(float)wg.xpoints_per_pixel;
  t1*=(float)ui.rx_rf_channels;      
  }
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
// The constant here sets the zero point for the waterfall graph
// generated by fft2.
  t1=(float)(FFT2_WATERFALL_ZERO)/((float)fft2_size*(float)fft1_size);
  t1/=(float)sqrt((float)(wg.waterfall_avgnum));
  if(fft_cntrl[FFT2_CURMODE].mmx != 0)
    {
    t1*=(float)(2<<(2*(genparm[SECOND_FFT_ATT_N])));
    }
  t1*=(float)(1<<(2*genparm[FIRST_BCKFFT_ATT_N]));
  t1*=(float)(1 + 1/(0.5+genparm[FIRST_FFT_SINPOW]));
  t1*=(float)(ui.rx_rf_channels*ui.rx_rf_channels);      
  }
// Straighten out the waterfall graph until the -10dB point is reached
for(i=0; i<fft1_size; i++)
  {
  if(fft1_desired[i] > 0.3162278)
    {
    wg_waterf_yfac[i]=t1/(float)pow(fft1_desired[i],2.0);
    }
  else
    {
    wg_waterf_yfac[i]=t1*10;
    }
  }
wg_waterf_yfac[0]=t1;
wg_waterf_yfac[fft1_size-1]=t1;
wg_yfac_log=10/wg_db_per_pixel;
}

void new_fft1_averages(int ptr, int ia, int ib)
{
int i,m,p0;
latest_wg_spectrum++;
change_fft1_flag=FALSE;
if(lir_status==LIR_POWTIM)return;
// Make fft1_slowsum from scratch over the specified interval
// We have power spectra in fft1_sumsq and we want to take
// the sum over the latest wg.fft_avg2num of them.
if(ia<0)lirerr(521233);
if(ib<ia)lirerr(521234);
if(ib>=fft1_size)lirerr(521235);
p0=(ptr-(wg_fft_avg2num-1)*fft1_size+fft1_sumsq_bufsize)
                                                          &fft1_sumsq_mask;
for(i=ia; i<=ib; i++)
  {
  fft1_slowsum[i]=fft1_sumsq[p0+i];
  }
p0=(p0+fft1_size)&fft1_sumsq_mask;   
for(m=1; m<wg_fft_avg2num; m++)
  {
  for(i=ia; i<=ib; i++)
    {
    fft1_slowsum[i]+=fft1_sumsq[p0+i];
    if(fft1_slowsum[i]<FFT1_SMALL)fft1_slowsum[i]=FFT1_SMALL;
    }
  p0=(p0+fft1_size)&fft1_sumsq_mask;   
  }
if(fft1_correlation_flag == 1)
  {
  p0=(ptr-(wg_fft_avg2num-1)*fft1_size+fft1_sumsq_bufsize)
                                                          &fft1_sumsq_mask;
  for(i=ia; i<=ib; i++)
    {
    fft1_slowcorr[2*i  ]=fft1_corrsum[2*(p0+i)  ];
    fft1_slowcorr[2*i+1]=fft1_corrsum[2*(p0+i)+1];
    } 
  p0=(p0+fft1_size)&fft1_sumsq_mask;   
  for(m=1; m<wg_fft_avg2num; m++)
    {
    for(i=ia; i<=ib; i++)
      {
      fft1_slowcorr[2*i  ]+=fft1_corrsum[2*(p0+i)  ];
      fft1_slowcorr[2*i+1]+=fft1_corrsum[2*(p0+i)+1];
      }
    p0=(p0+fft1_size)&fft1_sumsq_mask;   
    }
  }
}

void increase_wg_pixels_per_points(void)
{
if(wg.pixels_per_xpoint != 0)
  {
  wg.pixels_per_xpoint*=2;
  wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
  wg.xpoints_per_pixel=0;
  }
else
  {
  if(wg.xpoints_per_pixel < 30)
    {
    wg.xpoints_per_pixel --;
    }
  else
    {
    wg.xpoints_per_pixel*=.75F;
    }
  if(wg.xpoints_per_pixel == 0)wg.xpoints_per_pixel=1;
  wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
  if(wg.xpoints_per_pixel == 1)
    {
    wg.pixels_per_xpoint=1;
    wg.xpoints=wg_xpixels;
    }  
  else
    {
    wg.pixels_per_xpoint=0;
    }  
  }
}

void make_wide_graph(int clear_old)
{
char s[80];
int i, j, k;
int old_xpix;
int hgwat_xpoints;
float t2;
int ypixels;
float t1;
double db_scalestep, scale_value;
float scale_y;
int scale_decimals;
float scale_rnd;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(wg_old_x1,wg_old_x2,wg_old_y1,wg_old_y2);
  lir_fillbox(wg_old_x1,wg_old_y1,wg_old_x2-wg_old_x1+1,
                                                    wg_old_y2-wg_old_y1+1,0);
  }
// If there is another window open, make sure we stay away from it
current_graph_minh=YBO+3*text_height;
current_graph_minw=32+6*text_width;
check_graph_placement(&wg);
clear_button(wgbutt, MAX_WGBUTT);
hide_mouse(wg.xleft,wg.xright,wg.ytop,wg.ybottom);  
if(wg.ybottom-wg.ytop<3*text_height+YBO)
  {
  wg.yborder=(wg.ybottom+wg.ytop)/2;
  }
else
  { 
  if(wg.yborder < wg.ytop+text_height+YBO)wg.yborder=wg.ytop+text_height+YBO;
  if(wg.yborder > wg.ybottom-2*text_height)wg.yborder=wg.ybottom-2*text_height;
  }
wg_waterf_lines=wg.yborder-wg.ytop-text_height-YWF;
wg_waterf_y1=wg.ytop+text_height+YWF+1;
wg_waterf_y2=wg_waterf_y1+2+wg_waterf_lines/16;
if(wg_waterf_y2 > wg_waterf_y1+wg_waterf_lines-1)
                           wg_waterf_y2=wg_waterf_y1+wg_waterf_lines-1;
wg_waterf_y=wg_waterf_y2;
if(wg_waterf_y2 > wg.yborder-1)wg_waterf_y2=wg.yborder-1;
wg_waterf_yinc=wg_waterf_y2-wg_waterf_y1+1;
if(wg_freq_adjustment_mode >= 2)
  {
  wg_freq_adjustment_mode=0;
// Find out whether we can expand our window.  
  k=wg.xright-wg.xleft;
  i=0;
  while(i!=k)
    {
    i=k;
    wg.xleft=wg.xleft-k/2;
    wg.xright=wg.xright+k/2;
    if(wg.xleft<0)wg.xleft=0;
    if(wg.xright >= screen_width)wg.xright=screen_width-1;
    check_graph_placement(&wg);
    k=wg.xright-wg.xleft;
    }
  wg.xpoints=(int)(1000000*(wg_highest_freq-wg_lowest_freq)/fft1_hz_per_point);
  if(wg.xpoints < 4)wg.xpoints=4;
  if(wg.xpoints > fft1_size)wg.xpoints=fft1_size;
  wg_first_frequency=(float)(1000000*wg_lowest_freq
                               -100000*frequency_scale_offset);
  wg.first_xpoint=(int)(wg_first_frequency/fft1_hz_per_point);
  wg_xpixels=wg.xright-wg.xleft-6*text_width+1;
  wg.pixels_per_xpoint=wg_xpixels/wg.xpoints;
  if(wg.pixels_per_xpoint > 0)
    {
    if(wg.pixels_per_xpoint == 1)
      {
      wg.xpoints_per_pixel=1;
      }
    else
      {
      wg.xpoints_per_pixel=0;
      }
    wg_xpixels=wg.xpoints*wg.pixels_per_xpoint; 
    }
  else
    {
    wg.xpoints_per_pixel=wg.xpoints/wg_xpixels;
    wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
    }
  wg.xright=wg_xpixels+wg.xleft+6*text_width-1;
  }
if(wg.first_xpoint<0)wg.first_xpoint=0;
// Check if the desired spectrum fits within the frame and
// make changes as required if not.
wg_first_xpixel=wg.xleft+4*text_width;
wg_last_xpixel=wg.xright-2*text_width;
wg_xpixels = wg_last_xpixel-wg_first_xpixel+1;
old_xpix=wg_xpixels;
if(wg.pixels_per_xpoint != 0)
  {
  wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
  }
else
  {
  wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
  }
if(wg.xpoints < 4)
  {
  wg.xpoints=4;
  wg.pixels_per_xpoint=wg_xpixels/wg.xpoints;
  wg.xpoints_per_pixel=0;
  } 
if(rx_mode == MODE_TXTEST)
  {
  if(wg.xpoints_per_pixel != 0)
    {
    if( fft1_hz_per_point*wg.xpoints_per_pixel > 600)
      {
      wg.xpoints_per_pixel=595/fft1_hz_per_point;
      wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
      }
    }  
  else
    {
    if(wg.pixels_per_xpoint*fft1_hz_per_point > 400)
      {
      wg.pixels_per_xpoint= 400/fft1_hz_per_point;
      wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
      wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
      }
    }  
  txtest_no_of_segs=wg.xpoints*fft1_hz_per_point/600;
  if(txtest_no_of_segs > screen_width-4*text_width)
    {
    wg.xpoints=600*(screen_width-4*text_width)/fft1_hz_per_point;
    if(wg.pixels_per_xpoint != 0)
      {
      wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
      }
    else
      {
      wg_xpixels=wg.xpoints/wg.xpoints_per_pixel;
      wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
      }
    }
  if(wg_xpixels > wg.xright-wg.xleft-6*text_width+1)
    {
    wg_xpixels=wg.xright-wg.xleft-6*text_width+1;
    if(wg.pixels_per_xpoint != 0)
      {
      wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
      wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
      }
    else
      {
      wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
      }
    }
  }
if(wg.xpoints>fft1_size)
  {
  wg.xpoints=fft1_size;
  if(wg.pixels_per_xpoint != 0)
    {
    wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
    }
  else
    {
    wg_xpixels=wg.xpoints/wg.xpoints_per_pixel;
    }
  }  
if(wg.first_xpoint+wg.xpoints>=fft1_size)wg.first_xpoint=fft1_size-wg.xpoints;
// Now that we know that fft1 fits in the window, set the number of
// pixels per data point.
if(genparm[SECOND_FFT_ENABLE] != 0)
  { 
  hgwat_xpoints=wg.xpoints*fft2_to_fft1_ratio;
  if(hgwat_xpoints > wg_xpixels)
    {
    hgwat_xpoints_per_pixel=hgwat_xpoints/wg_xpixels;
    hgwat_pixels_per_xpoint=0;
    wg.xpoints_per_pixel=hgwat_xpoints_per_pixel/fft2_to_fft1_ratio;
    if(wg.xpoints_per_pixel > 0)
      {
      hgwat_xpoints_per_pixel=wg.xpoints_per_pixel*fft2_to_fft1_ratio;
      wg_xpixels=hgwat_xpoints/hgwat_xpoints_per_pixel;
      wg.xpoints=wg_xpixels*wg.xpoints_per_pixel;
      if(wg.xpoints_per_pixel == 1)
        {
        wg.pixels_per_xpoint=1;
        }
      else
        {  
        wg.pixels_per_xpoint=0;
        }
      wg_hz_per_pixel=fft1_hz_per_point*(float)wg.xpoints_per_pixel;
      }
    else
      {
      wg.pixels_per_xpoint=fft2_to_fft1_ratio/hgwat_xpoints_per_pixel;
      wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
      while(wg_xpixels > old_xpix)
        {
        wg.xpoints--;
        wg_xpixels-=wg.pixels_per_xpoint;
        }
      wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
      wg_hz_per_pixel=fft1_hz_per_point/(float)wg.pixels_per_xpoint;
      make_power_of_two(&wg.pixels_per_xpoint);
      hgwat_xpoints_per_pixel=fft2_to_fft1_ratio/wg.pixels_per_xpoint;
      }  
    }
  else
    {
    hgwat_xpoints_per_pixel=0;
    hgwat_pixels_per_xpoint=wg_xpixels/hgwat_xpoints;
    wg.pixels_per_xpoint=fft2_to_fft1_ratio*hgwat_pixels_per_xpoint;
    wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
    wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
    wg_hz_per_pixel=fft1_hz_per_point/(float)wg.pixels_per_xpoint;
    if(wg.pixels_per_xpoint == 1)
      {
      wg.xpoints_per_pixel=1;
      }
    else
      {
      wg.xpoints_per_pixel=0;
      }
    }
  hgwat_first_xpoint=wg.first_xpoint*fft2_to_fft1_ratio;
  }
else
  {
  if(wg.pixels_per_xpoint != 0)
    {
    wg_xpixels=wg.xpoints*wg.pixels_per_xpoint;
    wg_hz_per_pixel=fft1_hz_per_point/(float)wg.pixels_per_xpoint;
    }
  else
    {
    wg_xpixels=wg.xpoints/wg.xpoints_per_pixel;
    wg_hz_per_pixel=fft1_hz_per_point*(float)wg.xpoints_per_pixel;
    }  
  }
wg_last_xpixel=wg_first_xpixel+wg_xpixels-1;
if(genparm[WG_WATERF_BLANKED_PERCENT] == 0)
  {
  wg_waterfall_blank_points=0;
  }
else
  {
  wg_waterfall_blank_points=1+(genparm[WG_WATERF_BLANKED_PERCENT]*wg_xpixels)/100;
  }
wg.xright=wg_last_xpixel+2*text_width;
wg_first_frequency=fft1_hz_per_point*(float)wg.first_xpoint;
set_fft1_endpoints();
mix1_lowest_fq=(float)(fft1_first_point+1)*fft1_hz_per_point;
t2=(float)wg.first_xpoint*fft1_hz_per_point;
if(mix1_lowest_fq<t2) mix1_lowest_fq=t2;
mix1_highest_fq=(float)(fft1_last_point-1)*fft1_hz_per_point;
t2=(float)((float)wg_last_point*fft1_hz_per_point);
if(mix1_highest_fq > t2)mix1_highest_fq=t2;
if(wg.ybottom >= screen_height)wg.ybottom=screen_height-1;
scro[wide_graph_scro].no=WIDE_GRAPH;
scro[wide_graph_scro].x1=wg.xleft;
scro[wide_graph_scro].x2=wg.xright;
scro[wide_graph_scro].y1=wg.ytop;
scro[wide_graph_scro].y2=wg.ybottom;
wgbutt[WG_LEFT].x1=wg.xleft;
wgbutt[WG_LEFT].x2=wg.xleft+2;
wgbutt[WG_LEFT].y1=wg.ytop;
wgbutt[WG_LEFT].y2=wg.ybottom;
wgbutt[WG_RIGHT].x1=wg.xright-2;
wgbutt[WG_RIGHT].x2=wg.xright;
wgbutt[WG_RIGHT].y1=wg.ytop;
wgbutt[WG_RIGHT].y2=wg.ybottom;
wgbutt[WG_TOP].x1=wg.xleft;
wgbutt[WG_TOP].x2=wg.xright;
wgbutt[WG_TOP].y1=wg.ytop;
wgbutt[WG_TOP].y2=wg.ytop+2;
wgbutt[WG_BORDER].x1=wg.xleft;
wgbutt[WG_BORDER].x2=wg.xright;
wgbutt[WG_BORDER].y1=wg.yborder-1;
wgbutt[WG_BORDER].y2=wg.yborder+1;
wgbutt[WG_BOTTOM].x1=wg.xleft;
wgbutt[WG_BOTTOM].x2=wg.xright;
wgbutt[WG_BOTTOM].y1=wg.ybottom-2;
wgbutt[WG_BOTTOM].y2=wg.ybottom;
// Draw the border lines
dual_graph_borders((void*)&wg,7);
// Free the waterfall buffer in case there is already one allocated
// Then allocate the number of bytes we actually need for the current screen.
wg_waterf=chk_free(wg_waterf);
// ************************************
wg_waterf_size=wg_xpixels*wg_waterf_lines;
max_wg_waterf_times=2+(int)(0.5F*(float)wg_waterf_lines/(float)text_height);
wg_waterf_ptr=0;
wg_waterf=malloc((unsigned int)max_wg_waterf_times*sizeof(WATERF_TIMES)+
                 (unsigned int)wg_waterf_size*sizeof(short int)+
                 (unsigned int)screen_height*sizeof(char));
if(wg_waterf == NULL)
  {
  lirerr(1036);
  return;
  }
wg_background=(unsigned char*)(wg_waterf)+
                              (unsigned int)wg_waterf_size*sizeof(short int);
wg_waterf_times=(WATERF_TIMES*)((unsigned char*)(wg_background)+
                                   (unsigned int)screen_height*sizeof(char));
for(i=0;i<wg_waterf_size;i++)wg_waterf[i]=(short int)0x8000;
for(i=0;i<max_wg_waterf_times; i++)wg_waterf_times[i].line=10000;
// Write out the y scale for logarithmic spectrum graph.
// We want a point with amplitude 1<<(fft1_n/2) to be placed at the
// zero point of the dB scale. 
for(i=0; i<screen_height; i++)wg_background[i]=0;
ypixels=wg.ybottom-wg.yborder-1;
wg_db_per_pixel=(float)(20*log10(wg.yrange)/ypixels);
db_scalestep=1.3*wg_db_per_pixel*text_height;
adjust_scale(&db_scalestep);
t1=(float)(20*log10(wg.yzero));
i=(int)(t1/db_scalestep);
scale_value=i*(1.00000*db_scalestep);
scale_y=(float)(wg.ybottom+(t1-scale_value)/wg_db_per_pixel);
scale_decimals=0;
scale_rnd=.01F;
if(db_scalestep < 1)
  {
  sprintf(s,"%f",db_scalestep);
  while(s[scale_decimals] != '.')scale_decimals++;
  scale_decimals++;
  while(s[scale_decimals] == '0' && s[scale_decimals] != 0)
    {
    scale_decimals++;
    scale_rnd/=10;
    }
  } 
while( scale_y-(float)(text_height/2-2) > wg.yborder+1.5*text_height)
  {
  i=0;
  if(scale_y+(float)(text_height+2) < wg.ybottom)
    {
    if(scale_value < 0)
      {
      t1=(float)(scale_value-scale_rnd);
      }
    else  
      {
      t1=(float)(scale_value+scale_rnd);
      }
    sprintf(s,"%f",t1);
    while(s[i] != '.')i++;
    if(scale_decimals != 0)
      {
      i+=scale_decimals;
      }
    s[i]=0;
    lir_pixwrite(wg.xleft+text_width/2,(int)scale_y-text_height/2+2,s);
    }
  if(scale_y+2 < wg.ybottom)
    {
    k=(int)scale_y;
    wg_background[k]=WG_DBSCALE_COLOR;    
    lir_hline(wg_first_xpixel+scale_decimals*text_width,
                                        k,wg_last_xpixel,WG_DBSCALE_COLOR);
    if(kill_all_flag) return;
    }
  scale_y-=(float)(db_scalestep/wg_db_per_pixel);
  scale_value+=db_scalestep;
  }
// Init fft1_spectrum as the base line and place it on the screen. 
for(i=wg_first_xpixel; i<=wg_last_xpixel; i++) 
  {
  fft1_spectrum[i]=(short int)(wg.ybottom-1);
  }
lir_hline(wg_first_xpixel,wg.ybottom-1,wg_last_xpixel,15);
new_fft1_averages(fft1_sumsq_pa, fft1_first_point, fft1_last_point);
make_button(wg.xleft+text_width,wg.ybottom-text_height/2-2,
                                         wgbutt,WG_YSCALE_EXPAND,24);
make_button(wg.xleft+3*text_width,wg.ybottom-text_height/2-2,
                                         wgbutt,WG_YSCALE_CONTRACT,25);
i=(wg.ybottom+wg.yborder)/2;
make_button(wg.xright-text_width,i-text_height/2-2,
                                         wgbutt,WG_YZERO_DECREASE,24);
make_button(wg.xright-text_width,i+text_height/2+2,
                                         wgbutt,WG_YZERO_INCREASE,25);
make_button(wg.xright-text_width,wg.ybottom-text_height/2-2,
                               wgbutt,WG_AVG1NUM,(char)(wg.fft_avg1num+48)); 
if(wg_freq_adjustment_mode == 0)
  {
  make_button(wg.xleft+text_width,wg.ytop+text_height/2+3,
                                         wgbutt,WG_FQMIN_DECREASE,26);
  make_button(wg.xleft+3*text_width,wg.ytop+text_height/2+3,
                                         wgbutt,WG_FQMIN_INCREASE,27);
  make_button(wg.xright-3*text_width,wg.ytop+text_height/2+3,
                                         wgbutt,WG_FQMAX_DECREASE,26);
  make_button(wg.xright-text_width,wg.ytop+text_height/2+3,
                                         wgbutt,WG_FQMAX_INCREASE,27); 
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].x1=wgbutt[WG_FQMIN_INCREASE].x2+2;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].x2=wgbutt[WG_FQMAX_DECREASE].x1-2;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].y1=wgbutt[WG_FQMAX_DECREASE].y1;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].y2=wgbutt[WG_FQMAX_DECREASE].y2;
  wg_freq_x1=wgbutt[WG_FQMIN_INCREASE].x2;
  wg_freq_x2=wgbutt[WG_FQMAX_DECREASE].x1;
  }
else
  {
  wgbutt[WG_LOWEST_FREQ].x1=wg.xleft+2;
  wgbutt[WG_LOWEST_FREQ].x2=wgbutt[WG_LOWEST_FREQ].x1+15*text_width;
  wgbutt[WG_LOWEST_FREQ].y1=wg.ytop+2;
  wgbutt[WG_LOWEST_FREQ].y2=wgbutt[WG_LOWEST_FREQ].y1+text_height+2;
  wgbutt[WG_HIGHEST_FREQ].x2=wg.xright-2;
  wgbutt[WG_HIGHEST_FREQ].x1=wgbutt[WG_HIGHEST_FREQ].x2-15*text_width;
  wgbutt[WG_HIGHEST_FREQ].y1=wgbutt[WG_LOWEST_FREQ].y1;
  wgbutt[WG_HIGHEST_FREQ].y2=wgbutt[WG_LOWEST_FREQ].y2;
  if(wgbutt[WG_LOWEST_FREQ].x2+17 >= wgbutt[WG_HIGHEST_FREQ].x1)
    {
    i=(wgbutt[WG_LOWEST_FREQ].x2+wgbutt[WG_HIGHEST_FREQ].x1-1)/2;
    wgbutt[WG_LOWEST_FREQ].x2=i+8;
    wgbutt[WG_HIGHEST_FREQ].x1=i-8;
    }
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].x1=wgbutt[WG_LOWEST_FREQ].x2+2;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].x2=wgbutt[WG_HIGHEST_FREQ].x1-2;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].y1=wgbutt[WG_LOWEST_FREQ].y1;
  wgbutt[WG_FREQ_ADJUSTMENT_MODE].y2=wgbutt[WG_LOWEST_FREQ].y2;
  wg_freq_x1=-1;
  }  
if(genparm[AFC_ENABLE] == 2)
  {
  if(wg.spur_inhibit == 0)
    {
    button_color=14;
    }
  else
    {
    button_color=3;
    }  
  settextcolor(button_color);
  make_button(wg.xright-text_width,wg.ytop+3*text_height+3,
                                         wgbutt,WG_SPUR_TOGGLE,'c'); 
  show_button(&wgbutt[WG_SPUR_TOGGLE],"c");
  button_color=7;
  settextcolor(7);
  }
spur_search_first_point=wg_first_point;
spur_search_last_point=wg_last_point;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  spur_search_first_point*=fft2_to_fft1_ratio;
  spur_search_last_point*=fft2_to_fft1_ratio;
  }
autospur_point=spur_search_first_point+SPUR_WIDTH/2+1;
// ***********  fft1 averaging number button ******************
wgbutt[WG_FFT1_AVGNUM].x1=2+wg.xleft;
wgbutt[WG_FFT1_AVGNUM].x2=2+wg.xleft+(11*text_width)/2;
wgbutt[WG_FFT1_AVGNUM].y1=2+wg.yborder;
wgbutt[WG_FFT1_AVGNUM].y2=2+wg.yborder+text_height;
wgbutt[WG_WATERF_AVGNUM].x1=2+wg.xleft;
wgbutt[WG_WATERF_AVGNUM].x2=2+wg.xleft+(9*text_width)/2;
wgbutt[WG_WATERF_AVGNUM].y1=wg.yborder-text_height-2;
wgbutt[WG_WATERF_AVGNUM].y2=wg.yborder-2;
wgbutt[WG_WATERF_GAIN].x1=wg.xright-(9*text_width)/2-2;
wgbutt[WG_WATERF_GAIN].x2=wg.xright-2;
wgbutt[WG_WATERF_GAIN].y1=wg.yborder-2*text_height-4;
wgbutt[WG_WATERF_GAIN].y2=wg.yborder-text_height-4;
wgbutt[WG_WATERF_ZERO].x1=wg.xright-(11*text_width)/2-2;
wgbutt[WG_WATERF_ZERO].x2=wg.xright-2;
wgbutt[WG_WATERF_ZERO].y1=wg.yborder-text_height-2;
wgbutt[WG_WATERF_ZERO].y2=wg.yborder-2;
make_wg_yfac();
make_modepar_file(GRAPHTYPE_WG);
wg_flag=1;
wg_waterf_block=(wg_waterf_y2-wg_waterf_y1+1)*wg_xpixels;
make_wg_waterf_cfac();
// Check if there are selected frequencies that are still
// within the permitted range.
// Place their cursors on screen and remove other selected frequencies.
if(rx_mode != MODE_TXTEST && rx_mode != MODE_RADAR)
  {
  for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
    {
    mix1_curx[i]=-1;
    if( mix1_selfreq[i] > mix1_lowest_fq  && mix1_selfreq[i]<mix1_highest_fq)
      {
      add_mix1_cursor(i);
      if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
        {
        for(j=0; j<max_fftxn; j++)
          {
          k=i*max_fftxn+j;
          if(mix1_fq_mid[k]<mix1_lowest_fq)mix1_fq_mid[k]=mix1_lowest_fq;
          if(mix1_fq_mid[k]>mix1_highest_fq)mix1_fq_mid[k]=mix1_highest_fq;
          }
        }
      }
    else
      {
      mix1_selfreq[i]=-1;
      new_mix1_curx[i]=-1;
      mix1_point[i]=-1;
      }
    }
  }
if( (ui.network_flag&NET_RX_OUTPUT) != 0)
  {  
  for(i=0; i<MAX_FREQLIST; i++)
    {
    netfreq_curx[i]=-1;
    }
  }
wg_timestamp_counter=0;
if(new_baseb_flag == -1)baseb_reset_counter++;
wg_refresh_time=current_time();
wg_refresh_flag=0;
if(rx_mode == MODE_TXTEST)txtest_init();
latest_wg_spectrum=0;
latest_wg_spectrum_shown=0;
resume_thread(THREAD_SCREEN);
//update_indicator();
sc[SC_WG_WATERF_INIT]++;
sc[SC_WG_BUTTONS]++;
sc[SC_WG_FQ_SCALE]++;
}

void wg_default(void)
{
wg.check=WG_VERNR;
wg.yrange=32768;
wg.yzero=1;
wg.waterfall_db_gain=.25;
wg.waterfall_db_zero=20;
wg.spek_avgnum=1+(int)(0.2/fft1_blocktime);  
if(wg.spek_avgnum > 800)wg.spek_avgnum=800;
wg.fft_avg1num=wg.spek_avgnum/3;
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  wg.waterfall_avgnum=2*wg.spek_avgnum;
  }
else
  {
  wg.waterfall_avgnum=1+(int)(5/fft2_blocktime);
  }  
check_wg_fft_avgnum();
}


void wg_default_x(void)
{
int i;
float t1;
// Make the default window for the wide graphs (spectrum and waterfall)
wg.xleft=0;
i=fft1_size+6*text_width-1;
if(i > screen_width-1)i=screen_width-1;
wg.xright=i;
// Decide what range in fft1 points we want to place on the x-axis 
// Default is to show approximately half the spectrum.
wg_first_xpixel=wg.xleft+4*text_width;
wg_last_xpixel=wg.xright-2*text_width;
wg_xpixels=wg_last_xpixel-wg_first_xpixel+1;
t1=(0.5F*(float)fft1_size)/(float)wg_xpixels;
if(t1 > 1)
  {
  wg.xpoints_per_pixel=(int)(t1+0.5F);
  wg.xpoints=wg.xpoints_per_pixel*wg_xpixels;
  wg.pixels_per_xpoint=0;
  }
else
  {
  wg.pixels_per_xpoint=(int)(1.01F/t1);
  wg.xpoints=wg_xpixels/wg.pixels_per_xpoint;
  if(wg.pixels_per_xpoint == 1)
    {  
    wg.xpoints_per_pixel=1;
    }
  else
    {  
    wg.xpoints_per_pixel=0;
    }
  }
wg.first_xpoint=(fft1_size-wg.xpoints)/3;
if(wg.first_xpoint<0)lirerr(314);
}

void wg_default_y(void)
{
wg.ytop=0;
wg.yborder=(int)(0.2F*(float)screen_height);
wg.ybottom=(int)(0.35F*(float)screen_height);
}


void init_wide_graph()
{
if (read_modepar_file(GRAPHTYPE_WG) == 0)
  {
  wg_default();
  }
if(wg.check!=WG_VERNR)wg_default();
if(wg.xpoints_per_pixel*wg.pixels_per_xpoint > 1)wg_default_x();
if(wg.first_xpoint < 0)wg_default_x();
if(wg.first_xpoint+wg.xpoints > fft1_size)wg_default_x();  
if(wg.xleft<0 || wg.xleft > wg.xright-32-6*text_width)wg_default_x();
if(wg.xright>=screen_width)wg_default_x();
if(wg.ytop < 0 || wg.ytop >wg.ybottom-50)wg_default_y(); 
if(wg.ybottom>=screen_height)wg_default_y();
if(wg.yborder < wg.ytop+text_height+YBO || 
                wg.yborder > wg.ybottom-2*text_height)wg_default_y();
wg.spur_inhibit &= 1;
if(genparm[SECOND_FFT_ENABLE] != 0 && wg.pixels_per_xpoint>2)
                                   make_power_of_two(&wg.pixels_per_xpoint);
check_wg_fft_avgnum();
wide_graph_scro=no_of_scro;
if(fft1_size < 32)
  {
  lir_status=LIR_PARERR;
  return;
  }
wg_freq_adjustment_mode=0;
airspyhf_old_point=10000;
make_wide_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}
