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
   
#define HG_STON_RANGE 6.0


#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "blnkdef.h"
#include "seldef.h"
#include "graphcal.h"
#include "vernr.h"
#include "thrdef.h"
#include "sigdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

int hires_graph_scro;
int hg_old_x1;
int hg_old_x2;
int hg_old_y1;
int hg_old_y2;
void make_hires_graph(int clear_old);

void new_hg_pol(void)
{
float *fftxy, *pwr;
int i, j, j_last;
int ib,k;
float t1,t2,t3,t4,r1,r2;
short int *zxy;
ib=hg_last_point;
k=2*hg_size;
for(i=0; i<k; i++) hg_fft2_pwrsum[i]=0;
j=fft2_nb;
j_last=(fft2_na+fft2n_mask)&fft2n_mask;
nwpol:;
pwr=&hg_fft2_pwr[2*j*hg_size];
k=0;
if(fft_cntrl[FFT2_CURMODE].mmx == 0)
  { 
  fftxy=&fft2_float[twice_rxchan*j*fft2_size];
  for(i=hg_first_point; i<ib; i++)
    {
    t1=fftxy[4*i  ];
    t2=fftxy[4*i+1];
    t3=fftxy[4*i+2];
    t4=fftxy[4*i+3];
    r1=pg.c1*t1+pg.c2*t3+pg.c3*t4;
    r2=pg.c1*t2+pg.c2*t4-pg.c3*t3;
    pwr[2*k  ]=r1*r1+r2*r2;
    hg_fft2_pwrsum[2*k  ]+=pwr[2*k  ];
    r1=pg.c1*t3-pg.c2*t1+pg.c3*t2;
    r2=pg.c1*t4-pg.c2*t2-pg.c3*t1;
    pwr[2*k+1]=r1*r1+r2*r2;
    hg_fft2_pwrsum[2*k+1]+=pwr[2*k+1];
    k++;
    }
  }
else
  {
  zxy=&fft2_short_int[twice_rxchan*j*fft2_size];
  for(i=hg_first_point; i<ib; i++)
    {
    t1=zxy[4*i  ];
    t2=zxy[4*i+1];
    t3=zxy[4*i+2];
    t4=zxy[4*i+3];
    r1=pg.c1*t1+pg.c2*t3+pg.c3*t4;
    r2=pg.c1*t2+pg.c2*t4-pg.c3*t3;
    pwr[2*k  ]=r1*r1+r2*r2;
    hg_fft2_pwrsum[2*k  ]+=pwr[2*k  ];
    r1=pg.c1*t3-pg.c2*t1+pg.c3*t2;
    r2=pg.c1*t4-pg.c2*t2-pg.c3*t1;
    pwr[2*k+1]=r1*r1+r2*r2;
    hg_fft2_pwrsum[2*k+1]+=pwr[2*k+1];
    k++;
    }
  }
if(j==j_last)return;
j=(j+1)&fft2n_mask;
goto nwpol;
}


void change_hg_spectrum_zero(void)
{
hg.spek_zero=numinput_int_data;
hg_ston1_yold=-1;
hg_ston2_yold=-1;
sc[SC_HG_Y_SCALE]++;
sc[SC_SHOW_FFT2]++;
make_modepar_file(GRAPHTYPE_HG);
}

void change_hg_spectrum_gain(void)
{
hg.spek_gain=numinput_int_data;
sc[SC_HG_Y_SCALE]++;
sc[SC_SHOW_FFT2]++;
hg_ston1_yold=-1;
hg_ston2_yold=-1;
make_modepar_file(GRAPHTYPE_HG);
}

void make_hires_valid(void)
{
float *pwra;
int i, j, k, ib;
if(sw_onechan)
  {
  ib=hg_last_point;
  j=fft2_nb;  
  pwra=&fft2_power_float[j*fft2_size];
  k=0;
  for(i=hg_first_point; i<ib; i++)
    {
    hg_fft2_pwrsum[k]=pwra[i];
    k++;
    }
sum_2:;
  k=0; 
  pwra=&fft2_power_float[j*fft2_size];
  for(i=hg_first_point; i<ib; i++)
    {
    hg_fft2_pwrsum[k]+=pwra[i];
    k++;
    }
  j=(j+1)&fft2n_mask;  
  if(j == fft2_na)goto finish;
  goto sum_2;
  }
else
  {  
  new_hg_pol();
  }
finish:;  
}

