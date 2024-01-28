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
int sg_old_y1;
int sg_old_y2;
int sg_old_x1;
int sg_old_x2;
int siganal_graph_scro;
void make_siganal_graph(int clear_old);
int max_sg_averages;
float sg_interleave_ratio;
int reinit_sg;
int new_averages_flag;
int old_timf3_pa;
#define SG_M 8
#define ZZ 100000000000000.

void addto_sg_avgpwr(void)
{
int i;
// Power spectra in sg_pwr have size sg_siz/2
// pwr[i  ] = an channel 0
// pwr[i+1] = pn channel 0
// pwr[i+2] = an channel 1
// pwr[i+3] = pn channel 1
for(i=0; i< sg_siz/2; i++)
  {
  sg_pwrsum[4*i  ]+=sg_pwr[sg_pa+4*i  ];
  sg_pwrsum[4*i+1]+=sg_pwr[sg_pa+4*i+1];
  sg_pwrsum[4*i+2]+=sg_pwr[sg_pa+4*i+2];
  sg_pwrsum[4*i+3]+=sg_pwr[sg_pa+4*i+3];
  }
}

void update_sg_avgpwr(void)
{
int i;
for(i=0; i< sg_siz/2; i++)
  {
  sg_pwrsum[4*i  ]+=
              sg_pwr[sg_pa+4*i  ]-sg_pwr[sg_px+4*i  ];
  sg_pwrsum[4*i+1]+=
              sg_pwr[sg_pa+4*i+1]-sg_pwr[sg_px+4*i+1];
  sg_pwrsum[4*i+2]+=
              sg_pwr[sg_pa+4*i+2]-sg_pwr[sg_px+4*i+2];
  sg_pwrsum[4*i+3]+=
              sg_pwr[sg_pa+4*i+3]-sg_pwr[sg_px+4*i+3];
  }
sg_px=(sg_px+2*sg_siz)&baseband_mask;
}

void new_sg_avgpwr(void)
{
int i;
int px;
// Power spectra in sg_pwr have size sg_siz/2
// pwr[i  ] = an channel 0
// pwr[i+1] = pn channel 0
// pwr[i+2] = an channel 1
// pwr[i+3] = pn channel 1
memset(sg_pwrsum,0,sg_siz*2*sizeof(double));
sg_px=(sg_pa-sg_numpow*2*sg_siz+baseband_size)&baseband_mask;
px=sg_px;
while(px != sg_pa)
  {
  for(i=0; i< sg_siz/2; i++)
    {
    sg_pwrsum[4*i  ]+=sg_pwr[px+4*i  ];
    sg_pwrsum[4*i+1]+=sg_pwr[px+4*i+1];
    sg_pwrsum[4*i+2]+=sg_pwr[px+4*i+2];
    sg_pwrsum[4*i+3]+=sg_pwr[px+4*i+3];
    }
  px=(px+2*sg_siz)&baseband_mask;
  }
}

