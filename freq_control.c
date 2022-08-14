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
#include "screendef.h"
#include "hwaredef.h"
#include "fft1def.h"
#include "thrdef.h"
#include "sdrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

int freq_graph_scro;
int fg_old_y1;
int fg_old_y2;
int fg_old_x1;
int fg_old_x2;
void make_freq_graph(int clear_old);

void check_fg_borders(void)
{
current_graph_minh=FG_VSIZ;
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)current_graph_minh+=text_height+4;
current_graph_minw=FG_HSIZ;
check_graph_placement((void*)(&fg));
set_graph_minwidth((void*)(&fg));
}

void check_filtercorr_direction(void)
{
int i,j,k,mm;
float t1,t2;
if( fft1_filtercorr_direction*fft1_direction < 0 )
  {
  fft1_filtercorr_direction = fft1_direction;
  mm=2*ui.rx_rf_channels;
  k=fft1_size;
  for(i=0; i<fft1_size/2; i++)
    {
    k--;
    for(j=0; j<mm; j+=2)
      {
      t1=fft1_filtercorr[mm*i+j];
      t2=fft1_filtercorr[mm*i+j+1];
      fft1_filtercorr[mm*i+j]=fft1_filtercorr[mm*k+j+1];
      fft1_filtercorr[mm*i+j+1]=fft1_filtercorr[mm*k+j];
      fft1_filtercorr[mm*k+j]=t2;
      fft1_filtercorr[mm*k+j+1]=t1;
      }
    t1=fft1_desired[i];
    fft1_desired[i]=fft1_desired[k];
    fft1_desired[k]=t1;
    }
  for(i=0; i<fft1_size; i++)fft1_slowsum[i]=0;
  for(i=0; i<fft1_sumsq_bufsize; i++)fft1_sumsq[i]=0;
  }  
if(fg.passband_center != 0 && ( (ui.rx_input_mode&IQ_DATA) != 0 || ui.rx_input_mode == 0 ))
  {
  frequency_scale_offset=fg.passband_center*10L-(double)timf1_sampling_speed/200000L;
  }
else
  {
  frequency_scale_offset=0;
  }
if( (ui.converter_mode & CONVERTER_USE) != 0)
  {
  if( (ui.converter_mode & CONVERTER_LO_BELOW) != 0)
    {
    if( (ui.converter_mode & CONVERTER_UP) == 0)
      {
      frequency_scale_offset+=10*converter_offset_mhz;
      }
    else
      {
      frequency_scale_offset-=10*converter_offset_mhz;
      }
    }  
  else  
    {
    frequency_scale_offset=10*converter_offset_mhz-frequency_scale_offset-timf1_sampling_speed/100000;
    }
  }
frequency_scale_offset+=0.00001*rx_hware_fqshift;
rx_passband_center_mhz=0.1*frequency_scale_offset+timf1_sampling_speed/2000000;
frequency_scale_offset_hz=100000*frequency_scale_offset;
}


void show_hardware_gain(void)
{
char s[80];
sprintf(s,"%3d dB",fg.gain);
lir_pixwrite(fg.xleft+2.5*text_width,fg.ybottom-text_height,s);
}

void copy_txfreq_to_rx(void)
{
double dt1;
if(use_tx == 0)return;
fg.passband_center=tg.freq;
set_hardware_rx_frequency();
dt1=1000000*tg.freq-100000*frequency_scale_offset;
make_new_signal(0, dt1);
sc[SC_SHOW_CENTER_FQ]++;
make_modepar_file(GRAPHTYPE_FG);
}