void new_fft2_averages(void)
{
float *pwra;
TWOCHAN_POWER *y;
int i, j, n;
if(mix1_selfreq[0] < 0)return;
// The number to average over may have changed, update fft2_nb
i=(fft2_na-hg.spek_avgnum+max_fft2n)&fft2n_mask;
n=0;
if(i != fft2_nb)
  {
  fft2_nb=i;
  if(sw_onechan)
    {
    pwra=&fft2_power_float[fft2_nb*fft2_size];
    for(i=0; i<fft2_size; i++)
      {
      fft2_powersum_float[i]=pwra[i];
      }
    n+=fft2_size;
    j=fft2_nb;
    if(j==fft2_na)goto ex1;
sum:;
    j=(j+1)&fft2n_mask;
    if(j==fft2_na)goto ex1;
    pwra=&fft2_power_float[j*fft2_size];
    for(i=0; i<fft2_size; i++)
      {
      fft2_powersum_float[i]+=pwra[i];
      }
    n+=fft2_size;
    if(n>3000000)
      {
      lir_sched_yield();
      n=0;
      }
    goto sum;
ex1:;
    }
  else
    {  
    y=&fft2_xypower[fft2_nb*fft2_size];
    for(i=0; i<fft2_size; i++)
      {
      fft2_xysum[i].x2   =y[i].x2;
      fft2_xysum[i].y2   =y[i].y2;
      fft2_xysum[i].im_xy=y[i].im_xy;
      fft2_xysum[i].re_xy=y[i].re_xy;
      }
    n+=fft2_size;
    j=fft2_nb;
    if(j==fft2_na)goto ex2;
sumxy:;
    j=(j+1)&fft2n_mask;
    if(j==fft2_na)goto ex2;
    y=&fft2_xypower[j*fft2_size];
    for(i=0; i<fft2_size; i++)
      {
      fft2_xysum[i].x2   +=y[i].x2;
      fft2_xysum[i].y2   +=y[i].y2;
      fft2_xysum[i].im_xy+=y[i].im_xy;
      fft2_xysum[i].re_xy+=y[i].re_xy;
      }
    n+=fft2_size;
    if(n>1000000)
      {
      lir_sched_yield();
      n=0;
      }
    goto sumxy;
ex2:;
    }
  }
make_hires_valid();
}

void help_on_hires_graph(void)
{
int msg_no;
int event_no;
// Set msg to select a frequency in case it is not button or border
msg_no=13;
// In case we are on one of the control bars, select the
// appropriate message.
if(mouse_y >= hg_ston_y0)
  {
  if(mouse_x >= timf2_hg_xmin && mouse_x <= timf2_hg_xmax)
    {
    msg_no=14;
    }
  else
    {
    msg_no=-1;
    }  
  }
else
  {

  if( mouse_x<hg_first_xpixel && mouse_y > hg_stonbars_ytop)
    {
    if(mouse_x <= hg_ston1_x2)
      {
      msg_no=15;
      }
    else
      {
      msg_no=16;
      }  
    }
  }
for(event_no=0; event_no<MAX_HGBUTT; event_no++)
  {
  if( hgbutt[event_no].x1 <= mouse_x && 
      hgbutt[event_no].x2 >= mouse_x &&      
      hgbutt[event_no].y1 <= mouse_y && 
      hgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case HG_TOP:
      case HG_BOTTOM:
      case HG_LEFT:
      case HG_RIGHT:
      msg_no=100;
      break;

      case HG_BLN_STUPID:
      msg_no=17;
      break;

      case HG_BLN_CLEVER:
      msg_no=18;
      break;  

      case HG_TIMF2_STATUS:
      if(sw_onechan)
        {
        msg_no=19;
        }
      else
        {
        msg_no=20;
        }  
      break;

      case HG_TIMF2_ST_INC:
      msg_no=73;
      break;

      case HG_TIMF2_ST_DEC:
      msg_no=74;
      break;

      case HG_TIMF2_WK_INC:
      msg_no=75;
      break;

      case HG_TIMF2_WK_DEC:
      msg_no=76;
      break;

      case HG_TIMF2_LINES:
      msg_no=77;
      break;
  
      case HG_TIMF2_HOLD:
      msg_no=78;
      break;

      case HG_FFT2_AVGNUM:
      msg_no=62;
      break;

      case HG_SPECTRUM_GAIN:
      msg_no=317;
      break;

      case HG_SPECTRUM_ZERO:
      msg_no=318;
      break;

      case HG_MAP65_GAIN:
      msg_no=334;
      break;
      
      case HG_MAP65_STRONG:
      msg_no=335;
      break;

      case HG_SELLIM_PAR1:
      msg_no=345;
      break;
      
      case HG_SELLIM_PAR2:
      msg_no=346;
      break;
      
      case HG_SELLIM_PAR3:
      msg_no=347;
      break;
      
      case HG_SELLIM_PAR4:
      msg_no=348;
      break;
      
      case HG_SELLIM_PAR5:
      msg_no=349;
      break;

      case HG_SELLIM_PAR6:
      msg_no=350;
      break;
      
      case HG_SELLIM_PAR7:
      msg_no=351;
      break;
      
      case HG_SELLIM_PAR8:
      msg_no=352;
      break;
      }
    }  
  }
help_message(msg_no);
}  

void change_fft2avgnum(void)
{
hg.spek_avgnum=numinput_int_data;
if(hg.spek_avgnum <1)hg.spek_avgnum=1;
if(hg.spek_avgnum>fft2n_mask)hg.spek_avgnum=fft2n_mask;
make_modepar_file(GRAPHTYPE_HG);
new_fft2_averages();
hg_ston1_yold=-1;
hg_ston2_yold=-1;
sc[SC_HG_FQ_SCALE]++;
sc[SC_SHOW_FFT2]++;
sc[SC_HG_Y_SCALE]++;
}