void do_siganal(void)
{
int i, j, pa, px;
double dt1, dt2, dt3, dt4;
double ch1_ani, ch1_anq, ch2_ani, ch2_anq;
double ch1_pni, ch1_pnq, ch2_pni, ch2_pnq;
for(j=0; j<4; j+=2)
  {
  i=0;
  px=baseb_px;
  pa=(px+sg_siz)&baseband_mask;
  dt1=0;
  dt2=0;
  while(px != pa)
    {
    dt1+=d_baseb[4*px+j  ];
    sg_tmp[2*i  ]=d_baseb[4*px+j  ];
    dt2+=d_baseb[4*px+j+1];
    sg_tmp[2*i+1]=d_baseb[4*px+j+1];
    i++;
    px=(px+1)&baseband_mask;
    }
  dt3=sqrt(dt1*dt1+dt2*dt2);
  dt1/=dt3;
  dt2/=dt3;
  dt3/=sg_siz;    
  for(i=0; i<sg_siz; i++)
    {
    dt4=((dt1*sg_tmp[2*i  ]+dt2*sg_tmp[2*i+1])-dt3)*
                                                sg_window[i]/dt3;
    sg_tmp[2*i+1]=(-dt2*sg_tmp[2*i  ]+dt1*sg_tmp[2*i+1])*
                                                sg_window[i]/dt3;
    sg_tmp[2*i  ]=dt4;
    }
  d_fftforward(sg_siz, sg.fft_n, sg_tmp, sg_tab, sg_permute);
// separate the AN parts and the PN parts for i=1 to sg_siz-1
// AN[0]=tmp[0]
// PN[0]=tmp[sg_siz/2]
// AN[i]=tmp[i]+tmp[sg_siz-i]
// PN[i]=tmp[i]-tmp[sg_siz-i]
// pwr[2*i+j  ]=an[i]*an[i] for chan j/2
// pwr[2*i+j+1]=pn[i]*pn[i] for chan j/2
// AN
  pa=sg_pa;
  sg_fft[j  ]=sg_tmp[0];
  sg_fft[j+1]=0;
// PN
  sg_fft[sg_siz+j  ]=sg_tmp[1];
  sg_fft[sg_siz+j+1]=0;
  sg_pwr[pa+j  ]=sg_tmp[0]*sg_tmp[0];
  sg_pwr[pa+j+1]=sg_tmp[1]*sg_tmp[1];
  pa=(pa+4)&baseband_mask;
// Include the sideband power in the first 5 bins into the carrier.
// The power here also contributes to noise at higher offsets.
// This may be significant for very large fft sizes.
  dt2=pow(sg_tmp[0],2);
  for(i=1; i<6; i++)
    {
    dt2+=pow(sg_tmp[2*i  ]+sg_tmp[2*(sg_siz-i)  ],2.)+
         pow(sg_tmp[2*i+1]-sg_tmp[2*(sg_siz-i)+1],2.);
    }     
  dt2=1-dt2;
  if(dt2 < 0.9)
    {
// Skip data when more than 10% of the carrier is not DC.   
    baseb_px=(baseb_px+sg_interleave_points)&baseband_mask;
    return;
    }
  dt3=sqrt(dt2);
  sg_fft[j]*=dt3;
  for(i=1; i<sg_siz/2; i++)
    {
// AN
    sg_pwr[pa+j   ]=dt2*(pow(sg_tmp[2*i  ]+sg_tmp[2*(sg_siz-i)  ],2.)+
                      pow(sg_tmp[2*i+1]-sg_tmp[2*(sg_siz-i)+1],2.));
    sg_fft[j+4*i  ]=dt3*(sg_tmp[2*i  ]+sg_tmp[2*(sg_siz-i)  ]);
    sg_fft[j+4*i+1]=dt3*(sg_tmp[2*i+1]-sg_tmp[2*(sg_siz-i)+1]);
//PN
    sg_pwr[pa+j+1]=dt2*(pow(sg_tmp[2*i  ]-sg_tmp[2*(sg_siz-i)  ],2.)+
                          pow(sg_tmp[2*i+1]+sg_tmp[2*(sg_siz-i)+1],2.));
    sg_fft[2*sg_siz+4*i+j  ]=dt3*(sg_tmp[2*i  ]-sg_tmp[2*(sg_siz-i)  ]);
    sg_fft[2*sg_siz+4*i+j+1]=dt3*(sg_tmp[2*i+1]+sg_tmp[2*(sg_siz-i)+1]);
    pa=(pa+4)&baseband_mask;
    }
  }
sg_corrnum++;
for(i=0; i<sg_siz/2; i++)
  {
  ch1_ani=sg_fft[4*i  ];
  ch2_ani=sg_fft[4*i+2];
  ch1_anq=sg_fft[4*i+1];
  ch2_anq=sg_fft[4*i+3];
  sg_corrsum[2*i  ]+=ch1_ani*ch2_ani+ch1_anq*ch2_anq;
  sg_corrsum[2*i+1]+=ch1_anq*ch2_ani-ch1_ani*ch2_anq;
  ch1_pni=sg_fft[4*i+2*sg_siz  ];
  ch2_pni=sg_fft[4*i+2*sg_siz+2];
  ch1_pnq=sg_fft[4*i+2*sg_siz+1];
  ch2_pnq=sg_fft[4*i+2*sg_siz+3];
  sg_corrsum[2*i+sg_siz  ]+=ch1_pni*ch2_pni+ch1_pnq*ch2_pnq;
  sg_corrsum[2*i+sg_siz+1]+=ch1_pnq*ch2_pni-ch1_pni*ch2_pnq;
// correlation between an and pn.
  sg_anpn_corr[2*i  ]+=ch1_ani*ch2_pni+ch1_anq*ch2_pnq;
  sg_anpn_corr[2*i+1]+=ch1_anq*ch2_pni-ch1_ani*ch2_pnq;
  }
/*
if(sg_numpow == 1000)
  {  
  for(j=0; j<sg_siz/2; j++)
    {
    dt1=sqrt(sg_anpn_corr[2*j  ]*sg_anpn_corr[2*j  ]+
            sg_anpn_corr[2*j+1]*sg_anpn_corr[2*j+1]);

    fprintf(dmp,"\nj=%d  %f  %f",j,sg_anpn_corr[2*j  ]/dt1,
                               sg_anpn_corr[2*j+1]/dt1);
    }                              
  }
*/                            
sg_numpow++;
if(sg_valid)
  {
  if( new_averages_flag != 0)
    {
    if(sg_numpow < sg_corrnum-1)sg_numpow=sg_corrnum-1;
    if(sg_numpow > sg.avg)sg_numpow=sg.avg;
    new_sg_avgpwr();
    new_averages_flag = 0;
    }
  else
    {    
    if(sg_numpow > sg.avg)
      {
      sg_numpow=sg.avg;
      if(sg_corrnum%(8*sg.avg) ==0)
        {
        new_sg_avgpwr();
        }
      else
        {  
        update_sg_avgpwr();
        }
      }
    else
      {
      if(sg_numpow == 1)
        {
        new_sg_avgpwr();
        }
      else
        {  
        addto_sg_avgpwr();
        }
      }  
    }
  }
else
  {
  sg_numpow=0;
  sg_valid=TRUE;
  }  
sg_pa=(sg_pa+2*sg_siz)&baseband_mask;
baseb_px=(baseb_px+sg_interleave_points)&baseband_mask;
sc[SC_SG_UPDATE]++;
}

