// Copyright (c) <2024> <Leif Asbrink>
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

#include <string.h>

#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "screendef.h"
#include "thrdef.h"
#include "vernr.h"
#include "seldef.h"
#include "sigdef.h"
#include "fft3def.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif
int vg_old_y1;
int vg_old_y2;
int vg_old_x1;
int vg_old_x2;
int vgf_old_y1;
int vgf_old_y2;
int vgf_old_x1;
int vgf_old_x2;
int allan_graph_scro;
int allanfreq_graph_scro;
int reinit_vg;
float ampfreq_i;
int fix_allanfreq;
void fix_ampfreq_time(void);
int allan_line;
char allan_filename[160];

void allansave_start(void)
{
int i;
char *allanwr_screencopy;
char s[160];
#if (ALLANFILE_CH1 == TRUE ||\
     ALLANFILE_CH2 == TRUE ||\
     ALLANFILE_DIFF == TRUE)  
#else
  return;
#endif
pause_thread(THREAD_SCREEN);
allanwr_screencopy=malloc(DISKSAVE_SCREEN_SIZE);
if(allanwr_screencopy == NULL)
  {
  lirerr(1031);
  return;
  }
allan_write_flag = 0;
lir_getbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,(size_t*)allanwr_screencopy);
lir_fillbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,10);
sprintf(s,"Enter name for ASCII files. Tau=%fs",vgf.time);
lir_text(0,0,s);
lir_text(0,1,"ENTER to skip.");
i=lir_get_filename(2,2,s);          
if(kill_all_flag || i==0) return;
allan_line=2;
#if ALLANFILE_CH1 == TRUE
complete_filename(i, s, "_ch1.txt", ALLANDIR, allan_filename);
allan_file_ch1 = fopen( allan_filename, "wb");
if(allan_file_ch1 == NULL)
  {
  could_not_create(allan_filename, allan_line);
  allan_line++;
  }
else
  {
  allan_write_flag = 1;
  }
#endif
#if ALLANFILE_CH2 == TRUE
complete_filename(i, s, "_ch2.txt", ALLANDIR, allan_filename);
allan_file_ch2 = fopen( allan_filename, "wb");
if(allan_file_ch2 == NULL)
  {
  could_not_create(allan_filename, allan_line);
  allan_line++;
  }
else
  {
  allan_write_flag = 1;
  }
#endif
#if ALLANFILE_DIFF == TRUE
complete_filename(i, s, "_diff.txt", ALLANDIR, allan_filename);
allan_file_diff = fopen( allan_filename, "wb");
if(allan_file_diff == NULL)
  {
  could_not_create(allan_filename, allan_line);
  allan_line++;
  }
else
  {
  allan_write_flag = 1;
  }
#endif
lir_putbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,(size_t*)allanwr_screencopy);
free(allanwr_screencopy);
resume_thread(THREAD_SCREEN);
}

void allansave_stop(void)
{
if(allan_file_ch1)fclose(allan_file_ch1);
if(allan_file_ch2)fclose(allan_file_ch2);
if(allan_file_diff)fclose(allan_file_diff);
allan_write_flag = 0;
}