void change_map65(void)
{
hg.map65_gain_db=numinput_int_data;
if(hg.map65_gain_db < 0)hg.map65_gain_db=0;
if(hg.map65_gain_db > 15)hg.map65_gain_db=15;
hg_map65_gain=(float)pow(10.0,-(float)hg.map65_gain_db/20.F);
make_modepar_file(GRAPHTYPE_HG);
sc[SC_SHOW_MAP65]++;
}

void mouse_continue_hires_graph(void)
{
int j;
switch (mouse_active_flag-1)
  {
  case HG_TOP:
  if(hg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&hg,0);
    hg.ytop=mouse_y;
    j=hg.ybottom-4*text_height;
    if(hg.ytop > j)hg.ytop=j;      
    if(hg_old_y1 > hg.ytop)hg_old_y1=hg.ytop;
    graph_borders((void*)&hg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case HG_BOTTOM:
  if(hg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&hg,0);
    hg.ybottom=mouse_y;
    j=hg.ytop+4*text_height;
    if(hg.ybottom < j)hg.ybottom=j;      
    if(hg_old_y2 < hg.ybottom)hg_old_y2=hg.ybottom;
    graph_borders((void*)&hg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case HG_LEFT:
  if(hg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&hg,0);
    hg.xleft=mouse_x;
    j=hg.xright-32-6*text_width;
    if(hg.xleft > j)hg.xleft=j;      
    if(hg_old_x1 > hg.xleft)hg_old_x1=hg.xleft;
    graph_borders((void*)&hg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case HG_RIGHT:
  if(hg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&hg,0);
    hg.xright=mouse_x;
    j=hg.xleft+32+6*text_width;
    if(hg.xright < j)hg.xright=j;      
    if(hg_old_x2 < hg.xright)hg_old_x2=hg.xright;
    graph_borders((void*)&hg,15);
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
  case HG_BLN_STUPID:
  hg.stupid_bln_mode++;
  if(hg.stupid_bln_mode>2)hg.stupid_bln_mode=0;
  break;

  case HG_BLN_CLEVER:
  if( (fft1_calibrate_flag&CALAMP)==CALAMP)
    {
    hg.clever_bln_mode++;
    if(hg.clever_bln_mode>2)hg.clever_bln_mode=0;
    }
  break;  

  case HG_TIMF2_STATUS:
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    hg.timf2_oscilloscope+=1;
    if(hg.timf2_oscilloscope > 6)hg.timf2_oscilloscope=0;
    timf2_oscilloscope_counter=0;
    timf2_oscilloscope_maxval_uint=0;
    timf2_oscilloscope_powermax_uint=0;
    timf2_oscilloscope_maxval_float=0;
    timf2_oscilloscope_powermax_float=0;
    timf2_show_pointer=-1;
    }
  break;

  case HG_TIMF2_ST_INC:
  hg.timf2_oscilloscope_st_gain *=2;
  if(hg.timf2_oscilloscope_st_gain > 256)hg.timf2_oscilloscope_st_gain = 256;
  break;

  case HG_TIMF2_ST_DEC:
  hg.timf2_oscilloscope_st_gain /=2;
  if(hg.timf2_oscilloscope_st_gain < 0.00002)hg.timf2_oscilloscope_st_gain = 1./32768;
  break;

  case HG_TIMF2_WK_INC:
  hg.timf2_oscilloscope_wk_gain *=2;
  if(hg.timf2_oscilloscope_wk_gain > 256)hg.timf2_oscilloscope_wk_gain = 256;
  break;

  case HG_TIMF2_WK_DEC:
  hg.timf2_oscilloscope_wk_gain /=2;
  if(hg.timf2_oscilloscope_wk_gain < 0.00002)hg.timf2_oscilloscope_wk_gain = 1./32768;
  break;

  case HG_TIMF2_LINES:
  hg.timf2_oscilloscope_lines^=1;
  break;
  
  case HG_TIMF2_HOLD:
  hg.timf2_oscilloscope_hold^=1;
  break;

  case HG_FFT2_AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=hgbutt[HG_FFT2_AVGNUM].x1+text_width/2-1;
  numinput_ypix=hgbutt[HG_FFT2_AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_fft2avgnum;
  return;

  case HG_SPECTRUM_ZERO:
  mouse_active_flag=1;
  numinput_xpix=hgbutt[HG_SPECTRUM_ZERO].x1+text_width/2-1;
  numinput_ypix=hgbutt[HG_SPECTRUM_ZERO].y1+2;
  numinput_chars=3;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_hg_spectrum_zero;
  return;

  case HG_SPECTRUM_GAIN:
  mouse_active_flag=1;
  numinput_xpix=hgbutt[HG_SPECTRUM_GAIN].x1+text_width/2-1;
  numinput_ypix=hgbutt[HG_SPECTRUM_GAIN].y1+2;
  numinput_chars=3;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_hg_spectrum_gain;
  return;
  
  case HG_MAP65_GAIN:
  if(show_map65)
    {
    mouse_active_flag=1;
    numinput_xpix=hgbutt[HG_MAP65_GAIN].x1+text_width/2-1;
    numinput_ypix=hgbutt[HG_MAP65_GAIN].y1+2;
    numinput_chars=2;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=change_map65;
    }
  return;
  
  case HG_MAP65_STRONG:
  if(show_map65)
    {
    hg.map65_strong++;
    hg.map65_strong&=1;
    make_modepar_file(GRAPHTYPE_HG);
    sc[SC_SHOW_MAP65]++;
    }
  break;
  
  case HG_SELLIM_PAR1:
  hg.sellim_par1++;
  if(hg.sellim_par1 > 2)hg.sellim_par1=0;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR2:
  hg.sellim_par2 ^=1;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR3:
  hg.sellim_par3 ^=1;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR4:
  hg.sellim_par4 ^=1;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR5:
  hg.sellim_par5++;
  if(hg.sellim_par5 > 2)hg.sellim_par5=0;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR6:
  hg.sellim_par6 ^=1;
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR7:
  if(mg.scale_type == MG_SCALE_STON)
    {
    hg.sellim_par7=1; 
    }
  else
    {  
    hg.sellim_par7 ^=1;
    }
  sc[SC_SHOW_SELLIM]++;
  break;

  case HG_SELLIM_PAR8:
  hg.sellim_par8 ^=1;
  sc[SC_SHOW_SELLIM]++;
  break;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
timf2_hg_x[0]=-1;
timf2_hg_x[1]=-1;
new_fft2_averages();
make_hires_graph(TRUE);
if(kill_all_flag) return;
}


void hg_control_finish(void)
{
sc[SC_BLANKER_INFO]++;
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_HG);
  mouse_active_flag=0;
  }
}

void clever_limit_control(void)
{
if(mouse_x < timf2_hg_xmin || mouse_x > timf2_hg_xmax)
  {
  if(leftpressed == BUTTON_RELEASED)leftpressed=BUTTON_IDLE;  
  return;
  }
hg.clever_bln_limit=(unsigned int)(2*exp((float)
                (mouse_x-timf2_hg_xmin)/timf2_hg_xfac)/timf2_hg_xlog);
if(hg.clever_bln_mode == 1)
  {
  hg.clever_bln_factor=(float)hg.clever_bln_limit/(float)timf2_noise_floor;
  if(hg.clever_bln_factor < 0.7F)hg.clever_bln_factor=0.7F;
  }
hg_control_finish();
}

void stupid_limit_control(void)
{
if(mouse_x < timf2_hg_xmin || mouse_x > timf2_hg_xmax)
  {
  if(leftpressed == BUTTON_RELEASED)leftpressed=BUTTON_IDLE;  
  return;
  }
hg.stupid_bln_limit=(unsigned int)(2*exp((float)
                 (mouse_x-timf2_hg_xmin)/timf2_hg_xfac)/timf2_hg_xlog);
if(hg.stupid_bln_mode == 1)
  {
  hg.stupid_bln_factor=(float)hg.stupid_bln_limit/(float)timf2_noise_floor;
  if(hg.stupid_bln_factor < 0.7F) hg.stupid_bln_factor=0.7F;
  }
hg_control_finish();
}

void make_blanker_ston1(void)
{
int k;
k=hg_ston_y0-hg_ston1_y;
if(k<2)
  {
  k=2;
  hg_ston1_y=hg_ston_y0+2;
  }
hg.blanker_ston_fft1=(float)pow(10.,HG_STON_RANGE*k/hg_floatypix);
}

void make_blanker_ston2(void)
{
int k;
k=hg_ston_y0-hg_ston2_y;
if(k<2)
  {
  k=2;
  hg_ston2_y=hg_ston_y0+2;
  }
hg.blanker_ston_fft2=(float)pow(10.,HG_STON_RANGE*k/hg_floatypix);
}

void hg_ston1_control(void)
{
int yb;
yb=mouse_y;
if(yb > hg_ston_y0-2)yb=hg_ston_y0-2;
if(yb < hg_stonbars_ytop)yb=hg_stonbars_ytop;
if(hg_ston1_y!=yb)
  {
  hg_ston1_y=yb;
  make_blanker_ston1();
  }
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_HG);
  mouse_active_flag=0;
  }
sc[SC_HG_STONBARS_REDRAW]++;
}