void new_center_frequency(void)
{
double dt1;
if(numinput_double_data < 0)goto errinp;
numinput_double_data*=10000L;
numinput_double_data=rint(numinput_double_data);
numinput_double_data=numinput_double_data/10000L;
if( (ui.converter_mode & CONVERTER_USE) != 0)
  {
  if( (ui.converter_mode & CONVERTER_LO_BELOW) != 0)
    {
    if( (ui.converter_mode & CONVERTER_UP) == 0)
      {
      dt1=numinput_double_data-converter_offset_mhz;
      }
    else  
      {
      dt1=numinput_double_data+converter_offset_mhz;
      }
    }
  else
    {
    dt1=converter_offset_mhz-numinput_double_data;
    }
  }
else
  {
  dt1=numinput_double_data;
  }  
if(dt1 < 0)goto errinp;
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  if(dt1*1000000+0.5 >= PERSEUS_SAMPLING_CLOCK/2)
    {
errinp:;
    sc[SC_SHOW_CENTER_FQ]++;
    return;
    }
  }
fg.passband_center=dt1;
set_hardware_rx_frequency();
sc[SC_SHOW_CENTER_FQ]++;
make_modepar_file(GRAPHTYPE_FG);
}

void new_freq_step(void)
{
if(numinput_double_data >= 0.0001 && numinput_double_data <= 10.1)
  {
  fg.passband_increment=numinput_double_data;
  make_modepar_file(GRAPHTYPE_FG);
  }
sc[SC_SHOW_CENTER_FQ]++;
}

void new_rx_gain_value(void)
{
fg.gain=numinput_int_data;
set_hardware_rx_gain();
pause_screen_and_hide_mouse();
show_hardware_gain();
resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_FG);
}

void help_on_freq_graph(void)
{
int msg_no;
int event_no;
if(mouse_y <= fg.yborder)
  {
  msg_no=84;
  }
else
  {  
  msg_no=87;
  }
for(event_no=0; event_no<MAX_FGBUTT; event_no++)
  {
  if( fgbutt[event_no].x1 <= mouse_x && 
      fgbutt[event_no].x2 >= mouse_x &&      
      fgbutt[event_no].y1 <= mouse_y && 
      fgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case FG_TOP:
      case FG_BOTTOM:
      case FG_LEFT:
      case FG_RIGHT:
      msg_no=101;
      break;

      case FG_INCREASE_FQ: 
      msg_no=83;
      break;
  
      case FG_DECREASE_FQ:
      msg_no=82;
      break;

      case FG_INCREASE_GAIN: 
      msg_no=85;
      break;
  
      case FG_DECREASE_GAIN:
      msg_no=86;
      break;
      }
    }  
  }
help_message(msg_no);
}