void do_allan(void)
{
int i, k, ia, iaold, ib, ic, id;
double dt1, dt2, dt3, dt4;
double freqinv;
double disksave_time;
ia=baseb_px;
iaold=(ia-1+baseband_size)&baseband_mask;
if(fabs(vg_phase[2*iaold  ]) > 10000. ||
   fabs(vg_phase[2*iaold+1]) > 10000.)
  {
// Reset pointers. Collection of Allan deviation can go
// on for several days. Not likely however...
  dt1=atan2(d_baseb_carrier[4*ia+1], d_baseb_carrier[4*ia  ])/PI_L;
  dt2=atan2(d_baseb_carrier[4*ia+3], d_baseb_carrier[4*ia+2])/PI_L;
  vg_phase[2*ia  ]=dt1;
  vg_phase[2*ia+1]=dt2;
  for(i=0; i<vg_no_of_tau; i++)
    {
    vg_start_pointer[i]=ia;
    vg_n[i]=0;
    }
  ia=(ia+1)&baseband_mask;
  baseb_px=ia;
  return;
  }
disksave_time=current_time();
while(((baseb_pa-ia+baseband_size)&baseband_mask) > 1)
  {
  dt1=atan2(d_baseb_carrier[4*ia+1], d_baseb_carrier[4*ia  ])/PI_L;
  dt2=atan2(d_baseb_carrier[4*ia+3], d_baseb_carrier[4*ia+2])/PI_L;
  dt1-=round(dt1-vg_phase[2*iaold]);
  dt2-=round(dt2-vg_phase[2*iaold+1]);
  vg_phase[2*ia  ]=dt1;  
  vg_phase[2*ia+1]=dt2;
  for(i=0; i<vg_no_of_tau; i++)
    {
    if(vg_n[i] == 3*vg_tau[i])
      {
      freqinv=0.5*(double)baseband_sampling_speed/(double)vg_tau[i];
// Data now spans at least three times the desired tau. 
      ib=(ia-vg_tau[i]+baseband_size)&baseband_mask;
      ic=(ib-vg_tau[i]+baseband_size)&baseband_mask;
      id=(ic-vg_tau[i]+baseband_size)&baseband_mask;
      if(i == ampfreq_i)
        {
        if(vgf_n == vgf_tau)
          {
          vg_basebfreq1=(vg_phase[2*ib  ]-dt1)*freqinv;
          vg_basebfreq2=(vg_phase[2*ib+1]-dt2)*freqinv;
          vg_interchannel_phase=vg_phase[2*ib  ]-vg_phase[2*ib+1];
          vg_interchannel_phase-=round(vg_interchannel_phase);
          if(vg_interchannel_phase > 0.5)vg_interchannel_phase-=1.0;
          if(vg_interchannel_phase < -0.5)vg_interchannel_phase+=1.0;
          vg_interchannel_phase*=360;
          vgf_freq[2*vgf_pa  ]=vg_basebfreq1;
          vgf_freq[2*vgf_pa+1]=vg_basebfreq2;
          vgf_pa=(vgf_pa+1)&vgf_mask;
          k=(vgf_pa-vgf_px+vgf_size)&vgf_mask;
          if(k >= vgf_xpixels)
            {
            k=vgf_xpixels-1;
            vgf_px=(vgf_pa-k+vgf_size)&vgf_mask;
            }
          vgf_n=0;
          if(allan_write_flag == 1)
            {
#if ALLANFILE_CH1 == TRUE
            fprintf(allan_file_ch1,"\n%.16e",vg_basebfreq1+1000.*hwfreq);
#endif
#if ALLANFILE_CH2 == TRUE
            fprintf(allan_file_ch2,"\n%.16e",vg_basebfreq2+1000.*hwfreq);
#endif
#if ALLANFILE_DIFF == TRUE
            fprintf(allan_file_diff,"\n%.16e",
                                  vg_basebfreq2-vg_basebfreq1+1000.*hwfreq);
#endif 
            if(recent_time-disksave_time > 2.F)
              {
              fflush(NULL);
              disksave_time=current_time();
              }
            }
          }
        else
          {  
          vgf_n++;
          }
        }    
// Allan variance for phase/frequency
// Get the phase shift for the three latest regions and compute by how 
// much the phase shifts differ.
      dt3=freqinv*(dt1-2*vg_phase[2*ib  ]+vg_phase[2*ic  ]);
      dt4=freqinv*(dt2-2*vg_phase[2*ib+1]+vg_phase[2*ic+1]);
      vg_asum[2*i  ]+=2*dt3*dt3;
      vg_asum[2*i+1]+=2*dt4*dt4;
      vg_acorrsum[i]+=2*dt3*dt4;
// Hadamard for phase/frequency
      freqinv/=sqrt(3.0);
      dt3=freqinv*(dt1-3*(vg_phase[2*ib  ]-vg_phase[2*ic  ])-vg_phase[2*id  ]);
      dt4=freqinv*(dt2-3*(vg_phase[2*ib+1]-vg_phase[2*ic+1])-vg_phase[2*id+1]);
      vg_hsum[2*i  ]+=2*dt3*dt3;
      vg_hsum[2*i+1]+=2*dt4*dt4;
      vg_hcorrsum[i]+=2*dt3*dt4;
      vg_sumno[i]++;
// Control overlaping with vg_n[i]
//  vg_n[i]=vg_tau[i]+1; Gives old, not overlapping Allan variance
//  vg_n[i] unchanged gives fully overlapped variances
//  Anything between gives partly overlapped variances 
      }
    else
      {
      vg_n[i]++;
      }  
    }
  iaold=ia;
  ia=(ia+1)&baseband_mask;
  }
baseb_px=ia;
if(fix_allanfreq != 0)
  {
  lir_sched_yield();
  while(sc[SC_VG_REDRAW] != sd[SC_VG_REDRAW])lir_sleep(100);
  sc[SC_VGF_REDRAW]++;
  while(sc[SC_VGF_REDRAW] != sd[SC_VGF_REDRAW])lir_sleep(100);
  }
sc[SC_VGF_UPDATE]++;
sc[SC_VG_UPDATE]++;
}

void new_vg_mintau(void)
{
float t1;
t1=vg.mintau;
vg.mintau=numinput_float_data;
if(vg.mintau < 0.01)vg.mintau=0.01;
if(vg.mintau > 0.1*vg.maxtau)vg.mintau=0.1*vg.maxtau;
if(vg.mintau != t1)
  {
  make_modepar_file(GRAPHTYPE_VG);
  }
sc[SC_VG_REDRAW]++;
fix_allanfreq=1;
}