void hg_ston2_control(void)
{
int yb;
yb=mouse_y;
if(yb > hg_ston_y0-2)yb=hg_ston_y0-2;
if(yb < hg_stonbars_ytop)yb=hg_stonbars_ytop;
if(hg_ston2_y!=yb)
  {
  hg_ston2_y=yb;
  make_blanker_ston2();
  }
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_HG);
  mouse_active_flag=0;
  }
sc[SC_HG_STONBARS_REDRAW]++;
}

void hires_graph_selfreq(void)
{
int i;
float t1;
if(leftpressed != BUTTON_RELEASED)return;
leftpressed=BUTTON_IDLE;  
t1=(float)hg_first_fq+(float)(mouse_x-hg_first_xpixel)*hg_hz_per_pixel;
new_mix1_curx[0]=-1;
if(t1 <  mix1_lowest_fq)t1=mix1_lowest_fq;
if(t1 > mix1_highest_fq)t1=mix1_highest_fq;
if(MAX_MIX1 > 1)
 {
  for(i=1; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
    {
    if( fabs(t1-mix1_selfreq[i]) < 3*wg_hz_per_pixel)
      {
      new_mix1_curx[i]=-1;
      mix1_selfreq[i]=-1;
      mix1_point[i]=-1;
      }
    }
  }  
make_new_signal(0, t1);
sc[SC_FREQ_READOUT]++;
sc[SC_HG_FQ_SCALE]++;
sc[SC_BG_FQ_SCALE]++;
baseb_reset_counter++;
mouse_active_flag=0;
}

void mouse_on_hires_graph(void)
{
int i, j, event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_HGBUTT; event_no++)
  {
  if( hgbutt[event_no].x1 <= mouse_x && 
      hgbutt[event_no].x2 >= mouse_x &&      
      hgbutt[event_no].y1 <= mouse_y && 
      hgbutt[event_no].y2 >= mouse_y) 
    {
    hg_old_y1=hg.ytop;
    hg_old_y2=hg.ybottom;
    hg_old_x1=hg.xleft;
    hg_old_x2=hg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_hires_graph;
    return;
    }
  }
// Not button or border.
// Check if mouse is on bars or in the spectrum
if(mouse_y >= hg_ston_y0)
  {
  i=abs(mouse_x - timf2_hg_xmin-timf2_hg_clex);
  j=abs(mouse_x - timf2_hg_xmin-timf2_hg_stux);
  if(hg.clever_bln_mode ==0)i+=screen_width;
  if(hg.stupid_bln_mode ==0)j+=screen_width;
  if(i<j)
    {
    if(i<text_width)
      {
      current_mouse_activity=clever_limit_control;
      }
    else
      {
      current_mouse_activity=mouse_nothing;
      }
    }
  else  
    {
    if(j<text_width)
      {
      current_mouse_activity=stupid_limit_control;
      }
    else
      {
      current_mouse_activity=mouse_nothing;
      }
    }
  }
else
  {
  if( mouse_x<hg_first_xpixel && mouse_y > hg_stonbars_ytop)
    {
    if(mouse_x <= hg_ston1_x2)
      {
      current_mouse_activity=hg_ston1_control;
      }
    else
      {
      current_mouse_activity=hg_ston2_control;
      }  
    }
  else
    {
    current_mouse_activity=hires_graph_selfreq;
    }
  }  
mouse_active_flag=1;
}