void mouse_continue_freq_graph(void)
{
int i, j;
switch (mouse_active_flag-1)
  {
  case FG_TOP:
  if(fg.ytop!=mouse_y)goto fgm;
  break;

  case FG_BOTTOM:
  if(fg.ybottom!=mouse_y)goto fgm;
  break;

  case FG_LEFT:
  if(fg.xleft!=mouse_x)goto fgm;
  break;

  case FG_RIGHT:
  if(fg.xright==mouse_x)break;
fgm:;
  pause_screen_and_hide_mouse();
  dual_graph_borders((void*)&fg,0);
  if(fg_oldx==-10000)
    {
    fg_oldx=mouse_x;
    fg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-fg_oldx;
    j=mouse_y-fg_oldy;  
    fg_oldx=mouse_x;
    fg_oldy=mouse_y;
    fg.ytop+=j;
    fg.ybottom+=j;      
    fg.xleft+=i;
    fg.xright+=i;
    check_fg_borders();
    fg.yborder=(fg.ytop+fg.ybottom)>>1;
    }
  dual_graph_borders((void*)&fg,15);
  resume_thread(THREAD_SCREEN);
  break;
    
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
if(ui.network_flag != 2)
  {
  switch (mouse_active_flag-1)
    {
    case FG_INCREASE_FQ: 
    if(hware_flag == 0)
      {
      fg.passband_center+=fg.passband_increment;
      set_hardware_rx_frequency();
      }
    break;
  
    case FG_DECREASE_FQ:
    if(hware_flag == 0)
      {
      fg.passband_center-=fg.passband_increment;
      set_hardware_rx_frequency();
      }
    break;

    case FG_INCREASE_GAIN: 
    if(hware_flag == 0)
      {
      fg.gain+=fg.gain_increment;
      set_hardware_rx_gain();
      }
    break;
  
    case FG_DECREASE_GAIN:
    if(hware_flag == 0)
      {
      fg.gain-=fg.gain_increment;
      set_hardware_rx_gain();
      }
    break;

    default:
    lirerr(872);
    break;
    }
  }  
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_freq_graph(TRUE);
fg_oldx=-10000;
}

void mouse_on_freq_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
numinput_flag=0;
for(event_no=0; event_no<MAX_FGBUTT; event_no++)
  {
  if( fgbutt[event_no].x1 <= mouse_x && 
      fgbutt[event_no].x2 >= mouse_x &&      
      fgbutt[event_no].y1 <= mouse_y && 
      fgbutt[event_no].y2 >= mouse_y) 
    {
    fg_old_y1=fg.ytop;
    fg_old_y2=fg.ybottom;
    fg_old_x1=fg.xleft;
    fg_old_x2=fg.xright;
    if(rightpressed == BUTTON_IDLE)
      {
      mouse_active_flag=1+event_no;
      current_mouse_activity=mouse_continue_freq_graph;
      return;
      }
    else
      {
      current_mouse_activity=mouse_nothing;
      mouse_active_flag=1;
      return;
      }
    }
  }
// Not button or border.
// Prompt the user for a new center frequency from keyboard.
if(ui.network_flag != 2)
  {
  if(hware_flag == 0)
    {
    numinput_xpix=fg.xleft+2.5*text_width;
    if(mouse_y < fg.yborder)
      {
      numinput_ypix=fg.yborder-text_height-1;
      numinput_chars=FREQ_MHZ_DIGITS+FREQ_MHZ_DECIMALS+1;
      erase_numinput_txt();
      numinput_flag=FIXED_DOUBLE_PARM;
      if(rightpressed == BUTTON_IDLE)
        {
        par_from_keyboard_routine=new_center_frequency;
        }
      else
        {
        par_from_keyboard_routine=new_freq_step;
        }
      }
    else
      {
      if(diskread_flag < 2 && rightpressed == BUTTON_IDLE)
        {
        numinput_ypix=fg.ybottom-text_height;
        numinput_chars=4;
        erase_numinput_txt();
        numinput_flag=FIXED_INT_PARM;
        par_from_keyboard_routine=new_rx_gain_value;
        }
      else
        {
        current_mouse_activity=mouse_nothing;
        }
      }
    }  
  else
    {
    current_mouse_activity=mouse_nothing;
    }
  }
else
  {
  current_mouse_activity=mouse_nothing;
  }
mouse_active_flag=1;
}

void make_freq_graph(int clear_old)
{
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(fg_old_x1,fg_old_x2,fg_old_y1,fg_old_y2);
  lir_fillbox(fg_old_x1,fg_old_y1,fg_old_x2-fg_old_x1+1,
                                                    fg_old_y2-fg_old_y1+1,0);
  }
fg_flag=1;
check_fg_borders();
clear_button(fgbutt, MAX_FGBUTT);
hide_mouse(fg.xleft,fg.xright,fg.ytop,fg.ybottom);  
fg.yborder=(fg.ytop+fg.ybottom)>>1;
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  fg.yborder+=text_height/2+2;
  lir_pixwrite(fg.xleft+2, fg.ytop+text_height/2-2,"Center Freq");
  }