void new_vg_maxtau(void)
{
float t1;
t1=vg.maxtau;
vg.maxtau=numinput_float_data;
if(vg.maxtau < 10*vg.mintau)vg.maxtau=10*vg.mintau;
if(vg.maxtau > genparm[BASEBAND_STORAGE_TIME]/4)
              vg.maxtau=genparm[BASEBAND_STORAGE_TIME]/4;
if(vg.maxtau != t1)
  {
  make_modepar_file(GRAPHTYPE_VG);
  }
sc[SC_VG_REDRAW]++;
}

void new_vg_points_per_decade(void)
{
int i;
i=vg.points_per_decade;
vg.points_per_decade=numinput_int_data;
if(vg.points_per_decade < 4)vg.points_per_decade=4;
if(vg.points_per_decade > 99)vg.points_per_decade=99;
if(vg.points_per_decade != i)
  {
  make_modepar_file(GRAPHTYPE_VG);
  }
sc[SC_VG_REDRAW]++;
fix_allanfreq=1;
}

void new_vg_ymin(void)
{
int i, j;
i=vg.ymax;
j=vg.ymin;
vg.ymin=numinput_int_data;
if(vg.ymin < 5)vg.ymin=5;
if(vg.ymin > 16)vg.ymin=16;
if(vg.ymin - vg.ymax > 6)vg.ymax=vg.ymin-6;
if(vg.ymin != j || vg.ymax != i)
  {
  make_modepar_file(GRAPHTYPE_VG);
  }
sc[SC_VG_REDRAW]++;
}

void new_vg_ymax(void)
{
int i, j;
i=vg.ymax;
j=vg.ymin;
vg.ymax=numinput_int_data;
if(vg.ymax < 4)vg.ymax=4;
if(vg.ymax > 15)vg.ymax=15;
if(vg.ymin - vg.ymax > 6)vg.ymin=vg.ymax+6;
if(vg.ymax != i || vg.ymin != j)
  {
  make_modepar_file(GRAPHTYPE_VG);
  }
sc[SC_VG_REDRAW]++;
}

void help_on_allan_graph(void)
{
int msg_no;
int event_no;
msg_no=366;
for(event_no=0; event_no<MAX_VGBUTT; event_no++)
  {
  if( vgbutt[event_no].x1 <= mouse_x && 
      vgbutt[event_no].x2 >= mouse_x &&      
      vgbutt[event_no].y1 <= mouse_y && 
      vgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case VG_TOP:
      case VG_BOTTOM:
      case VG_LEFT:
      case VG_RIGHT:
      msg_no=100;
      break;

      case VG_NEW_POINTS_PER_DECADE:
      msg_no=360;
      break;
      
      case VG_NEW_MINTAU:
      msg_no=361;
      break;

      case VG_NEW_MAXTAU:
      msg_no=362;
      break;

      case VG_NEW_YMIN:
      msg_no=363;
      break;

      case VG_NEW_YMAX:
      msg_no=364;
      break;
      
      case VG_NEW_CLEAR:
      msg_no=367;
      break;
      
      case VG_NEW_MODE:
      msg_no=368;
      break;
      }
    }  
  }
help_message(msg_no);      
}

void mouse_continue_allan_graph(void)
{
reinit_vg=FALSE;
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Look in freq_control.c how to move a fixed size window.
  case VG_TOP:
  graph_borders((void*)&vg,0);
  vg.ytop=mouse_y;
  graph_borders((void*)&vg,15);
  break;

  case VG_BOTTOM:
  graph_borders((void*)&vg,0);
  vg.ybottom=mouse_y;
  graph_borders((void*)&vg,15);
  break;
  
  case VG_LEFT:
  graph_borders((void*)&vg,0);
  vg.xleft=mouse_x;
  graph_borders((void*)&vg,15);
  break;
  
  case VG_RIGHT:
  graph_borders((void*)&vg,0);
  vg.xright=mouse_x;
  graph_borders((void*)&vg,15);
  break;
      
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)
  {
finish:;
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  make_allan_graph(TRUE,FALSE);
  }
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case VG_NEW_MINTAU: 
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_MINTAU].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_MINTAU].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_vg_mintau;
  clear_graph_flag=2*ALLAN_GRAPH+1;
  return;

  case VG_NEW_MAXTAU: 
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_MAXTAU].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_MAXTAU].y1+2;
  numinput_chars=5;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_vg_maxtau;
  clear_graph_flag=2*ALLAN_GRAPH+1;
  return;

  case VG_NEW_POINTS_PER_DECADE: 
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_POINTS_PER_DECADE].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_POINTS_PER_DECADE].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_vg_points_per_decade;
  clear_graph_flag=2*ALLAN_GRAPH+1;
  return;

  case VG_NEW_YMIN:
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_YMIN].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_YMIN].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_vg_ymin;
  clear_graph_flag=2*ALLAN_GRAPH;
  return;

  case VG_NEW_YMAX:
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_YMAX].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_YMAX].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_vg_ymax;
  clear_graph_flag=2*ALLAN_GRAPH;
  return;

  case VG_NEW_CLEAR:
  vg.clear^=1;
  goto finish;
  
  case VG_NEW_MODE:
  vg.mode^=1;
  goto finish;

  default:
// This should never happen.    
  lirerr(211053);
  break;
  }  
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_allan_graph(TRUE,TRUE);
}

void mouse_on_allan_graph(void)
{
char s[32];
int i, j, x, x1, event_no;
unsigned char color;
double centerinv, sum;
float time;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_VGBUTT; event_no++)
  {
  if( vgbutt[event_no].x1 <= mouse_x && 
      vgbutt[event_no].x2 >= mouse_x &&      
      vgbutt[event_no].y1 <= mouse_y && 
      vgbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_allan_graph;
    return;
    }
  }
// Not button or border.
// find the nearest tau value.
current_mouse_activity=mouse_nothing;
x=mouse_x;
if(x<vg_first_xpixel)x=vg_first_xpixel;
i=1;
j=vg_tau_to_xpix((double)vg_tau[0]/baseband_sampling_speed);
while(i < vg_no_of_tau)
  {
  if(j > x)break;
  j=vg_tau_to_xpix((double)vg_tau[i]/baseband_sampling_speed);
  i++;
  }
i--;  
if(i != 0)
  { 
  if(j-x > 
      x-vg_tau_to_xpix((double)vg_tau[i-1]/baseband_sampling_speed))
  i--;
  }
time=vg_tau[i]/baseband_sampling_speed;
if(time < 999)
  { 
  sprintf(s,"%4.2f",time);
  }
else
  {
  sprintf(s,"%6.0f",time);  
  }
x1=vgbutt[VG_NEW_YMAX].x2+text_width;
lir_pixwrite(x1,vg.ytop+text_height/2,s);  
centerinv=.000001/(fg.passband_center*sqrt(2.0));
x1+=6*text_width;
if(vg.mode == 0)
  {
  sprintf(s,"%.2e",centerinv*sqrt(vg_asum[2*i]/(2*vg_sumno[i])));
  settextcolor(10);
  lir_pixwrite(x1,vg.ytop+text_height/2,s);  
  x1+=9*text_width;
  sprintf(s,"%.2e",centerinv*sqrt(vg_asum[2*i+1]/(2*vg_sumno[i])));
  settextcolor(55);
  lir_pixwrite(x1,vg.ytop+text_height/2,s);
  if(vg_acorrsum[i] < 0)
    {
    color=3;
    sum=-vg_acorrsum[i];
    }
  else
    {  
    color=14;
    sum=vg_acorrsum[i];
    }
  }
else
  {
  sprintf(s,"%.2e",centerinv*sqrt(vg_hsum[2*i]/(2*vg_sumno[i])));
  settextcolor(10);
  lir_pixwrite(x1,vg.ytop+text_height/2,s);  
  x1+=9*text_width;
  sprintf(s,"%.2e",centerinv*sqrt(vg_hsum[2*i+1]/(2*vg_sumno[i])));
  settextcolor(55);
  lir_pixwrite(x1,vg.ytop+text_height/2,s);
  if(vg_hcorrsum[i] < 0)
    {
    color=3;
    sum=-vg_hcorrsum[i];
    }
  else
    {  
    color=14;
    sum=vg_hcorrsum[i];
    }
  }
sprintf(s,"%.2e",centerinv*sqrt(sum/(2*vg_sumno[i])));
settextcolor(color);
x1+=9*text_width;
lir_pixwrite(x1,vg.ytop+text_height/2,s);
settextcolor(7);
mouse_active_flag=1;
}

void new_vgf_freqgain(void)
{
float t1;
t1=vgf.freqgain;
vgf.freqgain=numinput_float_data;
if(vgf.freqgain != t1)
  {
  make_modepar_file(GRAPHTYPE_VGF);
  sc[SC_VGF_REDRAW]++;
  }
}

void new_vgf_time(void)
{
float t1;
t1=vgf.time;
vgf.time=numinput_float_data;
if(vgf.time < 0.01)vgf.time=0.01;
if(vgf.time > 30)vgf.time=30;
if(vgf.time != t1)
  {
  make_modepar_file(GRAPHTYPE_VGF);
  fix_ampfreq_time();
  sc[SC_VGF_REDRAW]++;
  }
}