void make_hires_graph(int clear_old)
{
char s[80],chr;
int i,x,y,xt,yt;
float t1,t2;
pause_thread(THREAD_SCREEN);

if(clear_old)
  {
  hide_mouse(hg_old_x1,hg_old_x2,hg_old_y1,hg_old_y2);
  lir_fillbox(hg_old_x1,hg_old_y1,hg_old_x2-hg_old_x1+1,
                                                    hg_old_y2-hg_old_y1+1,0);
  }
current_graph_minh=7*text_height/2;
current_graph_minw=25*text_width;
check_graph_placement((void*)(&hg));
clear_button(hgbutt, MAX_HGBUTT);
hide_mouse(hg.xleft,hg.xright,hg.ytop,hg.ybottom);  
hg_first_xpixel=hg.xleft+2*text_width+2;
hg_last_xpixel=hg.xright-text_width-3;
hg_size=hg_last_xpixel-hg_first_xpixel;
if(hg_size > fft2_size)
  {
  hg_size=fft2_size;
  hg_first_xpixel=hg_last_xpixel-hg_size;
  hg.xleft=hg_first_xpixel-2*text_width-2;
  }
scro[hires_graph_scro].no=HIRES_GRAPH;
scro[hires_graph_scro].x1=hg.xleft;
scro[hires_graph_scro].x2=hg.xright;
scro[hires_graph_scro].y1=hg.ytop;
scro[hires_graph_scro].y2=hg.ybottom;
hgbutt[HG_LEFT].x1=hg.xleft;
hgbutt[HG_LEFT].x2=hg.xleft+2;
hgbutt[HG_LEFT].y1=hg.ytop;
hgbutt[HG_LEFT].y2=hg.ybottom;
hgbutt[HG_RIGHT].x1=hg.xright-2;
hgbutt[HG_RIGHT].x2=hg.xright;
hgbutt[HG_RIGHT].y1=hg.ytop;
hgbutt[HG_RIGHT].y2=hg.ybottom;
hgbutt[HG_TOP].x1=hg.xleft;
hgbutt[HG_TOP].x2=hg.xright;
hgbutt[HG_TOP].y1=hg.ytop;
hgbutt[HG_TOP].y2=hg.ytop+2;
hgbutt[HG_BOTTOM].x1=hg.xleft;
hgbutt[HG_BOTTOM].x2=hg.xright;
hgbutt[HG_BOTTOM].y1=hg.ybottom-2;
hgbutt[HG_BOTTOM].y2=hg.ybottom;
// Draw the border lines
graph_borders((void*)&hg,7);
hg_ymax=hg.ytop+3*text_height/2;
hg_stonbars_ytop=hg_ymax+text_height+1;
settextcolor(7);
x=hg.xleft+text_width;
y=hg.ybottom-text_height/2-2;
settextcolor(14);
make_button(x,y,hgbutt,HG_BLN_STUPID,modes_man_auto[hg.stupid_bln_mode]);
x+=2*text_width;
settextcolor(11);
make_button(x,y,hgbutt,HG_BLN_CLEVER,modes_man_auto[hg.clever_bln_mode]);
settextcolor(7);
hg_hz_per_pixel=timf1_sampling_speed/(float)fft2_size;
// Allocate memory for the high resolution graph.
hg_ston1_x1=hg.xleft+1;
hg_ston1_x2=hg.xleft+text_width;
hg_ston2_x1=hg_first_xpixel-text_width;
hg_ston2_x2=hg_first_xpixel-1;
hg_ston_y0=hg.ybottom-3*text_height/2-1;
if(hires_handle != NULL)
  {
  memcheck(2,hiresmem,&hires_handle);
  if(kill_all_flag)return;
  hires_handle=chk_free(hires_handle);
  }
init_memalloc(hiresmem, MAX_HIRES_ARRAYS);
mem(1,&hg_spectrum,(size_t)(ui.rx_rf_channels*screen_width)*sizeof(short int),0);
mem(2,&hg_background,(size_t)screen_height*sizeof(char),0);
mem(3,&hg_stonbuf,(size_t)(text_width*(hg.ybottom-hg.ytop))*sizeof(char),0);
hires_totmem=memalloc(&hires_handle,"hires");
if(hires_totmem == 0)
  {
  lirerr(1066);
  return;
  }
// ********************************************************
// ********************************************************
// Write out the y scale for logarithmic spectrum graph.
// The zero level is the level set by wg.waterfall_db_zero and the number
// of dB per pixel is set by wg.waterfall_db_gain
// This way the wide graph will reflect the colour scale of the
// waterfall graph.
hg_redraw_counter=0;
hg_y0=hg.ybottom-text_height/2-4;
if(hg.timf2_oscilloscope != 0)
  {
  hg_y0-=text_height+2;
  make_button(hg.xright-text_width,hg_y0, hgbutt,HG_TIMF2_WK_INC,'+');
  make_button(hg.xright-3*text_width,hg_y0, hgbutt,HG_TIMF2_WK_DEC,'-');
  xt=hg.xleft+7*text_width/2;
  chr='L';
  if(  (hg.timf2_oscilloscope_lines&1) != 0)chr='P';
  if(xt+6*text_width > hg.xright)goto buttskip1;  
  make_button(xt,hg_y0, hgbutt,HG_TIMF2_LINES,chr);
  xt+=2*text_width;
  chr='C';
  if(  (hg.timf2_oscilloscope_hold&1) != 0)chr='H';
  if(xt+6*text_width > hg.xright)goto buttskip1;  
  make_button(xt,hg_y0, hgbutt,HG_TIMF2_HOLD,chr);
  xt+=2*text_width;
  if(xt+6*text_width > hg.xright)goto buttskip1;  
  make_button(xt,hg_y0, hgbutt,HG_TIMF2_ST_DEC,'-');
  xt+=2*text_width;
  if(xt+6*text_width > hg.xright)goto buttskip1;  
  make_button(xt,hg_y0, hgbutt,HG_TIMF2_ST_INC,'+');
  xt+=3*text_width/2;
  sprintf(s,"%f",hg.timf2_oscilloscope_st_gain);
  s[8]=0;
  i=7;
  while(s[i-1]!='.' && s[i]=='0')
    {
    s[i]=0;
    i--;
    }
  if(xt+6*text_width > hg.xright)goto buttskip1;  
  lir_pixwrite(xt,hg_y0-text_height/2+2,s);
//  xt+=i*text_width+3*text_width/2;
buttskip1:;
  sprintf(s,"%f",hg.timf2_oscilloscope_wk_gain);
  s[8]=0;
  i=7;
  while(s[i-1]!='.' && s[i]=='0')
    {
    s[i]=0;
    i--;
    }
  xt=hg.xright-11*text_width/2-i*text_width;
  if(xt <= hg.xleft)goto buttskip2;
  lir_pixwrite(xt,hg_y0-text_height/2+2,s);
  lir_fillbox(0,timf2_ymin[4*ui.rx_rf_channels],screen_width/2,
                         timf2_ymax[0]-timf2_ymin[4*ui.rx_rf_channels],0);
  hg_y0-=2;
buttskip2:;
  }
xt=hg.xright-3.5*text_width;
yt=hg.ytop+2*text_height+2;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR8,hg.sellim_par8+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR7,hg.sellim_par7+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR6,hg.sellim_par6+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR5,hg.sellim_par5+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR4,hg.sellim_par4+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR3,hg.sellim_par3+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR2,hg.sellim_par2+'0');
xt-=2*text_width;
make_button(xt,yt, hgbutt,HG_SELLIM_PAR1,hg.sellim_par1+'0');
// hg_fft2_powersum, which may be the sum of many power values
// may become inaccurate because it is calculated each time
// by adding the difference between a new and a old value.
// This will make the noise floor inaccurate when a very strong
// signal disappears.
// Make sure to acctually sum all power values now and then so
// errors disappear within a reasonable time, twice the time
// covered by all the data or 2 seconds, whichever is longest.
t2=(float)fft2_new_points/(float)timf1_sampling_speed;
t1=(float)hg.spek_avgnum*t2;
if(t1<2)t1=2;
hg_powersum_recalc=(int)((float)hg_size*t2/t1);
// Make a box in which timf2 levels will be drawn.
timf2_hg_xmin=x+3*text_width/2;
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  if(hg.timf2_oscilloscope == 0)
    {
    s[0]='o';
    }
  else
    {
    s[0]=hg.timf2_oscilloscope+'0';
    }  
  make_button(hg.xright-text_width,y,hgbutt,HG_TIMF2_STATUS,s[0]);
  }