void new_sg_averages(void)
{
sg.avg=numinput_int_data;
if(sg.avg > max_sg_averages)sg.avg=max_sg_averages;
if(sg.avg < 1)sg.avg=1;
new_averages_flag=1;
make_modepar_file(GRAPHTYPE_SG);
sc[SC_SG_BUTTONS]++;
}

void make_sg_siz(void)
{
float t1;
t1=powf(2.0F,(float)sg.fft_n);
sg_siz=round(t1);
if(sg_siz > baseband_size>>3)
  {
  sg_siz=baseband_size>>3;
  sg.fft_n=make_power_of_two(&sg_siz);
  }
sg_interleave_points=1+(1-sg_interleave_ratio)*sg_siz;
}

void new_sg_fft_n(void)
{
// Wait for display to finish
while(sc[SC_SG_UPDATE] != sd[SC_SG_UPDATE])lir_sleep(10000);
sg.fft_n=numinput_int_data;
make_modepar_file(GRAPHTYPE_SG);
make_siganal_graph(TRUE);
reinit_sg=FALSE;
}

void new_sg_ygain(void)
{
double z;
z=(double)numinput_float_data;
if(z < 0.1)z=0.1;
if(z > 50)z=50;
z-=.01;
adjust_scale(&z);
sg.ygain=z;
make_modepar_file(GRAPHTYPE_SG);
sc[SC_SG_REDRAW]++;
}

void new_sg_ymax(void)
{
sg.ymax=numinput_int_data;
if(sg.ymax < 0)sg.ymax=0;
if(sg.ymax > 200)sg.ymax=200;
sg.ymax=sg.ymax-sg.ymax%5;
make_modepar_file(GRAPHTYPE_SG);
sc[SC_SG_REDRAW]++;
}

void fix_sg_xgain(void)
{
if(sg.xgain < 1)
  {
  sg_pixels_per_point=1;
  sg_points_per_pixel=1/sg.xgain;
  sg.xgain=1.0F/(float)(sg_points_per_pixel);
  }
else
  {
  sg_pixels_per_point=sg.xgain;
  sg_points_per_pixel=1;
  }
}

void new_sg_xgain(void)
{
float t1;
t1=sg.xgain;
sg.xgain=numinput_float_data;
if(sg.xgain < 0.001)sg.xgain=0.001;
if(sg.xgain > 900)sg.xgain=900;
fix_sg_xgain();
if(sg.xgain != t1)
  {
  make_modepar_file(GRAPHTYPE_SG);
  }
sc[SC_SG_REDRAW]++;
}

void set_ytop2(void)
{
if(sg.mode == 3)
  {
  sg_ytop2=sg.ytop-(sg.ytop-sg.ybottom)/4;
  sg_mode3_ymid=(sg.ytop+sg_ytop2)/2;
  sg_mode3_ypix=sg_ytop2-sg_mode3_ymid;
  if(sg_mode3_ypix > sg_mode3_ymid-sg.ytop)sg_mode3_ypix=sg_mode3_ymid-sg.ytop;
  sg_mode3_ypix--;
  }
else
  {
  sg_ytop2=sg.ytop;
  }  
}