void help_on_allanfreq_graph(void)
{
int msg_no;
int event_no;
msg_no=366;
for(event_no=0; event_no<MAX_VGFBUTT; event_no++)
  {
  if( vgfbutt[event_no].x1 <= mouse_x && 
      vgfbutt[event_no].x2 >= mouse_x &&      
      vgfbutt[event_no].y1 <= mouse_y && 
      vgfbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case VGF_TOP:
      case VGF_BOTTOM:
      case VGF_LEFT:
      case VGF_RIGHT:
      msg_no=100;
      break;

      case VGF_NEW_FREQGAIN:
      msg_no=370;
      break;
      
      case VGF_NEW_AMPLGAIN:
      msg_no=371;
      break;
      
      case VGF_NEW_TIME:
      msg_no=372;
      break;

      case VGF_NEW_CENTER_TRACES:
      msg_no=373;
      break;
      }
    }  
  }
help_message(msg_no);      
}

void mouse_continue_allanfreq_graph(void)
{
reinit_vg=FALSE;
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Look in freq_control.c how to move a fixed size window.
  case VGF_TOP:
  graph_borders((void*)&vgf,0);
  vgf.ytop=mouse_y;
  graph_borders((void*)&vgf,15);
  break;

  case VGF_BOTTOM:
  graph_borders((void*)&vgf,0);
  vgf.ybottom=mouse_y;
  graph_borders((void*)&vgf,15);
  break;
  
  case VGF_LEFT:
  graph_borders((void*)&vgf,0);
  vgf.xleft=mouse_x;
  graph_borders((void*)&vgf,15);
  break;
  
  case VGF_RIGHT:
  graph_borders((void*)&vgf,0);
  vgf.xright=mouse_x;
  graph_borders((void*)&vgf,15);
  break;
      
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)
  {
finish:;
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  make_allanfreq_graph(TRUE, FALSE);
  }
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case VGF_NEW_TIME: 
  mouse_active_flag=1;
  numinput_xpix=vgfbutt[VGF_NEW_TIME].x1+5*text_width/2-1;
  numinput_ypix=vgfbutt[VGF_NEW_TIME].y1+2;
  numinput_chars=6;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_vgf_time;
  clear_graph_flag=2*ALLANFREQ_GRAPH+1;
  return;

  case VGF_NEW_FREQGAIN:
  mouse_active_flag=1;
  numinput_xpix=vgfbutt[VGF_NEW_FREQGAIN].x1+9*text_width/2-1;
  numinput_ypix=vgfbutt[VGF_NEW_FREQGAIN].y1+2;
  numinput_chars=6;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_vgf_freqgain;
  return;
      
  case VGF_NEW_CENTER_TRACES:
  vgf_center_traces=TRUE;
  goto finish;
  break;
  
  default:
// This should never happen.    
  lirerr(211053);
  break;
  }  
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_allanfreq_graph(TRUE,TRUE);
}

void mouse_on_allanfreq_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_VGFBUTT; event_no++)
  {
  if( vgfbutt[event_no].x1 <= mouse_x && 
      vgfbutt[event_no].x2 >= mouse_x &&      
      vgfbutt[event_no].y1 <= mouse_y && 
      vgfbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_allanfreq_graph;
    return;
    }
  }
// Not button or border.
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}

void fix_ampfreq_time(void)
{
int i;
float t1, t2;
vgf_tau=(rint)(baseband_sampling_speed*vgf.time);
t1=(float)(vgf_tau/vg_tau[0]);
t2=(float)(vgf_tau/vg_tau[1]);
i=1;
while(t1 > 1 && i < vg_no_of_tau)
  {
  t2=t1;
  t1=(float)vgf_tau/vg_tau[i];
  i++; 
  }
i--;    
if(i>0 && i< vg_no_of_tau-1)
  {
  if(1.0/t1 > t2)
    {
    i--;
    }
  vgf_tau=vg_tau[i];  
  ampfreq_i=i;
  vgf.time=vgf_tau/baseband_sampling_speed;  
  }
vgf_n=0;
}

void make_pix_per_decade(void)
{
int i;
// find out how many pixels below 1.0 to place 0.9, 0.8,0.7,...
vg_ypix_per_decade=(float)(vg_yb-vg_yt)/(vg.ymin - vg.ymax);
i=9;
while(i > 1)
  {
  vg_decimal_ypixel[i]=(int)((vg_ypix_per_decade)*(1.F-log10((double)i))+0.5F);
  i--;
  }
// find out how many powers of 10 we want. Fractional number may occur.
vg_xpix_per_decade=(float)(vg_last_xpixel-vg_first_xpixel)/
                     log10(vg.maxtau/vg.mintau);
i=9;
while(i >= 0)
  {
  vg_decimal_xpixel[i]=(int)((float)(vg_xpix_per_decade)*(1.F-log10((double)i))+0.5F);
  i--;
  }
}