hg_y0-=text_height/2+2;
xt=hg.xleft+5*text_width/2+2;
hgbutt[HG_FFT2_AVGNUM].x1=2+hg.xleft;
hgbutt[HG_FFT2_AVGNUM].x2=2+hg.xleft+9*text_width/2;
hgbutt[HG_FFT2_AVGNUM].y1=2+hg.ytop;
hgbutt[HG_FFT2_AVGNUM].y2=2+hg.ytop+text_height;
hgbutt[HG_SPECTRUM_GAIN].x1=hg.xright-7*text_width/2-2;
hgbutt[HG_SPECTRUM_GAIN].x2=hg.xright-2;
hgbutt[HG_SPECTRUM_GAIN].y1=hg_y0-text_height;
hgbutt[HG_SPECTRUM_GAIN].y2=hg_y0;
hgbutt[HG_SPECTRUM_ZERO].x1=xt;
hgbutt[HG_SPECTRUM_ZERO].x2=xt+7*text_width/2;
hgbutt[HG_SPECTRUM_ZERO].y1=hg_y0-text_height;
hgbutt[HG_SPECTRUM_ZERO].y2=hg_y0;
hgbutt[HG_MAP65_GAIN].x1=hgbutt[HG_SPECTRUM_ZERO].x2+text_width/2;
hgbutt[HG_MAP65_GAIN].x2=hgbutt[HG_MAP65_GAIN].x1+5*text_width/2;
hgbutt[HG_MAP65_GAIN].y1=hg_y0-text_height;
hgbutt[HG_MAP65_GAIN].y2=hg_y0;
hgbutt[HG_MAP65_STRONG].x1=hgbutt[HG_MAP65_GAIN].x2+text_width/2;
hgbutt[HG_MAP65_STRONG].x2=hgbutt[HG_MAP65_STRONG].x1+3*text_width/2;
hgbutt[HG_MAP65_STRONG].y1=hg_y0-text_height;
hgbutt[HG_MAP65_STRONG].y2=hg_y0;
show_map65=(genparm[SECOND_FFT_ENABLE] != 0 &&
                          (ui.network_flag & NET_RXOUT_TIMF2) );
