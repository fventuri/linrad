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
#include "thrdef.h"
#include "vernr.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define MG_MIN_WIDTH 5.5*text_width
#define MG_MIN_HEIGHT 8*text_height

void make_meter_graph(int clear_old);
int meter_graph_scro;
int mg_old_y1;
int mg_old_y2;
int mg_old_x1;
int mg_old_x2;
#define MG_MIN_SCALESEP 1.2


void change_meter_cal(void)
{
if(mg.scale_type == MG_SCALE_DBM)mg.cal_dbm=numinput_float_data;
if(mg.scale_type == MG_SCALE_STON)mg.cal_ston=numinput_float_data;
if(mg.scale_type == MG_SCALE_SUNITS)mg.cal_s_units=numinput_float_data;
if(mg.scale_type == MG_SCALE_DBHZ)mg.cal_dbhz=numinput_float_data;
make_meter_graph(TRUE);
}

void change_meter_ston_sigshift(void)
{
mg.cal_ston_sigshift=numinput_float_data;
make_meter_graph(TRUE);
}


void help_on_meter_graph(void)
{
int mmg_no;
int event_no;
// Nothing is selected in the data area.
mmg_no=312;
for(event_no=0; event_no<MAX_MGBUTT; event_no++)
  {
  if( mgbutt[event_no].x1 <= mouse_x && 
      mgbutt[event_no].x2 >= mouse_x &&      
      mgbutt[event_no].y1 <= mouse_y && 
      mgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case MG_TOP:
      case MG_BOTTOM:
      case MG_LEFT:
      case MG_RIGHT:
      mmg_no=102;
      break;

      case MG_CHANGE_CAL:
      mmg_no=90;
      break;

      case MG_CHANGE_TYPE:
      mmg_no=91;
      break;

      case MG_INCREASE_AVGN:
      mmg_no=92;
      break;
  
      case MG_DECREASE_AVGN:
      mmg_no=93;
      break;
  
      case MG_INCREASE_GAIN:
      mmg_no=94;
      break;
  
      case MG_DECREASE_GAIN:
      mmg_no=95;
      break;
  
      case MG_INCREASE_YREF:
      mmg_no=96;
      break;
  
      case MG_DECREASE_YREF:
      mmg_no=97;
      break;
   
      case MG_CHANGE_TRACKS:
      mmg_no=98;
      break;

      case MG_SCALE_STON_SIGSHIFT:
      mmg_no=341;
      break;  
      }
    }  
  }
help_message(mmg_no);
}  