void make_allan_graph(int clear_old, int restart)
{
int i, k, x1,x2,iy1,y2;
float t1, t2, t3;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(vg_old_x1,vg_old_x2,vg_old_y1,vg_old_y2);
  lir_fillbox(vg_old_x1,vg_old_y1,vg_old_x2-vg_old_x1+1,
                                                    vg_old_y2-vg_old_y1+1,0);
  }
clear_graph_flag=-1;
vg_old_y1=vg.ytop;
vg_old_y2=vg.ybottom;
vg_old_x1=vg.xleft;
vg_old_x2=vg.xright;
current_graph_minh=10*text_height;
current_graph_minw=98*text_width;
check_graph_placement((void*)(&vg));
vg_yt=vg.ytop+2*text_height;
vg_yb=vg.ybottom-3*text_height/2;
vg_first_xpixel=vg.xleft+6*text_width;
vg_last_xpixel=vg.xright-2;
scro[allan_graph_scro].no=ALLAN_GRAPH;
scro[allan_graph_scro].x1=vg.xleft;
scro[allan_graph_scro].x2=vg.xright;
scro[allan_graph_scro].y1=vg.ytop;
scro[allan_graph_scro].y2=vg.ybottom;
vgbutt[VG_LEFT].x1=vg.xleft;
vgbutt[VG_LEFT].x2=vg.xleft+2;
vgbutt[VG_LEFT].y1=vg.ytop;
vgbutt[VG_LEFT].y2=vg.ybottom;
vgbutt[VG_RIGHT].x1=vg.xright-2;
vgbutt[VG_RIGHT].x2=vg.xright;
vgbutt[VG_RIGHT].y1=vg.ytop;
vgbutt[VG_RIGHT].y2=vg.ybottom;
vgbutt[VG_TOP].x1=vg.xleft;
vgbutt[VG_TOP].x2=vg.xright;
vgbutt[VG_TOP].y1=vg.ytop;
vgbutt[VG_TOP].y2=vg.ytop+2;
vgbutt[VG_BOTTOM].x1=vg.xleft;
vgbutt[VG_BOTTOM].x2=vg.xright;
vgbutt[VG_BOTTOM].y1=vg.ybottom-2;
vgbutt[VG_BOTTOM].y2=vg.ybottom;
settextcolor(7);
iy1=vg.ytop+text_height/4;
y2=iy1+text_height;
x1=vg.xleft+3*text_width;
x2=x1+19*text_width/2;
vgbutt[VG_NEW_MINTAU].x1=x1;
vgbutt[VG_NEW_MINTAU].x2=x2;
vgbutt[VG_NEW_MINTAU].y1=iy1;
vgbutt[VG_NEW_MINTAU].y2=y2;
x1=x2+text_width;
x2=x1+21*text_width/2;
vgbutt[VG_NEW_MAXTAU].x1=x1;
vgbutt[VG_NEW_MAXTAU].x2=x2;
vgbutt[VG_NEW_MAXTAU].y1=iy1;
vgbutt[VG_NEW_MAXTAU].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_POINTS_PER_DECADE].x1=x1;
vgbutt[VG_NEW_POINTS_PER_DECADE].x2=x2;
vgbutt[VG_NEW_POINTS_PER_DECADE].y1=iy1;
vgbutt[VG_NEW_POINTS_PER_DECADE].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_YMIN].x1=x1;
vgbutt[VG_NEW_YMIN].x2=x2;
vgbutt[VG_NEW_YMIN].y1=iy1;
vgbutt[VG_NEW_YMIN].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_YMAX].x1=x1;
vgbutt[VG_NEW_YMAX].x2=x2;
vgbutt[VG_NEW_YMAX].y1=iy1;
vgbutt[VG_NEW_YMAX].y2=y2;
x2=vg.xright-text_width/2;
x1=x2-text_width/2;
iy1=vg.ytop+text_height-2;
make_button(x1,iy1,vgbutt, VG_NEW_MODE,vg_modes[vg.mode]);
x2=x1-text_height/2;
x1=x2-3*text_width/2;
make_button(x1,iy1,vgbutt, VG_NEW_CLEAR,vg_clears[vg.clear]);
if(!restart)
  {
  make_pix_per_decade();
  fix_ampfreq_time();
  make_modepar_file(GRAPHTYPE_VG);
  make_modepar_file(GRAPHTYPE_VGF);
  sc[SC_VG_REDRAW]++;
  resume_thread(THREAD_SCREEN);
  return;
  }
if(allan_handle != NULL)
  {
  allan_handle=chk_free(allan_handle);
  }