for(i=0; i<screen_width*ui.rx_rf_channels; i++) hg_spectrum[i]=(short int)hg_y0;
if(kill_all_flag) return;
timf2_hg_xmax=hg.xright-2*text_width-2;
timf2_hg_y[1]=y;
timf2_hg_y[0]=y-text_height/2;
timf2_hg_x[0]=0;
if(sw_onechan)
  {
  timf2_hg_yh=text_height;
  }
else
  {
  timf2_hg_yh=text_height/2;
  timf2_hg_x[1]=0;
  }
lir_fillbox(timf2_hg_xmin,timf2_hg_y[0],
                                timf2_hg_xmax-timf2_hg_xmin+1,text_height,8);
// timf2 is 16 bit, level is 0 to 32767 (+ or -)
// Make a logarithmic scale that uses all pixels for the power level. 
t1=1;
t2=32767.F*32767.F;
for(i=0; i<4; i++)
  {
  timf2_hg_xfac=(float)log(t2/t1)/((float)(timf2_hg_xmax-timf2_hg_xmin)+0.5F);
  t1=(float)exp(timf2_hg_xfac);
  }
timf2_hg_xfac=1/timf2_hg_xfac;
timf2_hg_xlog=1/t1;
timf2_hg_stux=-1;
timf2_hg_clex=-1;
// Set the quantisation error limit to some reasonable value (20dB = 100)
t1=100;
timf2_hg_qex=(int)(log(timf2_hg_xlog*t1)*timf2_hg_xfac);
lir_line(timf2_hg_xmin+timf2_hg_qex,timf2_hg_y[0],
                     timf2_hg_xmin+timf2_hg_qex,timf2_hg_y[0]+text_height,12);
if(kill_all_flag) return;
make_modepar_file(GRAPHTYPE_HG);
hg_flag=1;
fft2_nb=(fft2_na-hg.spek_avgnum+max_fft2n)&fft2n_mask;
hg_cury2=hg.ybottom-3*text_height/2;
hg_cury1=hg_stonbars_ytop;
hg_cury0=(hg_cury1+4*hg_cury2)/5;
// Get the number of pixels we have vertically for control bars.
hg_floatypix=hg_ston_y0-hg.ytop-2*text_height;
hg_ston1_y=hg_ston_y0-(int)(hg_floatypix*log10(hg.blanker_ston_fft1)/HG_STON_RANGE);
hg_ston2_y=hg_ston_y0-(int)(hg_floatypix*log10(hg.blanker_ston_fft2)/HG_STON_RANGE);
hg_ston1_yold=-1;
hg_ston2_yold=-1;
make_blanker_ston1();
make_blanker_ston2();
hg_center=0;
if(kill_all_flag)return;
afc_curx=-1;
sc[SC_HG_Y_SCALE]++;
sc[SC_BLANKER_INFO]++;
sc[SC_HG_FQ_SCALE]++;
sc[SC_SHOW_FFT2]++;
hg_map65_gain=(float)pow(10.0,-hg.map65_gain_db/20.);
if(show_map65)sc[SC_SHOW_MAP65]++;
resume_thread(THREAD_SCREEN);
awake_screen();
if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
                                                          sc[SC_AFC_CURSOR]++;
}