void mouse_continue_meter_graph(void)
{
int j, ypix;
switch (mouse_active_flag-1)
  {
  case MG_TOP:
  if(mg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&mg,0);
    mg.ytop=mouse_y;
    j=mg.ybottom-MG_MIN_HEIGHT;
    if(j<0)j=0;
    if(mg.ytop > j)mg.ytop=j;      
    if(mg_old_y1 > mg.ytop)mg_old_y1=mg.ytop;
    graph_borders((void*)&mg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case MG_BOTTOM:
  if(mg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&mg,0);
    mg.ybottom=mouse_y;
    j=mg.ytop+MG_MIN_HEIGHT;
    if(j>=screen_height)j=screen_height-1;
    if(mg.ybottom < j)mg.ybottom=j;      
    if(mg_old_y2 < mg.ybottom)mg_old_y2=mg.ybottom;
    graph_borders((void*)&mg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case MG_LEFT:
  if(mg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&mg,0);
    mg.xleft=mouse_x;
    j=mg.xright-MG_MIN_WIDTH;
    if(j<0)j=0;
    if(mg.xleft > j)mg.xleft=j;      
    if(mg_old_x1 > mg.xleft)mg_old_x1=mg.xleft;
    graph_borders((void*)&mg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case MG_RIGHT:
  if(mg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&mg,0);
    mg.xright=mouse_x;
    j=mg.xleft+MG_MIN_WIDTH;
    if(j >= screen_width)j=screen_width-1;
    if(mg.xright < j)mg.xright=j;      
    if(mg_old_x2 < mg.xright)mg_old_x2=mg.xright;
    graph_borders((void*)&mg,15);
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
  case MG_CHANGE_CAL:
  mouse_active_flag=1;
  numinput_xpix=mgbutt[MG_CHANGE_CAL].x1+text_width/2-1;
  numinput_ypix=mgbutt[MG_CHANGE_CAL].y1+2;
  numinput_chars=8;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=change_meter_cal;
  return;

  case MG_CHANGE_TYPE:
  mg.scale_type++;
  if(genparm[SECOND_FFT_ENABLE] == 0 && 
                             mg.scale_type == MG_SCALE_STON)mg.scale_type++;
  if(mg.scale_type >= MG_SCALE_MAX)mg.scale_type=0;
  break;

  case MG_INCREASE_AVGN:
  mg.avgnum<<=1;
  timf2_blockpower_px=-1;
  break;
  
  case MG_DECREASE_AVGN:
  mg.avgnum>>=1;
  if(mg.avgnum ==0)mg.avgnum=1;
  timf2_blockpower_px=-1;
  break;
  
  case MG_INCREASE_GAIN:
  ypix=mg.yzero+mg.ygain*mg_midscale;;
  mg.ygain*=1.3;
  mg.yzero=ypix-mg.ygain*mg_midscale;;
  break;
  
  case MG_DECREASE_GAIN:
  ypix=mg.yzero+mg.ygain*mg_midscale;;
  mg.ygain/=1.4;
  mg.yzero=ypix-mg.ygain*mg_midscale;;
  break;

  
  case MG_INCREASE_YREF:
  mg.yzero-=0.09*(mg.ybottom-mg.ytop);
  break;
  
  case MG_DECREASE_YREF:
  mg.yzero+=0.1*(mg.ybottom-mg.ytop);
  break;
   
  case MG_SCALE_STON_SIGSHIFT:
  if(mg.scale_type == MG_SCALE_STON)
    {  
    mouse_active_flag=1;
    numinput_xpix=mgbutt[MG_SCALE_STON_SIGSHIFT].x1+text_width/2-1;
    numinput_ypix=mgbutt[MG_SCALE_STON_SIGSHIFT].y1+2;
    numinput_chars=6;    
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
    par_from_keyboard_routine=change_meter_ston_sigshift;
    return;
    }
  break;
      
  case MG_CHANGE_TRACKS:
  mg.tracks++;
  if(mg.tracks >= 3)mg.tracks=0;
  break;

  default:
  break;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
make_meter_graph(TRUE);
mouse_active_flag=0;
}


void mg_signal_stonavg(void)
{
if(rightpressed == BUTTON_IDLE)
  {
  if(leftpressed == BUTTON_RELEASED)
    {
    mg_stonavg_mouse=mouse_x;
    sc[SC_METER_MOUSE]++;
    leftpressed=BUTTON_IDLE;
    mouse_active_flag=0;
    }
  }
else
  {
  if(rightpressed == BUTTON_RELEASED)
    {
    rightpressed=BUTTON_IDLE;
    mouse_active_flag=0;
    }
  }
}

void mouse_on_meter_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_MGBUTT; event_no++)
  {
  if( mgbutt[event_no].x1 <= mouse_x && 
      mgbutt[event_no].x2 >= mouse_x &&      
      mgbutt[event_no].y1 <= mouse_y && 
      mgbutt[event_no].y2 >= mouse_y) 
    {
    mg_old_y1=mg.ytop;
    mg_old_y2=mg.ybottom;
    mg_old_x1=mg.xleft;
    mg_old_x2=mg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_meter_graph;
    return;
    }
  }
// Not button or border.
if(mouse_x >= mg_first_xpixel && mouse_x < mg_last_xpixel)
  {
  mg_stonavg_x1=mouse_x;
  lir_line(mouse_x,mg.ytop,mouse_x,mg.ybottom,12);
  current_mouse_activity=mg_signal_stonavg;
  }
else
  {   
  current_mouse_activity=mouse_nothing;
  }
mouse_active_flag=1;
}

void check_mg(int n);

void make_meter_graph(int clear_old)
{
int i, ypix, sunit_flag;
double t1, t2;
if(clear_old)
  {
  pause_thread(THREAD_SCREEN);
  hide_mouse(mg_old_x1,mg_old_x2,mg_old_y1,mg_old_y2);
  lir_fillbox(mg_old_x1,mg_old_y1,mg_old_x2-mg_old_x1+1,
                                                    mg_old_y2-mg_old_y1+1,0);
  }
current_graph_minh=MG_MIN_HEIGHT;
current_graph_minw=MG_MIN_WIDTH;
check_graph_placement((void*)(&mg));
clear_button(mgbutt, MAX_MGBUTT);
scro[meter_graph_scro].no=METER_GRAPH;
scro[meter_graph_scro].x1=mg.xleft;
scro[meter_graph_scro].x2=mg.xright;
scro[meter_graph_scro].y1=mg.ytop;
scro[meter_graph_scro].y2=mg.ybottom;
// Each border line is treated as a button.
// That is for the mouse to get hold of them so the window can be moved.
mgbutt[MG_LEFT].x1=mg.xleft;
mgbutt[MG_LEFT].x2=mg.xleft+2;
mgbutt[MG_LEFT].y1=mg.ytop;
mgbutt[MG_LEFT].y2=mg.ybottom;
mgbutt[MG_RIGHT].x1=mg.xright-2;
mgbutt[MG_RIGHT].x2=mg.xright;
mgbutt[MG_RIGHT].y1=mg.ytop;
mgbutt[MG_RIGHT].y2=mg.ybottom;
mgbutt[MG_TOP].x1=mg.xleft;
mgbutt[MG_TOP].x2=mg.xright;
mgbutt[MG_TOP].y1=mg.ytop;
mgbutt[MG_TOP].y2=mg.ytop+2;
mgbutt[MG_BOTTOM].x1=mg.xleft;
mgbutt[MG_BOTTOM].x2=mg.xright;
mgbutt[MG_BOTTOM].y1=mg.ybottom-2;
mgbutt[MG_BOTTOM].y2=mg.ybottom;
mg_first_xpixel=mg.xleft+7*text_width;
mg_last_xpixel=mg.xright-1;
if(mg.xright-mg.xleft < 12.5*text_width)
  {
  mg_bar=TRUE;
  }
else
  {
  if(mg_bar)
    {
    mg_pa=0;
    mg_px=0;
    mgw_p=mg_first_xpixel;
    }
  mg_bar=FALSE;
  make_button(mg.xright-3*text_width,mg.ybottom-text_height/2-2,
                                                  mgbutt,MG_INCREASE_AVGN,27); 
  make_button(mg.xright-5*text_width,mg.ybottom-text_height/2-2,
                                                  mgbutt,MG_DECREASE_AVGN,26);
  make_button(mg.xleft+text_width,mg.ytop+text_height/2+2,
                                                  mgbutt,MG_INCREASE_GAIN,24); 
  make_button(mg.xleft+text_width,mg.ybottom-text_height/2-2,
                                                  mgbutt,MG_DECREASE_GAIN,25);
  make_button(mg.xright-text_width,mg.ybottom-text_height/2-2,
                                                  mgbutt,MG_INCREASE_YREF,25); 
  make_button(mg.xright-text_width,mg.ytop+text_height/2+2,
                                                  mgbutt,MG_DECREASE_YREF,24); 
  make_button(mg.xright-3*text_width,mg.ytop+text_height/2+2,
                                                 mgbutt,MG_CHANGE_TRACKS,' ');
  mgbutt[MG_CHANGE_CAL].y1=mg.ytop+1;
  mgbutt[MG_CHANGE_CAL].y2=mg.ytop+text_height+2;
  if(mg.scale_type != MG_SCALE_DB)
    {
    mgbutt[MG_CHANGE_CAL].x1=mg.xleft+2*text_width;
    mgbutt[MG_CHANGE_CAL].x2=mg.xleft+10.5*text_width;
    }
  else
    {
    mgbutt[MG_CHANGE_CAL].x1=screen_width+1;
    mgbutt[MG_CHANGE_CAL].x2=screen_width+1;
    }
  if(mg.scale_type == MG_SCALE_STON)
    {
    mgbutt[MG_SCALE_STON_SIGSHIFT].x1=mgbutt[MG_CHANGE_CAL].x2+text_width/2;
    mgbutt[MG_SCALE_STON_SIGSHIFT].x2=
                           mgbutt[MG_SCALE_STON_SIGSHIFT].x1+6.5*text_width;
    mgbutt[MG_SCALE_STON_SIGSHIFT].y1=mgbutt[MG_CHANGE_CAL].y1;
    mgbutt[MG_SCALE_STON_SIGSHIFT].y2=mgbutt[MG_CHANGE_CAL].y2;
    }
  mgbutt[MG_CHANGE_TYPE].x1=mg.xleft+2*text_width;
  mgbutt[MG_CHANGE_TYPE].y1=mg.ybottom-3-text_height;
  mgbutt[MG_CHANGE_TYPE].y2=mg.ybottom-2;
  switch (mg.scale_type)
    {
    case MG_SCALE_DB:
    mgbutt[MG_CHANGE_TYPE].x2=mg.xleft+4.5*text_width;
    break;
  
    case MG_SCALE_STON:
    hg.sellim_par7=1;
    sc[SC_SHOW_SELLIM]++;
    mgbutt[MG_CHANGE_TYPE].x2=mg.xleft+5.5*text_width;
    break;
    
    case MG_SCALE_DBM:
    mgbutt[MG_CHANGE_TYPE].x2=mg.xleft+5.5*text_width;
    break;

    case MG_SCALE_SUNITS:
    mgbutt[MG_CHANGE_TYPE].x2=mg.xleft+3.5*text_width;
    break;

    case MG_SCALE_DBHZ:
    mgbutt[MG_CHANGE_TYPE].x2=mg.xleft+7.5*text_width;
    break;
    
    default:
    lirerr(532907);
    break;
    }
  }
// ************************************************
// Make an Y-scale in dB, dBm or S-units.
// ************************************************
// The separation between scale lines, has to be at least
// MG_MIN_SCALESEP*text_height. In dB this corresponds to mg_dbstep.
mg_dbstep=10*MG_MIN_SCALESEP*text_height/mg.ygain;
if(mg.scale_type == MG_SCALE_SUNITS)
  {  
  i=1+mg_dbstep/6;
  mg_dbstep=6*i;
  }
else
  {
  adjust_scale(&mg_dbstep);
  }
// Make mg_dbscale_start the dB value for the first pixel in the graph (mg_rms_meter[]=0)
// Then make it a multiple of mg_dbstep and make sure we have enough
// space to write text. 
mg_scale_offset=0;
if(mg.scale_type == MG_SCALE_DBM)mg_scale_offset=DB2DBM+mg.cal_dbm;
if(mg.scale_type == MG_SCALE_STON)mg_scale_offset=mg.cal_ston;
if(mg.scale_type == MG_SCALE_SUNITS)mg_scale_offset=6+mg.cal_s_units;
if(mg.scale_type == MG_SCALE_DBHZ)mg_scale_offset=DB2DBM+mg.cal_dbhz;
mg_dbscale_start=-10*mg.yzero/mg.ygain-mg_scale_offset;
mg_dbscale_start/=mg_dbstep;
mg_dbscale_start=(int)mg_dbscale_start;
mg_dbscale_start*=mg_dbstep;
for(i=0; i<screen_height; i++)mg_behind_meter[i]=0;
mg_ymin=mg.ybottom-5-text_height;
mg_ymax=mg.ytop+3+text_height;
ypix=mg.yzero+0.1*mg.ygain*(mg_dbscale_start+mg_scale_offset);
mg_midscale=mg_dbscale_start;
t1=mg_dbscale_start;
t2=mg_dbstep;
sunit_flag=0;
mg_bar_x1=mg.xleft+2*text_width;
mg_bar_x2=mg_bar_x1+3*text_width;
mg_bar_y=mg.ybottom-2;
mg_y0=mg.ybottom-1;
while(mg_ymin-ypix > mg_ymax)
  {
  if(ypix >=0)
    {
    if(mg.scale_type == MG_SCALE_SUNITS)
      {
      if(t1 > 54.111 && sunit_flag == 0)
        {
        sunit_flag=1;
        i=1+mg_dbstep/5;
        t2=5*i;
        t1=54+t2;
        ypix=mg.yzero+0.1*mg.ygain*(t1+mg_scale_offset);
        }        
      }
    if(mg_ymin-ypix >  mg_ymax && mg_ymin-ypix < mg_ymin)
      {
      mg_behind_meter[mg_ymin-ypix]=57;
      }
    }
  t1+=t2;
  ypix=mg.yzero+0.1*mg.ygain*(t1+mg_scale_offset);
  }
mg_midscale=0.1*(mg_scale_offset+(mg_midscale+t1-t2)*0.5);
make_modepar_file(GRAPHTYPE_MG);
for(i=0; i<ui.rx_rf_channels*mg_size; i++)
  {
  mg_peak_ypix[i]=mg_ymin;
  mg_rms_ypix[i]=mg_ymin;
  }
sc[SC_MG_REDRAW]++;
if(clear_old) resume_thread(THREAD_SCREEN);
}

void init_meter_graph(void)
{
if (read_modepar_file(GRAPHTYPE_MG) == 0)
  {
mg_default:;  
  mg.xright=screen_width-1;
  mg.xleft=mg.xright-14*text_width;
  mg.ytop=wg.ybottom+1;
  mg.ybottom=mg.ytop+22*text_height;
  mg.scale_type=MG_SCALE_SUNITS;
  mg.avgnum=baseband_sampling_speed*0.05;
  if(mg.avgnum < 1)mg.avgnum=1;
  make_power_of_two(&mg.avgnum);
  mg.ygain=(1.01+MG_MIN_SCALESEP)*text_height;
  mg.yzero=-26;
  mg.cal_dbm=0;
  mg.cal_ston=0;
  mg.cal_ston_sigshift=0;
  mg.cal_dbhz=0;
  mg.cal_s_units=0;
  mg.tracks=0;
  mg.check=MG_VERNR;
  }
if(mg.ygain<=0)goto mg_default;  
if(mg.scale_type < 0 || mg.scale_type >= MG_SCALE_MAX)goto mg_default;
if(genparm[SECOND_FFT_ENABLE] == 0 && 
                              mg.scale_type == MG_SCALE_STON)goto mg_default;
if(mg.avgnum < 1 || mg.avgnum > baseband_sampling_speed*3600)goto mg_default;
if(mg.ygain < 0.01 || mg.ygain > 100000)goto mg_default;
if(fabs(mg.cal_dbm) > 1000)goto mg_default;
if(fabs(mg.cal_ston) > 1000)goto mg_default;
if(fabs(mg.cal_dbhz) > 1000)goto mg_default;
if(fabs(mg.cal_ston_sigshift) > 1000)goto mg_default;
if(fabs(mg.cal_s_units) > 100)goto mg_default;
if(mg.check != MG_VERNR)goto mg_default; 
if(mg.tracks < 0 || mg.tracks > 3)goto mg_default;
make_meter_graph(FALSE);
mg_pa=0;
mg_px=0;
mgw_p=mg_first_xpixel;
baseb_pm=baseb_pa;
mg_flag=1;
if(genparm[BG_METER_FILE_AVERAGES] != 0)
  {
  if(mg_meter_file == NULL)mg_meter_file=fopen("s-meter.txt","w");
  if(mg_meter_file == NULL)lirerr(1206);
  mg_meter_file_cnt=0;
  mg_meter_file_sum=0.;
  mg_meter_file_tot=0;
  mg_meter_start_time=current_time();
  }
else
  {
  if(mg_meter_file != NULL)fclose(mg_meter_file);
  mg_meter_file=NULL;
  }
}

void manage_meter_graph(void)
{
if(fft1_correlation_flag == 2)return;
sc[SC_UPDATE_METER_GRAPH]=sd[SC_UPDATE_METER_GRAPH];
if(cg.meter_graph_on == 1)
  {
  if(mg_flag==0)
    {
    mg_clear_flag=TRUE;
    init_meter_graph();
    make_meter_graph(FALSE);
    }
  }
else
  {
  if(mg_flag==1)
    {
    mg_flag=0;
    lir_fillbox(mg.xleft,mg.ytop,mg.xright-mg.xleft+1,mg.ybottom-mg.ytop+1,0);
    clear_button(mgbutt, MAX_MGBUTT);
    }
  }  
}