void mouse_continue_siganal_graph(void)
{
reinit_sg=TRUE;
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Look in freq_control.c how to move a fixed size window.
  case SG_TOP:
  graph_borders((void*)&sg,0);
  sg.ytop=mouse_y;
  set_ytop2();
  graph_borders((void*)&sg,15);
  break;

  case SG_BOTTOM:
  graph_borders((void*)&sg,0);
  sg.ybottom=mouse_y;
  graph_borders((void*)&sg,15);
  break;
  
  case SG_LEFT:
  graph_borders((void*)&sg,0);
  sg.xleft=mouse_x;
  graph_borders((void*)&sg,15);
  break;
  
  case SG_RIGHT:
  graph_borders((void*)&sg,0);
  sg.xright=mouse_x;
  graph_borders((void*)&sg,15);
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
  case SG_NEW_AVGNUM: 
  mouse_active_flag=1;
  numinput_xpix=sgbutt[SG_NEW_AVGNUM].x1+9*text_width/2-1;
  numinput_ypix=sgbutt[SG_NEW_AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_sg_averages;
  return;
  
  case SG_NEW_MODE:
  sg.mode++;
  if(sg.mode >= NO_SG_MODES)sg.mode=0;
  set_ytop2();
  make_modepar_file(GRAPHTYPE_SG);
  sc[SC_SG_REDRAW]++;
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  return;

  case SG_NEW_FFT_N: 
  mouse_active_flag=1;
  numinput_xpix=sgbutt[SG_NEW_FFT_N].x1+5*text_width/2-1;
  numinput_ypix=sgbutt[SG_NEW_FFT_N].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_sg_fft_n;
  return;

  case SG_NEW_YGAIN: 
  mouse_active_flag=1;
  numinput_xpix=sgbutt[SG_NEW_YGAIN].x1+9*text_width/2-1;
  numinput_ypix=sgbutt[SG_NEW_YGAIN].y1+2;
  numinput_chars=5;
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_sg_ygain;
  return;

  case SG_NEW_YMAX: 
  mouse_active_flag=1;
  numinput_xpix=sgbutt[SG_NEW_YMAX].x1+5*text_width/2-1;
  numinput_ypix=sgbutt[SG_NEW_YMAX].y1+2;
  numinput_chars=3;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_sg_ymax;
  return;

  case SG_NEW_XGAIN: 
  mouse_active_flag=1;
  numinput_xpix=sgbutt[SG_NEW_XGAIN].x1+11*text_width/2-1;
  numinput_ypix=sgbutt[SG_NEW_XGAIN].y1+2;
  numinput_chars=6;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_sg_xgain;
  return;

  default:
// This should never happen.    
  lirerr(211053);
  break;
  }  
finish:;
//hide_mouse(sg_old_x1,sg_old_x2,sg_old_y1,sg_old_y2);
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
if(reinit_sg)make_siganal_graph(TRUE);
}

void mouse_on_siganal_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_SGBUTT; event_no++)
  {
  if( sgbutt[event_no].x1 <= mouse_x && 
      sgbutt[event_no].x2 >= mouse_x &&      
      sgbutt[event_no].y1 <= mouse_y && 
      sgbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_siganal_graph;
    return;
    }
  }
// Not button or border.
if(mouse_x > sg.xleft+2*text_width && mouse_x<sg.xright-2*text_width)
  {
  current_mouse_activity=mouse_nothing;
  mouse_active_flag=1;
  }
else
  {
// If we did not select a numeric input by setting numinput_flag
// we have to set mouse_active flag.
// Set the routine to mouse_nothing, we just want to
// set flags when the mouse button is released.    
  current_mouse_activity=mouse_nothing;
  mouse_active_flag=1;
  }
}


void make_siganal_graph(int clear_old)
{
int i;
int x1, x2, iy1, y2;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(sg_old_x1,sg_old_x2,sg_old_y1,sg_old_y2);
  lir_fillbox(sg_old_x1,sg_old_y1,sg_old_x2-sg_old_x1+1,
                                                    sg_old_y2-sg_old_y1+1,0);
  }
sg_old_y1=sg.ytop;
sg_old_y2=sg.ybottom;
sg_old_x1=sg.xleft;
sg_old_x2=sg.xright;