init_memalloc(allanmem, MAX_SIGANAL_ARRAYS);
mem( 1,&vg_phase, baseband_size*2*sizeof(double),0);
// Find out at what tau values we want to store sigma.
// Allocate some extra just in case
i=(int)(float)(vg.points_per_decade)*(1+log10(vg.maxtau/vg.mintau));
mem( 2,&vg_start_pointer, i*sizeof(int),0);
mem( 3,&vg_tau,i*sizeof(int),0);
mem( 4,&vg_asum,2*i*sizeof(double),0);
mem( 5,&vg_acorrsum,i*sizeof(double),0);
mem( 6,&vg_n,i*sizeof(int),0);
mem( 7,&vg_sumno,i*sizeof(int),0);
mem( 8,&vg_y1pix,i*sizeof(short int),0);
mem( 9,&vg_y2pix,i*sizeof(short int),0);
mem(10,&vg_ycpix,2*i*sizeof(short int),0);
mem(11,&vg_decimal_xpixel,10*sizeof(short int),0);
mem(12,&vg_decimal_ypixel,10*sizeof(short int),0);
mem(13,&vg_hsum,2*i*sizeof(double),0);
mem(14,&vg_hcorrsum,i*sizeof(double),0);


allan_totmem=memalloc(&allan_handle,"allan");
t1=pow(10,1.0F/(float)(vg.points_per_decade));
t2=vg.mintau*(float)baseband_sampling_speed;
t3=vg.maxtau*(float)baseband_sampling_speed;
k=0;
while(t2 < t3 && k < i)
  {
  vg_tau[k]=(int)(t2+0.5F);
  t2*=t1;
  k++;
  }