scro[freq_graph_scro].no=FREQ_GRAPH;
scro[freq_graph_scro].x1=fg.xleft;
scro[freq_graph_scro].x2=fg.xright;
scro[freq_graph_scro].y1=fg.ytop;
scro[freq_graph_scro].y2=fg.ybottom;
fgbutt[FG_LEFT].x1=fg.xleft;
fgbutt[FG_LEFT].x2=fg.xleft+2;
fgbutt[FG_LEFT].y1=fg.ytop;
fgbutt[FG_LEFT].y2=fg.ybottom;
fgbutt[FG_RIGHT].x1=fg.xright;
fgbutt[FG_RIGHT].x2=fg.xright-2;
fgbutt[FG_RIGHT].y1=fg.ytop;
fgbutt[FG_RIGHT].y2=fg.ybottom;
fgbutt[FG_TOP].x1=fg.xleft;
fgbutt[FG_TOP].x2=fg.xright;
fgbutt[FG_TOP].y1=fg.ytop;
fgbutt[FG_TOP].y2=fg.ytop+2;
fgbutt[FG_BOTTOM].x1=fg.xleft;
fgbutt[FG_BOTTOM].x2=fg.xright;
fgbutt[FG_BOTTOM].y1=fg.ybottom-2;
fgbutt[FG_BOTTOM].y2=fg.ybottom;
// Draw the border lines
dual_graph_borders((void*)&fg,7);
fg_oldx=-10000;
settextcolor(7);
make_button(fg.xleft+text_width,fg.yborder-text_height/2-2,
                                         fgbutt,FG_DECREASE_FQ,25);
make_button(fg.xright-text_width,fg.yborder-text_height/2-2,
                                     fgbutt,FG_INCREASE_FQ,24); 
make_button(fg.xleft+text_width,fg.ybottom-text_height/2-2,
                                         fgbutt,FG_DECREASE_GAIN,25);
make_button(fg.xright-text_width,fg.ybottom-text_height/2-2,
                                     fgbutt,FG_INCREASE_GAIN,24); 
show_hardware_gain();
resume_thread(THREAD_SCREEN);
sc[SC_SHOW_CENTER_FQ]++;
make_modepar_file(GRAPHTYPE_FG);
}

void read_freq_control_data(void)
{
if (read_modepar_file(GRAPHTYPE_FG) == 0)
  {
  fg.xleft=0;
  fg.xright=FG_HSIZ;
  fg.ytop=0.75*screen_height;
  fg.ybottom=fg.ytop+FG_VSIZ;
  switch (ui.rx_addev_no)
    {
    case SDR14_DEVICE_CODE:
    case SDRIQ_DEVICE_CODE:
    case PERSEUS_DEVICE_CODE:
    case SDRIP_DEVICE_CODE:
    case EXCALIBUR_DEVICE_CODE:
    case BLADERF_DEVICE_CODE:
    case OPENHPSDR_DEVICE_CODE:
    case NETAFEDRI_DEVICE_CODE:
    fg.passband_center=7.0;
    fg.passband_direction=1;
    fg.gain=0;
    fg.gain_increment=5;
    fg.passband_increment=0.1;
    break;
    
    case MIRISDR_DEVICE_CODE:
    case SDRPLAY2_DEVICE_CODE:
    case SDRPLAY3_DEVICE_CODE:
    case RTL2832_DEVICE_CODE:
    fg.passband_center=97.0;
    fg.passband_direction=1;
    fg.gain=15;
    fg.gain_increment=5;
    fg.passband_increment=0.5;
    break;
        
    case FDMS1_DEVICE_CODE:
    fg.passband_center=7.0;
    fg.passband_direction=1;
    fg.gain=0;
    fg.gain_increment=20;
    fg.passband_increment=0.1;
    break;
    
    default:
    fg.passband_center=7;
    fg.passband_direction=1;
    fg.gain=0;
    fg.gain_increment=0;
    fg.passband_increment=0.05;
    break;
    }
  make_modepar_file(GRAPHTYPE_FG);
  }
if(diskread_flag == 2)
  {  
  fg.passband_direction=fft1_direction;
  }
else
  {
  fft1_direction=fg.passband_direction;
  }
check_filtercorr_direction();
}

void init_freq_control(void)
{
clear_hware_data();
freq_graph_scro=no_of_scro;
make_freq_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
set_hardware_rx_frequency();
}