current_graph_minh=10*text_height;
current_graph_minw=10*text_width;
check_graph_placement((void*)(&sg));
clear_button(sgbutt, MAX_SGBUTT);
hide_mouse(sg.xleft,sg.xright,sg.ytop,sg.ybottom);  
sg_first_xpixel=sg.xleft+4*text_width;
sg_last_xpixel=sg.xright-2;
sg_interleave_ratio=0.8F;
make_sg_siz();
max_sg_averages=baseband_size/(2*sg_siz)-1;
if(sg.avg > max_sg_averages)sg.avg=max_sg_averages;
sg_y0=sg.ybottom-text_height-4;
scro[siganal_graph_scro].no=SIGANAL_GRAPH;
scro[siganal_graph_scro].x1=sg.xleft;
scro[siganal_graph_scro].x2=sg.xright;
scro[siganal_graph_scro].y1=sg.ytop;
scro[siganal_graph_scro].y2=sg.ybottom;
sgbutt[SG_LEFT].x1=sg.xleft;
sgbutt[SG_LEFT].x2=sg.xleft+2;
sgbutt[SG_LEFT].y1=sg.ytop;
sgbutt[SG_LEFT].y2=sg.ybottom;
sgbutt[SG_RIGHT].x1=sg.xright-2;
sgbutt[SG_RIGHT].x2=sg.xright;
sgbutt[SG_RIGHT].y1=sg.ytop;
sgbutt[SG_RIGHT].y2=sg.ybottom;
sgbutt[SG_TOP].x1=sg.xleft;
sgbutt[SG_TOP].x2=sg.xright;
sgbutt[SG_TOP].y1=sg.ytop;
sgbutt[SG_TOP].y2=sg.ytop+2;
sgbutt[SG_BOTTOM].x1=sg.xleft;
sgbutt[SG_BOTTOM].x2=sg.xright;
sgbutt[SG_BOTTOM].y1=sg.ybottom-2;
sgbutt[SG_BOTTOM].y2=sg.ybottom;
// Draw the border lines
//graph_borders((void*)&sg,7);
settextcolor(7);
iy1=sg.ytop+text_height/2;
y2=iy1+text_height;
x2=sg.xright-2;
x1=x2-17*text_width/2;
sgbutt[SG_NEW_AVGNUM].x1=x1;
sgbutt[SG_NEW_AVGNUM].x2=x2;
sgbutt[SG_NEW_AVGNUM].y1=iy1;
sgbutt[SG_NEW_AVGNUM].y2=y2;
x2=x1-2;
x1=x2-9*text_width/2;
sgbutt[SG_NEW_FFT_N].x1=x1;
sgbutt[SG_NEW_FFT_N].x2=x2;
sgbutt[SG_NEW_FFT_N].y1=iy1;
sgbutt[SG_NEW_FFT_N].y2=y2;
x1-=3*text_width/2;
make_button(x1,sg.ytop+text_height,sgbutt, SG_NEW_MODE,sg_modes[sg.mode]);
iy1+=text_height+2;
y2+=text_height+2;
x2=sg.xright-2;
x1=x2-17*text_width/2;
sgbutt[SG_NEW_YGAIN].x1=x1;
sgbutt[SG_NEW_YGAIN].x2=x2;
sgbutt[SG_NEW_YGAIN].y1=iy1;
sgbutt[SG_NEW_YGAIN].y2=y2;
x2=x1-2;
x1=x2-11*text_width/2;
sgbutt[SG_NEW_YMAX].x1=x1;
sgbutt[SG_NEW_YMAX].x2=x2;
sgbutt[SG_NEW_YMAX].y1=iy1;
sgbutt[SG_NEW_YMAX].y2=y2;
x2=sg.xright-2;
x1=x2-23*text_width/2;
iy1+=text_height+2;
y2+=text_height+2;
sgbutt[SG_NEW_XGAIN].x1=x1;
sgbutt[SG_NEW_XGAIN].x2=x2;
sgbutt[SG_NEW_XGAIN].y1=iy1;
sgbutt[SG_NEW_XGAIN].y2=y2;
sg_first_xpixel=sg.xleft+7*text_width;
//if(reinit_sg)return;
if(siganal_handle != NULL)
  {
  siganal_handle=chk_free(siganal_handle);
  }