void init_hires_graph(void)
{
if (read_modepar_file(GRAPHTYPE_HG) == 0)
  {
hg_default:;  
// Make the default window for the high resolution graph.
  hg.xleft=0;
  hg.xright=(int)(0.4F*(float)screen_width);
  hg.ytop=wg.ybottom+2;
  hg.ybottom=hg.ytop+(int)(0.3F*(float)screen_height);
  if(hg.ybottom>screen_height-text_height-1)
    {
    hg.ybottom=screen_height-text_height-1;
    hg.ytop=hg.ybottom-6*text_height;
    }
  hg.stupid_bln_mode=1;
  hg.clever_bln_mode=1;
  hg.stupid_bln_factor=5;
  hg.clever_bln_factor=10;
  hg.stupid_bln_limit=(unsigned int)((float)timf2_noise_floor*hg.stupid_bln_factor);
  hg.clever_bln_limit=(unsigned int)((float)timf2_noise_floor*hg.clever_bln_factor);
  hg.blanker_ston_fft1=30;
  hg.blanker_ston_fft2=30;
  hg.timf2_oscilloscope=0;
  hg.timf2_oscilloscope_lines=0;
  hg.timf2_oscilloscope_hold=0;
  hg.timf2_oscilloscope_wk_gain=1;
  hg.timf2_oscilloscope_st_gain=1;
  hg.spek_zero=500;
  hg.spek_gain=500;
  hg.check = HG_VERNR;
  hg.map65_gain_db=0;
  hg.map65_strong=1;
  hg.sellim_par1=2;  //0,1,2
  hg.sellim_par2=0;  //0,1
  hg.sellim_par3=0;  //0,1
  hg.sellim_par4=0;  //0,1
  hg.sellim_par5=0;  //0,1,2
  hg.sellim_par6=0;  // 0,1
  if(mg.scale_type == MG_SCALE_STON)
    {
    hg.sellim_par7=1;  // 0,1
    }
  else
    {      
    hg.sellim_par7=0;  // 0,1
    }
  hg.sellim_par8=0;  // 0,1
  }
if(hg.check != HG_VERNR)goto hg_default;  
if(hg.xleft < 0 || hg.xright > screen_last_xpixel)goto hg_default;
if(hg.xright-hg.xleft<4*text_width)goto hg_default;
if(hg.ytop < 0 || hg.ybottom > screen_height-1)goto hg_default;
if(hg.ybottom-hg.ytop < 4*text_height+5)goto hg_default;
if( (fft1_calibrate_flag&CALAMP)!=CALAMP)hg.clever_bln_mode=0;
if(hg.stupid_bln_mode <0 || hg.stupid_bln_mode >2)goto hg_default;
if(hg.clever_bln_mode <0 || hg.clever_bln_mode >2)goto hg_default;
if(hg.stupid_bln_factor<0.7 || hg.stupid_bln_factor> 10000)goto hg_default;
if(hg.clever_bln_factor<0.7 || hg.clever_bln_factor> 10000)goto hg_default;
if(hg.sellim_par1 < 0 || hg.sellim_par1 > 2)goto hg_default;
if(hg.sellim_par2 < 0 || hg.sellim_par2 > 1)goto hg_default;
if(hg.sellim_par3 < 0 || hg.sellim_par3 > 1)goto hg_default;
if(hg.sellim_par4 < 0 || hg.sellim_par4 > 1)goto hg_default;
if(hg.sellim_par5 < 0 || hg.sellim_par5 > 2)goto hg_default;
if(hg.sellim_par6 < 0 || hg.sellim_par6 > 1)goto hg_default;
if(hg.sellim_par7 < 0 || hg.sellim_par7 > 1)goto hg_default;
if(hg.sellim_par8 < 0 || hg.sellim_par8 > 1)goto hg_default;


if(hg.map65_gain_db < 0)hg.map65_gain_db=0;
if(hg.map65_gain_db > 15)hg.map65_gain_db=15;
hg.map65_strong&=1;
if(hg.spek_avgnum<1)hg.spek_avgnum=1;
if(hg.spek_avgnum>fft2n_mask)hg.spek_avgnum=fft2n_mask;
if(hg.blanker_ston_fft1<1.5)hg.blanker_ston_fft1=20;
if(hg.blanker_ston_fft2<5)hg.blanker_ston_fft2=30;
hires_graph_scro=no_of_scro;
if(ui.operator_skil != OPERATOR_SKIL_EXPERT)hg.timf2_oscilloscope=0;
make_hires_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

