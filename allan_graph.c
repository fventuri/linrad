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
int allan_graph_scro;
int reinit_vg;

void do_allan(void)
{
int i, ia, iaold, ib, ic, id;
double dt1, dt2, dt3, dt4, dt5, dt6;
double freqinv,totamp1, totamp2;
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
  vg_amplitude[2*ia  ]=sqrt(d_baseb_carrier[4*ia  ]*d_baseb_carrier[4*ia  ]+
                            d_baseb_carrier[4*ia+1]*d_baseb_carrier[4*ia+1]);
  vg_amplitude[2*ia+1]=sqrt(d_baseb_carrier[4*ia+2]*d_baseb_carrier[4*ia+2]+
                            d_baseb_carrier[4*ia+3]*d_baseb_carrier[4*ia+3]);
  for(i=0; i<vg_no_of_tau; i++)
    {
    vg_start_pointer[i]=ia;
    vg_n[i]=0;
    }
  ia=(ia+1)&baseband_mask;
  baseb_px=ia;
  return;
  }
while(((baseb_pa-ia+baseband_size)&baseband_mask) > 1)
  {
  dt1=atan2(d_baseb_carrier[4*ia+1], d_baseb_carrier[4*ia  ])/PI_L;
  dt2=atan2(d_baseb_carrier[4*ia+3], d_baseb_carrier[4*ia+2])/PI_L;
  dt1-=round(dt1-vg_phase[2*iaold]);
  dt2-=round(dt2-vg_phase[2*iaold+1]);
  vg_phase[2*ia  ]=dt1;  
  vg_phase[2*ia+1]=dt2;
  dt5=sqrt(d_baseb_carrier[4*ia  ]*d_baseb_carrier[4*ia  ]+
           d_baseb_carrier[4*ia+1]*d_baseb_carrier[4*ia+1]);
  dt6=sqrt(d_baseb_carrier[4*ia+2]*d_baseb_carrier[4*ia+2]+
           d_baseb_carrier[4*ia+3]*d_baseb_carrier[4*ia+3]);
  vg_amplitude[2*ia  ]=dt5;
  vg_amplitude[2*ia+1]=dt6;
  for(i=0; i<vg_no_of_tau; i++)
    {
//if(i==5)fprintf(dmp,"\n%d  %e  %e %f  %f",ia,dt5,dt6,dt1,dt2);

    if(vg_n[i] == 3*vg_tau[i])
      {
// Data now spans three times the desired tau. Get the phase shift
// Allan
// for the three regions and compute by how much the phase shifts differ.    
      ib=(ia-vg_tau[i]+baseband_size)&baseband_mask;
      ic=(ib-vg_tau[i]+baseband_size)&baseband_mask;
      id=(ic-vg_tau[i]+baseband_size)&baseband_mask;
      dt3=-2*vg_phase[2*ib]+dt1+vg_phase[2*ic];
      freqinv=0.5*(double)baseband_sampling_speed/(double)vg_tau[i];
      if(i == vg_no_of_tau/4)
        {
        vg_basebfreq=(vg_phase[2*ib]-dt1)*freqinv;
        }
      dt3*=freqinv;
      vg_asum[2*i  ]+=dt3*dt3;
      dt4=-2*vg_phase[2*ib+1]+dt2+vg_phase[2*ic+1];
      dt4*=freqinv;
      vg_asum[2*i+1]+=dt4*dt4;
      vg_acorrsum[i]+=dt3*dt4;
// Hadamard
      dt3=-3*(vg_phase[2*ib]-vg_phase[2*ic])+dt1-vg_phase[2*id];
      freqinv/=sqrt(3.0);
      dt3*=freqinv;
      vg_hsum[2*i  ]+=dt3*dt3;
      dt4=-3*(vg_phase[2*ib+1]-vg_phase[2*ic+1])+dt2-vg_phase[2*id+1];
      dt4*=freqinv;
      vg_hsum[2*i+1]+=dt4*dt4;
      vg_hcorrsum[i]+=dt3*dt4;
// Compute for amplitudes
      dt3=-2*vg_amplitude[2*ib  ]+dt5+vg_amplitude[2*ic  ];
      dt4=-2*vg_amplitude[2*ib+1]+dt6+vg_amplitude[2*ic+1];
      totamp1=sqrt((vg_amplitude[2*ib]*vg_amplitude[2*ib]+
                    vg_amplitude[2*ic]*vg_amplitude[2*ic]+dt5*dt5)/2);
      totamp2=sqrt((vg_amplitude[2*ib+1]*vg_amplitude[2*ib+1]+
                    vg_amplitude[2*ic+1]*vg_amplitude[2*ic+1]+dt6*dt6)/2);
      dt3/=totamp1;
      vg_asum_ampl[2*i  ]+=dt3*dt3;
      dt4/=totamp2;
      vg_asum_ampl[2*i+1]+=dt4*dt4;
      vg_acorrsum_ampl[i]+=dt3*dt4;
      dt3=-3*(vg_amplitude[2*ib  ]-vg_amplitude[2*ic  ])+dt5-
                                                     vg_amplitude[2*id];
      dt4=-3*(vg_amplitude[2*ib+1]-vg_amplitude[2*ic+1])+dt6-
                                                     vg_amplitude[2*id+1];
      totamp1=sqrt((vg_amplitude[2*ib]*vg_amplitude[2*ib]+
                    vg_amplitude[2*ic]*vg_amplitude[2*ic]+
                    vg_amplitude[2*id]*vg_amplitude[2*id]+dt5*dt5)/6);
      totamp2=sqrt((vg_amplitude[2*ib+1]*vg_amplitude[2*ib+1]+
                    vg_amplitude[2*ic+1]*vg_amplitude[2*ic+1]+
                    vg_amplitude[2*id+1]*vg_amplitude[2*id+1]+
                    dt6*dt6)/6);
      dt3/=totamp1;
      vg_hsum_ampl[2*i  ]+=dt3*dt3;
      dt4/=totamp2;
      vg_hsum_ampl[2*i+1]+=dt4*dt4;
      vg_hcorrsum_ampl[i]+=dt3*dt4;
      vg_sumno[i]++;
// Control overlaping with vg_n[i]
//  vg_n[i]=vg_tau[i]+1; Gives old, not overlapping Allan variance
//  vg_n[i] unchanged gives fully overlapped variances
//  Anything between gives partly overlapped variances 


/*
if(i==0)
{
fprintf(dmp,"\n ampl (%e %e %e %e)",dt5,
     vg_amplitude[2*ib], vg_amplitude[2*ic],vg_amplitude[2*id]);
fprintf(dmp,"\n ampl (%e %e %e %e)",dt6,
     vg_amplitude[2*ib+1], vg_amplitude[2*ic+1],vg_amplitude[2*id+1]);
fprintf(dmp,"\ntotamp %e  %e",totamp1, totamp2);
fprintf(dmp,"\ndt3 %e  dt4 %e",dt3,dt4);

fprintf(dmp,"\navg %e  %e",
     sqrt((vg_hsum_ampl[2*i  ])/(1+vg_sumno[i])),
     sqrt((vg_hsum_ampl[2*i+1])/(1+vg_sumno[i])));
}
*/

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
}

void new_vg_maxtau(void)
{
float t1;
t1=vg.maxtau;
vg.maxtau=numinput_float_data;
if(vg.maxtau < 10*vg.mintau)vg.maxtau=10*vg.mintau;
if(vg.maxtau > 10000)vg.maxtau=10000;
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
      
      case VG_NEW_TYPE:
      msg_no=369;
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
//clear_graph_flag=2*ALLAN_GRAPH;
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
  return;

  case VG_NEW_YMAX:
  mouse_active_flag=1;
  numinput_xpix=vgbutt[VG_NEW_YMAX].x1+11*text_width/2-1;
  numinput_ypix=vgbutt[VG_NEW_YMAX].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_vg_ymax;
  return;

  case VG_NEW_CLEAR:
  vg.clear^=1;
  goto finish;
  
  case VG_NEW_MODE:
  vg.mode^=1;
  goto finish;
    
  case VG_NEW_TYPE:
  vg.type^=1;
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
int event_no;
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
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}

void make_pix_per_decade(void)
{

int i;
// find out how many pixels below 1.0 to place 0.9, 0.8,0.7,...
vg_ypix_per_decade=(float)(vg_yb-vg_yt)/(vg.ymin - vg.ymax);i=9;
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
int i, k, x1,x2,y1,y2;
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
current_graph_minw=75*text_width;
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
y1=vg.ytop+text_height/4;
y2=y1+text_height;
x1=vg.xleft+3*text_width;
x2=x1+19*text_width/2;
vgbutt[VG_NEW_MINTAU].x1=x1;
vgbutt[VG_NEW_MINTAU].x2=x2;
vgbutt[VG_NEW_MINTAU].y1=y1;
vgbutt[VG_NEW_MINTAU].y2=y2;
x1=x2+text_width;
x2=x1+21*text_width/2;
vgbutt[VG_NEW_MAXTAU].x1=x1;
vgbutt[VG_NEW_MAXTAU].x2=x2;
vgbutt[VG_NEW_MAXTAU].y1=y1;
vgbutt[VG_NEW_MAXTAU].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_POINTS_PER_DECADE].x1=x1;
vgbutt[VG_NEW_POINTS_PER_DECADE].x2=x2;
vgbutt[VG_NEW_POINTS_PER_DECADE].y1=y1;
vgbutt[VG_NEW_POINTS_PER_DECADE].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_YMIN].x1=x1;
vgbutt[VG_NEW_YMIN].x2=x2;
vgbutt[VG_NEW_YMIN].y1=y1;
vgbutt[VG_NEW_YMIN].y2=y2;
x1=x2+text_width;
x2=x1+15*text_width/2;
vgbutt[VG_NEW_YMAX].x1=x1;
vgbutt[VG_NEW_YMAX].x2=x2;
vgbutt[VG_NEW_YMAX].y1=y1;
vgbutt[VG_NEW_YMAX].y2=y2;
vg_freq_xpix=x2+2*text_width;
x2=vg.xright-text_width/2;
x1=x2-text_width/2;
y1=vg.ytop+text_height-2;
make_button(x1,y1,vgbutt, VG_NEW_TYPE,vg_types[vg.type]);
x2=x1-text_height/2;
x1=x2-3*text_width/2;
make_button(x1,y1,vgbutt, VG_NEW_MODE,vg_modes[vg.mode]);
x2=x1-text_height/2;
x1=x2-3*text_width/2;
make_button(x1,y1,vgbutt, VG_NEW_CLEAR,vg_clears[vg.clear]);
if(!restart && vg.type != 2)
  {
  make_pix_per_decade();
  make_modepar_file(GRAPHTYPE_VG);
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
i=(int)(float)vg.points_per_decade*(vg.maxtau+1)/vg.mintau;
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
mem(15,&vg_amplitude, baseband_size*2*sizeof(double),0);
mem(16,&vg_asum_ampl,2*i*sizeof(double),0);
mem(17,&vg_acorrsum_ampl,i*sizeof(double),0);
mem(18,&vg_hsum_ampl,2*i*sizeof(double),0);
mem(19,&vg_hcorrsum_ampl,i*sizeof(double),0);


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
vg_basebfreq=0;
make_pix_per_decade();
for(i=0; i<2*baseband_size; i++)vg_phase[i]=BIGDOUBLE;
for(i=0; i<2*baseband_size; i++)vg_amplitude[i]=0;
for(i=0; i<2*vg_no_of_tau; i++)vg_asum[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_acorrsum[i]=0;
for(i=0; i<2*vg_no_of_tau; i++)vg_hsum[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_hcorrsum[i]=0;
for(i=0; i<2*vg_no_of_tau; i++)vg_asum_ampl[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_acorrsum_ampl[i]=0;
for(i=0; i<2*vg_no_of_tau; i++)vg_hsum_ampl[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_hcorrsum_ampl[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_sumno[i]=0;
for(i=0; i<vg_no_of_tau; i++)vg_y1pix[i]=vg_yb;
for(i=0; i<vg_no_of_tau; i++)vg_y2pix[i]=vg_yb;
for(i=0; i<2*vg_no_of_tau; i++)vg_ycpix[i]=vg_yb;

resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_VG);
sc[SC_VG_REDRAW]++;
vg_siz=baseband_sampling_speed*2;
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
  vg.type=0;
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
   vg.type < 0 ||
   vg.type > 2 ||
   vg.check != VG_VERNR)goto vg_default;
allan_graph_scro=no_of_scro;
make_allan_graph(FALSE,TRUE);
no_of_scro++;
vg_flag=1;

if(no_of_scro >= MAX_SCRO)lirerr(89);
}