init_memalloc(siganalmem, MAX_SIGANAL_ARRAYS);
mem( 1,&sg_fft,sg_siz*4*sizeof(double),0);
mem( 2,&sg_pwr,baseband_size*4*sizeof(float),0);
mem( 3,&sg_pwrsum, sg_siz*2*sizeof(double),0);
mem( 4,&sg_corrsum,sg_siz*2*sizeof(double),0);
mem( 5,&sg_window, sg_siz*sizeof(double),0);
mem( 6,&sg_tab, sg_siz*sizeof(D_COSIN_TABLE)/2,0);
mem( 7,&sg_permute,sg_siz*sizeof(unsigned int),0);
mem( 8,&sg_tmp,sg_siz*2*sizeof(double),0);
mem( 9,&sg_background,screen_height*sizeof(char),0);
mem(10,&sg_an1spectrum,screen_width*sizeof(short int),0);
mem(11,&sg_an2spectrum,screen_width*sizeof(short int),0);
mem(12,&sg_pn1spectrum,screen_width*sizeof(short int),0);
mem(13,&sg_pn2spectrum,screen_width*sizeof(short int),0);
mem(14,&sg_ancspectrum,2*screen_width*sizeof(short int),0);
mem(15,&sg_pncspectrum,2*screen_width*sizeof(short int),0);
mem(16,&sg_anpn_corr,2*sg_siz*sizeof(double),0);
mem(17,&sg_anpncorr_ispectrum,2*screen_width*sizeof(short int),0);
mem(17,&sg_anpncorr_qspectrum,2*screen_width*sizeof(short int),0);

// ******************************************************************
siganal_totmem=memalloc(&siganal_handle,"siganal");
for(i=0; i<screen_width; i++)sg_an1spectrum[i]=sg_y0;
for(i=0; i<screen_width; i++)sg_an2spectrum[i]=sg_y0;
for(i=0; i<screen_width; i++)sg_pn1spectrum[i]=sg_y0;
for(i=0; i<screen_width; i++)sg_pn2spectrum[i]=sg_y0;
for(i=0; i<2*screen_width; i++)sg_ancspectrum[i]=sg_y0;
for(i=0; i<2*screen_width; i++)sg_pncspectrum[i]=sg_y0;
for(i=0; i<2*screen_width; i++)sg_anpncorr_ispectrum[i]=sg_mode3_ymid;
for(i=0; i<2*screen_width; i++)sg_anpncorr_qspectrum[i]=sg_mode3_ymid;
init_d_fft(0, sg.fft_n, sg_siz, sg_tab, sg_permute);
make_d_window(4,sg_siz, 8, sg_window);
// init noise power spectra at the zero level
memset(sg_fft,0,sg_siz*4*sizeof(float));
memset(sg_corrsum,0,sg_siz*2*sizeof(double));
memset(sg_anpn_corr,0,sg_siz*sizeof(double));
memset(sg_pwrsum,0,sg_siz*2*sizeof(double));
sg_display_time=current_time();
resume_thread(THREAD_SCREEN);
fix_sg_xgain();
make_modepar_file(GRAPHTYPE_SG);
sg_hz_per_pixel=baseband_sampling_speed/sg_siz;
sc[SC_SG_REDRAW]++;
sg_pa=0;
sg_px=0;
sg_reset_time=current_time();
sg_display_time=sg_reset_time-2;
sg_corrnum=0;
sg_numpow=0;
sg_valid=FALSE;
new_averages_flag=0;
}

void init_siganal_graph(void)
{
// Set initial values in code or by reading
// a file of your own.
if (read_modepar_file(GRAPHTYPE_SG) == 0)
  {
sg_default:
  sg.xleft=screen_width/2;
  sg.xright=sg.xleft+50*text_width;
  sg.ytop=screen_height/2;
  sg.ybottom=sg.ytop+8.5*text_height;
  sg.mode=0;
  sg.avg=40;
  sg.fft_n=12;
  sg.ymax=0;
  sg.ygain=10.0;
  sg.xgain=1.0;
  sg.check=SG_VERNR;
  }
if(sg.mode < 0 ||
   sg.mode >= NO_SG_MODES ||
   sg.avg < 1 ||
   sg.ymax < 0 ||
   sg.ymax > 220 ||
   sg.ygain < 0.1 ||
   sg.ygain > 50 ||
   sg.xgain < 0.001 ||
   sg.xgain > 900 ||
   sg.check != SG_VERNR)goto sg_default;
set_ytop2();
siganal_graph_scro=no_of_scro;
reinit_sg=FALSE;
make_siganal_graph(FALSE);
no_of_scro++;
sg_flag=1;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