vg_no_of_tau=k;
vg_basebfreq1=0;
vg_basebfreq2=0;
make_pix_per_decade();
for(i=0; i<2*baseband_size; i++)vg_phase[i]=BIGDOUBLE;
for(i=0; i<2*vg_no_of_tau; i++)vg_asum[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_acorrsum[i]=0;
for(i=0; i<2*vg_no_of_tau; i++)vg_hsum[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_hcorrsum[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_sumno[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_y1pix[i]=vg_yb;
for(i=0; i<vg_no_of_tau; i++)vg_y2pix[i]=vg_yb;
for(i=0; i<2*vg_no_of_tau; i++)vg_ycpix[i]=vg_yb;
sc[SC_VG_REDRAW]++;
resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_VG);
vg_siz=baseband_sampling_speed/2;
vg_reset_time=current_time();
baseb_reset_counter++;
}

void init_allan_graph(void)
{
// Set initial values in code or by reading
// a file of your own.
if (read_modepar_file(GRAPHTYPE_VG) == 0)
  {
vg_default:
  vg.xleft=screen_width/2;
  vg.xright=vg.xleft+50*text_width;
  vg.ytop=screen_height/2;
  vg.ybottom=vg.ytop+8.5*text_height;
  vg.mintau=.1;
  vg.maxtau=100;
  vg.points_per_decade=10;
  vg.ymax=9;
  vg.ymin=11;
  vg.clear=0;
  vg.mode=0;
  vg.check=VG_VERNR;
  }
if(vg.mintau < .0099F ||
   vg.mintau > 10.5 ||
   vg.maxtau > 10000. ||
   vg.maxtau < 10* vg.mintau ||
   vg.points_per_decade < 4 ||
   vg.points_per_decade > 99 ||
   vg.ymin > 16 ||
   vg.ymin < 5 ||
   vg.ymax < 4 ||
   vg.ymax > 15 ||
   vg.ymin-vg.ymax > 6 ||
   vg.clear < 0 ||
   vg.clear > 1 ||
   vg.mode < 0 ||
   vg.mode > 1 ||
   vg.check != VG_VERNR)goto vg_default;
allan_graph_scro=no_of_scro;
make_allan_graph(FALSE,TRUE);
no_of_scro++;
vg_flag=1;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

void make_allanfreq_graph(int clear_old, int reset)
{
int x1,x2,iy1,y2;
if(clear_old)
  {
  hide_mouse(vgf_old_x1,vgf_old_x2,vgf_old_y1,vgf_old_y2);
  lir_fillbox(vgf_old_x1,vgf_old_y1,vgf_old_x2-vgf_old_x1+1,
                                                    vgf_old_y2-vgf_old_y1+1,0);
  }
pause_thread(THREAD_SCREEN);
vgf_old_y1=vgf.ytop;
vgf_old_y2=vgf.ybottom;
vgf_old_x1=vgf.xleft;
vgf_old_x2=vgf.xright;
current_graph_minh=10*text_height;
current_graph_minw=75*text_width;
check_graph_placement((void*)(&vgf));
vgf_yt=vgf.ytop+2*text_height;
vgf_yb=vgf.ybottom-3*text_height/2;
vgf_first_xpixel=vgf.xleft+7*text_width;
vgf_last_xpixel=vgf.xright-2-6*text_width;
vgf_xpixels=1+vgf_last_xpixel-vgf_first_xpixel;
scro[allanfreq_graph_scro].no=ALLANFREQ_GRAPH;
scro[allanfreq_graph_scro].x1=vgf.xleft;
scro[allanfreq_graph_scro].x2=vgf.xright;
scro[allanfreq_graph_scro].y1=vgf.ytop;
scro[allanfreq_graph_scro].y2=vgf.ybottom;
vgfbutt[VGF_LEFT].x1=vgf.xleft;
vgfbutt[VGF_LEFT].x2=vgf.xleft+2;
vgfbutt[VGF_LEFT].y1=vgf.ytop;
vgfbutt[VGF_LEFT].y2=vgf.ybottom;
vgfbutt[VGF_RIGHT].x1=vgf.xright-2;
vgfbutt[VGF_RIGHT].x2=vgf.xright;
vgfbutt[VGF_RIGHT].y1=vgf.ytop;
vgfbutt[VGF_RIGHT].y2=vgf.ybottom;
vgfbutt[VGF_TOP].x1=vgf.xleft;
vgfbutt[VGF_TOP].x2=vgf.xright;
vgfbutt[VGF_TOP].y1=vgf.ytop;
vgfbutt[VGF_TOP].y2=vgf.ytop+2;
vgfbutt[VGF_BOTTOM].x1=vgf.xleft;
vgfbutt[VGF_BOTTOM].x2=vgf.xright;
vgfbutt[VGF_BOTTOM].y1=vgf.ybottom-2;
vgfbutt[VGF_BOTTOM].y2=vgf.ybottom;
settextcolor(7);
iy1=vgf.ytop+text_height/4;
y2=iy1+text_height;
x1=vgf.xleft+3*text_width;
x2=x1+15*text_width/2;
vgfbutt[VGF_NEW_TIME].x1=x1;
vgfbutt[VGF_NEW_TIME].x2=x2;
vgfbutt[VGF_NEW_TIME].y1=iy1;
vgfbutt[VGF_NEW_TIME].y2=y2;
x1=x2+text_width;
x2=x1+23*text_width/2;
vgfbutt[VGF_NEW_FREQGAIN].x1=x1;
vgfbutt[VGF_NEW_FREQGAIN].x2=x2;
vgfbutt[VGF_NEW_FREQGAIN].y1=iy1;
vgfbutt[VGF_NEW_FREQGAIN].y2=y2;
x1=x2+text_width;
x2=x1+31*text_width/2;
vgfbutt[VGF_NEW_AMPLGAIN].x1=x1;
vgfbutt[VGF_NEW_AMPLGAIN].x2=x2;
vgfbutt[VGF_NEW_AMPLGAIN].y1=iy1;
vgfbutt[VGF_NEW_AMPLGAIN].y2=y2;
x1=vgf.xright-3*text_width/2;
iy1=vgf.ytop+text_height-2;
make_button(x1,iy1,vgfbutt,VGF_NEW_CENTER_TRACES,42);
vgf_freq_xpix=x1-33*text_width;

fix_ampfreq_time();
make_modepar_file(GRAPHTYPE_VGF);
if(!reset)
  {
  sc[SC_VGF_REDRAW]++;
  resume_thread(THREAD_SCREEN);
  return;
  }
vgf_pa=0;
vgf_px=0;
vgf_average_freq1=0;
vgf_average_freq2=0;
vgf_mid_freq=BIGDOUBLE;
fix_allanfreq=0;
vgf_center_traces=FALSE;
vgf_n=0;
sc[SC_VGF_REDRAW]++;
resume_thread(THREAD_SCREEN);
}

void init_allanfreq_graph(void)
{
int k;
if (read_modepar_file(GRAPHTYPE_VGF) == 0)
  {
vgf_default:
  vgf.xleft=screen_width/2;
  vgf.xright=vgf.xleft+50*text_width;
  vgf.ytop=3*screen_height/4;
  vgf.ybottom=vgf.ytop+8.5*text_height;
  vgf.freqgain=0.01;
  vgf.time=1.0;
  vgf.check=VGF_VERNR;
  }
if(vgf.time < 0.01)vgf.time=0.01;
if(vgf.time > 30)vgf.time=30;
if(vg.maxtau > genparm[BASEBAND_STORAGE_TIME]/4)
              vg.maxtau=genparm[BASEBAND_STORAGE_TIME]/4;
if(vg.mintau < .0099F ||
   vgf.check != VGF_VERNR)goto vgf_default;
allanfreq_graph_scro=no_of_scro;
vgf_size=screen_width;
make_power_of_two(&vgf_size);
vgf_mask=vgf_size-1;
if(vgf_freq == NULL)vgf_freq=malloc(2*vgf_size*sizeof(float));
for(k=0; k<2*vgf_size; k++)vgf_freq[k]=0;
make_allanfreq_graph(FALSE,TRUE);
no_of_scro++;
vgf_flag=1;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

