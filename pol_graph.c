// Copyright (c) <2012 to 2016> <Leif Asbrink and Piotr Hewelt>
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
#include <time.h>
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "thrdef.h"
#include "seldef.h"
#include "sdrdef.h"
#include "fft1def.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define PG_WIDTH (7*text_width)
#define PG_HEIGHT (PG_WIDTH+3.5*text_height)  

#define XG_HEIGHT 8.6*text_height
#define XG_WIDTH 40*text_width+text_width

void make_pol_graph(int clear_old);
int pg_old_y1;
int pg_old_y2;
int pg_old_x1;
int pg_old_x2;
int pol_graph_scro;
int line2, line3;

double phasing_fast_time;
extern float phasing_update_interval;
int sign_c1, sign_c3;
int sign;
int channels_toggle = 1;
float xg_ampl_bal, xg_phase_shift;
int xg_old_y1;
int xg_old_y2;
int xg_old_x1;
int xg_old_x2;
int xg_oldx;
int xg_oldy;
int phasing_graph_scro;

void make_phasing_graph(int clear_old);

void check_pg_borders(void)
{
current_graph_minh=PG_HEIGHT;
current_graph_minw=PG_WIDTH;
check_graph_placement((void*)(&pg));
set_graph_minwidth((void*)(&pg));
}

void help_on_pol_graph(void)
{
int msg_no;
int event_no;
// Set msg invalid in case we are not in any select area.
msg_no=-1;
for(event_no=0; event_no<MAX_PGBUTT; event_no++)
  {
  if( pgbutt[event_no].x1 <= mouse_x && 
      pgbutt[event_no].x2 >= mouse_x &&      
      pgbutt[event_no].y1 <= mouse_y && 
      pgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case PG_TOP:
      case PG_BOTTOM:
      case PG_LEFT:
      case PG_RIGHT:
      msg_no=101;
      break;

      case PG_AUTO:
      msg_no=54;
      break;
  
      case PG_ANGLE:
      msg_no=55;
      break;

      case PG_CIRC:
      msg_no=56;
      break;

      case PG_AVGNUM:
      msg_no=57;
      break;
      }
    }  
  }
help_message(msg_no);
}  

void mouse_continue_pol_graph(void)
{
int i, j;
float t1,t2;
switch (mouse_active_flag-1)
  {
  case PG_TOP:
  if(pg.ytop!=mouse_y)goto pgm;
  break;

  case PG_BOTTOM:
  if(pg.ybottom!=mouse_y)goto pgm;
  break;

  case PG_LEFT:
  if(pg.xleft!=mouse_x)goto pgm;
  break;

  case PG_RIGHT:
  if(pg.xright==mouse_x)break;
pgm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&pg,0);
  if(pg_oldx==-10000)
    {
    pg_oldx=mouse_x;
    pg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-pg_oldx;
    j=mouse_y-pg_oldy;  
    pg_oldx=mouse_x;
    pg_oldy=mouse_y;
    pg.ytop+=j;
    pg.ybottom+=j;      
    pg.xleft+=i;
    pg.xright+=i;
    check_pg_borders();
    }
  graph_borders((void*)&pg,15);
  resume_thread(THREAD_SCREEN);
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
  case PG_AUTO:
  pg.adapt^=1;
  break;
  
  case PG_ANGLE:
  t1=mouse_x-pg_x0;
  t2=mouse_y-pg_y0;
  if(fabs(t1)+fabs(t2)>0)
    {
    t1=atan2(-t2,t1)+PI_L*pg.angle/180;
    }
  else
    {
    t1=0;
    }
// Polarization plane is periodic in 180 degrees.
// Place angle in the interval 0 to 180 degrees so sin(angle) is
// always positive.     
  while(t1>=PI_L)t1-=PI_L;
  while(t1 < 0)t1+=PI_L;
  pg.c1=cos(t1);
  t2=sin(t1);
  pg.c2=sin(t1)*cos(pg_b);    
  pg.c3=sin(t1)*sin(pg_b);    
  break;

  case PG_CIRC:
  pg_b=1.2*PI_L*(mouse_x-pg_x0)/(pgbutt[PG_CIRC].x2-pgbutt[PG_CIRC].x1);
  if(pg_b > 0.499*PI_L)pg_b= 0.499*PI_L;
  if(pg_b < -0.499*PI_L)pg_b=-0.499*PI_L;
  if(pg.c1 == 0)pg.c1=0.0001;
  t2=sqrt(1-pg.c1*pg.c1);
  if(pg.c2 < 0)t2*=-1;
  if(t2 == 0)t2=0.0001;
  pg.c2=t2*cos(pg_b);  
  pg.c3=t2*sin(pg_b);  
  break;

  case PG_AVGNUM:
  pg.avg=mouse_x-pg_x0;
  if(pg.avg < 1)pg.avg=1;
  if(pg.avg > pg.xright-pg_x0)pg.avg=pg.xright-pg_x0;
  break;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_pol_graph(TRUE);
if(genparm[SECOND_FFT_ENABLE] != 0)new_hg_pol();
pg_oldx=-10000;
}

void mouse_on_pol_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_PGBUTT; event_no++)
  {
  if( pgbutt[event_no].x1 <= mouse_x && 
      pgbutt[event_no].x2 >= mouse_x &&      
      pgbutt[event_no].y1 <= mouse_y && 
      pgbutt[event_no].y2 >= mouse_y) 
    {
    pg_old_y1=pg.ytop;
    pg_old_y2=pg.ybottom;
    pg_old_x1=pg.xleft;
    pg_old_x2=pg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_pol_graph;
    return;
    }
  }
// Not button or border.
// Do nothing.
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}

void make_pol_graph(int clear_old)
{
int i, j;
char *adafix[]={"Adapt","Fixed"};
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(pg_old_x1,pg_old_x2,pg_old_y1,pg_old_y2);
  lir_fillbox(pg_old_x1,pg_old_y1,pg_old_x2-pg_old_x1+1,
                                                    pg_old_y2-pg_old_y1+1,0);
  }
else
  {
  clear_button(pgbutt, MAX_PGBUTT);
  }
check_pg_borders();
hide_mouse(pg.xleft,pg.xright,pg.ytop,pg.ybottom);  
scro[pol_graph_scro].no=POL_GRAPH;
scro[pol_graph_scro].x1=pg.xleft;
scro[pol_graph_scro].x2=pg.xright;
scro[pol_graph_scro].y1=pg.ytop;
scro[pol_graph_scro].y2=pg.ybottom;
pgbutt[PG_LEFT].x1=pg.xleft;
pgbutt[PG_LEFT].x2=pg.xleft+2;
pgbutt[PG_LEFT].y1=pg.ytop;
pgbutt[PG_LEFT].y2=pg.ybottom;
pgbutt[PG_RIGHT].x1=pg.xright-2;
pgbutt[PG_RIGHT].x2=pg.xright;
pgbutt[PG_RIGHT].y1=pg.ytop;
pgbutt[PG_RIGHT].y2=pg.ybottom;
pgbutt[PG_TOP].x1=pg.xleft;
pgbutt[PG_TOP].x2=pg.xright;
pgbutt[PG_TOP].y1=pg.ytop;
pgbutt[PG_TOP].y2=pg.ytop+2;
pgbutt[PG_BOTTOM].x1=pg.xleft;
pgbutt[PG_BOTTOM].x2=pg.xright;
pgbutt[PG_BOTTOM].y1=pg.ybottom-2;
pgbutt[PG_BOTTOM].y2=pg.ybottom;
// Draw the border lines
graph_borders((void*)&pg,7);
settextcolor(7);
pg_oldx=-10000;
// Place the Adapt/Fixed button in lower left corner
i=1+text_width/2;
pgbutt[PG_AUTO].x1=pg.xleft+i;
pgbutt[PG_AUTO].x2=pgbutt[PG_AUTO].x1+5*text_width+i;
pgbutt[PG_AUTO].y2=pg.ybottom-2;
pgbutt[PG_AUTO].y1=pgbutt[PG_AUTO].y2-text_height-2;
lir_pixwrite(pgbutt[PG_AUTO].x1+i,pgbutt[PG_AUTO].y1+i/2,adafix[pg.adapt]);
if(kill_all_flag) return;
lir_hline(pgbutt[PG_AUTO].x1,pgbutt[PG_AUTO].y1,pgbutt[PG_AUTO].x2,7);
if(kill_all_flag) return;
lir_hline(pgbutt[PG_AUTO].x1,pgbutt[PG_AUTO].y2,pgbutt[PG_AUTO].x2,7);
if(kill_all_flag) return;
lir_line(pgbutt[PG_AUTO].x1,pgbutt[PG_AUTO].y1,
        pgbutt[PG_AUTO].x1,pgbutt[PG_AUTO].y2,7);
if(kill_all_flag) return;
lir_line(pgbutt[PG_AUTO].x2,pgbutt[PG_AUTO].y1,
        pgbutt[PG_AUTO].x2,pgbutt[PG_AUTO].y2,7);
if(kill_all_flag) return;
// Control for major axis angle
pg_x0=(pg.xleft+pg.xright)/2;
i=pg.xright-pg.xleft-2;
j=pg.ybottom-pg.ytop-3*text_height;
if(i>j)i=j;
i/=2;
pg_y0=pg.ytop+i+2;
pgbutt[PG_ANGLE].x1=pg_x0-i;
pgbutt[PG_ANGLE].x2=pg_x0+i;
pgbutt[PG_ANGLE].y1=pg_y0-i;
pgbutt[PG_ANGLE].y2=pg_y0+i;
// Control for interchannel phase (circular / linear)
pgbutt[PG_CIRC].x1=pgbutt[PG_ANGLE].x1;
pgbutt[PG_CIRC].x2=pgbutt[PG_ANGLE].x2;
pgbutt[PG_CIRC].y1=pgbutt[PG_ANGLE].y2+text_height/2;
pgbutt[PG_CIRC].y2=pgbutt[PG_CIRC].y1+text_height/2;
// Control for averaging parameter.
pgbutt[PG_AVGNUM].x1=pg_x0+1;
pgbutt[PG_AVGNUM].x2=pgbutt[PG_ANGLE].x2;
pgbutt[PG_AVGNUM].y1=pgbutt[PG_CIRC].y2+1;
pgbutt[PG_AVGNUM].y2=pgbutt[PG_CIRC].y2+text_height-2;
if(pg.adapt == 0)
  {
  i=pgbutt[PG_AVGNUM].x2-pgbutt[PG_AVGNUM].x1-1;
  if(pg.avg>i)pg.avg=i;
  i=pgbutt[PG_AVGNUM].x1+pg.avg;
  lir_fillbox(pgbutt[PG_AVGNUM].x1,pgbutt[PG_AVGNUM].y1,
             pgbutt[PG_AVGNUM].x2-pgbutt[PG_AVGNUM].x1,
             pgbutt[PG_AVGNUM].y2-pgbutt[PG_AVGNUM].y1,PC_CONTROL_COLOR);
  lir_line(i,pgbutt[PG_AVGNUM].y1,i,pgbutt[PG_AVGNUM].y2,15);
  }
dpg.xleft=pg.xleft;        
dpg.xright=pg.xright;        
dpg.ytop=pg.ytop;        
dpg.ybottom=pg.ybottom;        
resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_PG);
show_pol_time=current_time();
sc[SC_SHOW_POL]++;
}

void select_pol_default(void)
{
char s[80];
int line;
float t1;
char *amode[]={"Auto","Man"};
t1=180*atan2(dpg.c2,dpg.c1)/PI_L;
t1=fabs(t1-dpg.angle);
pol_menu:;
clear_screen();
line=5;
settextcolor(14);
lir_text(5,line,"INIT POLARISATION PARAMETERS FOR TWO CHANNEL RADIO");
line+=3;
settextcolor(7);
if(fft1_correlation_flag == 0)
  {
  sprintf(s,"A => Angle for channel 1 (0=Hor.) %.1f deg",dpg.angle);
  lir_text(5,line,s);
  line++; 
  sprintf(s,"B => Phase for channel 2          %.1f deg",dpg.ch2_phase);
  lir_text(5,line,s);
  line++;
  }
sprintf(s,"C => Amplitude for channel 2      %.4f dB",dpg.ch2_amp);
lir_text(5,line,s);
line++;
if(fft1_correlation_flag == 0)
  {
  sprintf(s,"D => Toggle adjust mode           %s",amode[dpg.adapt]);
  lir_text(5,line,s); 
  line++;
  sprintf(s,"E => Start polarization           %d",dpg.startpol);
  lir_text(5,line,s); 
  line++;
  sprintf(s,"F => Initial averaging            %d",dpg.avg);
  lir_text(5,line,s); 
  line++;
  sprintf(s,"H => Toggle phasing graph         %s",
                                                  onoff[dpg.enable_phasing]);
  lir_text(5,line,s); 
  line+=2;
  }
lir_text(5,line,"S => Save to disk and exit");
await_processed_keyboard();
line+=3;
if(kill_all_flag) return;
switch (lir_inkey)
  {
  case 'A':
  if(fft1_correlation_flag == 0)
    {
    lir_text(10,line,"Angle: 0=Hor, 90=V =>");
    dpg.angle=lir_get_float(32, line, 7, 0.F, 90.F);
    if(kill_all_flag) return;
    }
  break;
    
  case 'B':
  if(fft1_correlation_flag == 0)
    {
    lir_text(10,line,"Ch2 phase (deg) =>");
    dpg.ch2_phase=lir_get_float(29, line, 7, -360.F, 360.F);
    if(kill_all_flag) return;
    }
  break;

  case 'C':
  lir_text(10,line,"Ch2 ampl (dB) =>");
  dpg.ch2_amp=lir_get_float(27, line, 7, -30.F, 30.F);
  if(kill_all_flag) return;
  break;

  case 'D':
  if(fft1_correlation_flag == 0)
    {
    dpg.adapt^=1;
    }
  break;

  case 'E':
  if(fft1_correlation_flag == 0)
    {
    lir_text(10,line,"Start polarization:");
    dpg.startpol=lir_get_integer(30, line, 4, 0, 179);
    if(kill_all_flag) return;
    }
  break;
  
  case 'F':
  if(fft1_correlation_flag == 0)
    {
    lir_text(10,line,"Averages:");
    dpg.avg=lir_get_integer(20, line, 4, 1, pg.xright-pg_x0);
    if(kill_all_flag) return;
    }
  break;
  
  case 'H':
  if(fft1_correlation_flag == 0)
    {
    dpg.enable_phasing^=1;
    }
  break;

  case 'S':
// Get polarization coefficients for start polarization    
  if(fft1_correlation_flag == 0)
    {
    t1=dpg.angle+dpg.startpol;
    t1*=PI_L/180;
    dpg.c1=cos(t1);
    dpg.c2=sin(t1);
    dpg.c3=0;
    }
  else
    {
    dpg.startpol=0;
    dpg.c1=1;
    dpg.c2=0;
    dpg.c3=0;
    }  
  dpg.check=PG_VERNR;
  make_modepar_file(GRAPHTYPE_PG);
  return;
  }
goto pol_menu;    
}

void init_pol_graph(void)
{
float t1;
if (read_modepar_file(GRAPHTYPE_PG) == 0)
  {
pg_default:;
  dpg.xleft=27*text_width;
  dpg.xright=dpg.xleft+PG_WIDTH;
  dpg.ytop=0.72*screen_height;
  dpg.ybottom=dpg.ytop+PG_HEIGHT;
  lir_status=LIR_NEW_POL;
  dpg.startpol=0;
  dpg.avg=8;
  dpg.angle=0;
  dpg.adapt=0;
  dpg.c1=1;
  dpg.c2=0;
  dpg.c3=0;
  dpg.enable_phasing=0;
  dpg.ch2_amp=0;
  dpg.ch2_phase=0;
  return;
  }
t1=dpg.c1*dpg.c1+dpg.c2*dpg.c2+dpg.c3*dpg.c3;
if( (dpg.enable_phasing|1) != 1 ||
     dpg.check != PG_VERNR ||
     dpg.avg < 1 || dpg.avg > 1000 ||
     t1 < 0.98 || t1 > 1.02 ||
     dpg.angle < 0.F || dpg.angle > 180.00001 ||
    (dpg.adapt&-2) != 0 ||
     dpg.startpol < 0 || dpg.startpol > 179||
     dpg.ch2_amp < -30.F || dpg.ch2_amp > 30.F ||
     dpg.ch2_phase < -360.F || dpg.ch2_phase > 360.F)goto pg_default;
pg=dpg;
if(fft1_correlation_flag != 0)
  {
  if(pg.startpol != 0 ||
     pg.c1 != 1 ||
     pg.c2 != 0 ||
     pg.c3 != 0 ||
     pg.ch2_phase != 0)
    {
    lir_status=LIR_NEW_POL;
    pg.startpol=0;
    pg.c1=1;
    pg.c2=0;
    pg.c3=0;
    pg.ch2_phase=0;
    }
  if(fft1_correlation_flag == 1)
    {
    pg_ch2_c1=pow( 10, (-0.05 * dpg.ch2_amp ));
    }
  else
    {  
    pg_ch2_c1=1;
    }
  pg_ch2_c2=0;
  return;
  }
else
  {  
  pg_ch2_c1=cos(PI_L*dpg.ch2_phase/180.);
  pg_ch2_c2=sin(PI_L*dpg.ch2_phase/180.);
  t1=pow( 10, (-0.05 * dpg.ch2_amp ));
  pg_ch2_c1*=t1;
  pg_ch2_c2*=t1;
  if(fabs(pg_ch2_c1-1.F) < 0.0001)
    {
    pg_ch2_c1=1.0F;
    pg_ch2_c2=0.0F;
    }
  pg_oldx=-10000;
  pg_flag=1;
  pol_graph_scro=no_of_scro;
  make_pol_graph(FALSE);
  no_of_scro++;
  if(no_of_scro >= MAX_SCRO)lirerr(89);
  }
}

void c_to_ampl_phase(void);

void init_phasing(void)
{
phasing_update_interval=0.25;
}

void phasing(void)
{
if(xg_oldx == -10000)c_to_ampl_phase();
}

void check_xg_borders(void)
{
current_graph_minh=XG_HEIGHT;
current_graph_minw=XG_WIDTH;
check_graph_placement((void*)(&xg));
set_graph_minwidth((void*)(&xg));
}

void ampl_phase_to_c (float ampl, float phase)
{
if ((phase < -90)||(phase > 90))
{
	sign = -1;
}
else
{
	sign = 1;
}

	pg.c1 = sign * cos( atan( pow( 10, (0.05 * ampl))));
	pg.c2 = pow(( 1 - pg.c1*pg.c1)/(1+(tan(phase*PI_L/180)* tan(phase*PI_L/180))),0.5);
	pg.c3 = pg.c2 * tan (phase*PI_L/180);
	
	make_pol_graph(TRUE);
}


void c_to_ampl_phase (void)
{
char s2[80];
// Calculate ampl_bal from pg.c parameters
xg.ampl_bal = 20.0 * log10(0.00000000001 + fabs(tan(acos(pg.c1))));
if (xg.ampl_bal > 50)
{
	xg.ampl_bal = 50;
}
else if (xg.ampl_bal < -50)
{
	xg.ampl_bal = -50;
}	
xg.ampl_bal = xg.ampl_bal * 10;
xg.ampl_bal = rint(xg.ampl_bal);
xg.ampl_bal = xg.ampl_bal / 10;
sprintf(s2," [dB]  %+.1f  ",xg.ampl_bal);
//s2[12]=0;
settextcolor(9);
lir_pixwrite(xg.xleft+4*text_width+6,line2,s2);

// Phase_shift calculating and rounding
if (pg.c1>=0)
{
	sign_c1=0;
}
else
{
	sign_c1=-1;
}

if (pg.c3>=0)
{
	sign_c3=1;
}
else
{
	sign_c3=-1;
}
xg.phase_shift = 180.0 * (( atan ( pg.c3 / (pg.c2 + 0.00000000001) ) / PI_L ) + sign_c1 * sign_c3 );
xg.phase_shift = xg.phase_shift * 10;
xg.phase_shift = rint(xg.phase_shift);
xg.phase_shift = xg.phase_shift / 10;
sprintf(s2,"[deg] %+.1f  ", xg.phase_shift);
settextcolor(9);
//s2[12]=0;
lir_pixwrite(xg.xleft+4*text_width+6,line3,s2);
settextcolor(11);
sprintf(s2," [dB]   %+.1f   ",xg.m1ampl);
//s2[12]=0;
lir_pixwrite(xg.xleft+text_width,xg.ytop+5*text_height,s2);
sprintf(s2,"[deg]   %+.1f   ",xg.m1phase);
//s2[12]=0;
lir_pixwrite(xg.xleft+text_width,xg.ytop+6*text_height,s2);
sprintf(s2,"%+.1f   ",xg.m2ampl);
lir_pixwrite(xg.xleft+19*text_width,xg.ytop+5*text_height,s2);
sprintf(s2,"%+.1f   ",xg.m2phase);
lir_pixwrite(xg.xleft+19*text_width,xg.ytop+6*text_height,s2);
sprintf(s2,"%+.1f   ",xg.m3ampl);
lir_pixwrite(xg.xleft+29*text_width,xg.ytop+5*text_height,s2);
sprintf(s2,"%+.1f   ",xg.m3phase);
lir_pixwrite(xg.xleft+29*text_width,xg.ytop+6*text_height,s2);
settextcolor(7);
}

void show_phasing_parms(void)
{
char s[80];
hide_mouse(xg.xleft, xg.xright,xg.ytop,xg.ybottom);
sprintf(s," [dB] %+.1f   ",xg_ampl_bal);
//s[12]=0;
settextcolor(12);
lir_pixwrite(xg.xleft+26*text_width,line2,s);
sprintf(s,"[deg] %+.1f   ",xg_phase_shift);
//s[12]=0;
settextcolor(12);
lir_pixwrite(xg.xleft+26*text_width,line3,s);
settextcolor(7);
}

void mouse_continue_phasing_graph(void)
{
int i, j;
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Look in freq_control.c how to move a fixed size window.
  case XG_TOP:
  if(xg.ytop != mouse_y)goto xgm;
  break;

  case XG_BOTTOM:
  if(xg.ybottom != mouse_y)goto xgm;
  break;

  case XG_LEFT:
  if(xg.xleft != mouse_x)goto xgm;
  break;

  case XG_RIGHT:
  if(xg.xright == mouse_x)break;
xgm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&xg,0);
  if(xg_oldx==-10000)
    {
    xg_oldx=mouse_x;
    xg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-xg_oldx;
    j=mouse_y-xg_oldy;  
    xg_oldx=mouse_x;
    xg_oldy=mouse_y;
    xg.ytop+=j;
    xg.ybottom+=j;      
    xg.xleft+=i;
    xg.xright+=i;
    check_xg_borders();
    }
  graph_borders((void*)&xg,15);
  resume_thread(THREAD_SCREEN);
  break;
  
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
// Assuming the user wants to control hardware we
// allow commands only when data is not over the network.
if( (ui.network_flag&NET_RX_INPUT) == 0)
  {
  switch (mouse_active_flag-1)
    {
    case XG_INCREASE_PAR1:
    if (xg.ampl_bal>=49.8)
	{
        xg.ampl_bal=50;
	}
	else
	{
        xg.ampl_bal=0.2*(floor(5*xg.ampl_bal+0.01)+1);
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;

    case XG_DECREASE_PAR1:
    if (xg.ampl_bal<=-49.8)
	{
        xg.ampl_bal=-50;
	}
	else
	{
	    xg.ampl_bal=0.2*(ceil(5*xg.ampl_bal-0.01)-1);
    }
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;

	case XG_INCREASE_PAR2:
    if (xg.phase_shift>=179)
	{
		xg.phase_shift=-180;
	}
	else
	{
		xg.phase_shift=floor(xg.phase_shift)+1;
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
	break;

    case XG_DECREASE_PAR2:
	if (xg.phase_shift<=-179)
	{
		xg.phase_shift=180;
	}
	else
	{
		xg.phase_shift=ceil(xg.phase_shift)-1;
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
	break;

	case XG_INCREASE_PAR1_F:
    if (xg.ampl_bal>=49)
	{
        xg.ampl_bal=50;
	}
	else
	{
        xg.ampl_bal=floor(xg.ampl_bal)+1;
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;

    case XG_DECREASE_PAR1_F:
    if (xg.ampl_bal<=-49)
	{
        xg.ampl_bal=-50;
	}
	else
	{
        xg.ampl_bal=ceil(xg.ampl_bal)-1;
    }
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;

	case XG_INCREASE_PAR2_F:
    if (xg.phase_shift>=175)
	{
        xg.phase_shift=-180;
	}
	else
	{
		xg.phase_shift=5*(floor(xg.phase_shift/5+0.01)+1);//rounding to nearest 5
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
	break;

    case XG_DECREASE_PAR2_F:
	if (xg.phase_shift<=-175)
	{
		xg.phase_shift=180;
	}
	else
	{
		xg.phase_shift=5*(ceil(xg.phase_shift/5-0.01)-1);//rounding to nearest 5
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
	break;
	
	case XG_MEM_1:
	xg.m1ampl = xg.ampl_bal;
	xg.m1phase = xg.phase_shift;
    break;

	case XG_MEM_2:
	xg.m2ampl = xg.ampl_bal;
	xg.m2phase = xg.phase_shift;
    break;

	case XG_MEM_3:
	xg.m3ampl = xg.ampl_bal;
	xg.m3phase = xg.phase_shift;
    break;

	case XG_DO_1:
	ampl_phase_to_c(xg.m1ampl,xg.m1phase);
    break;

	case XG_DO_2:
	ampl_phase_to_c(xg.m2ampl,xg.m2phase);
    break;

	case XG_DO_3:
	ampl_phase_to_c(xg.m3ampl,xg.m3phase);
    break;

	
    case XG_DO_A: //orthogonal transformation
	xg.ampl_bal = - xg.ampl_bal;
	if (xg.phase_shift < 0)
	{
		xg.phase_shift = xg.phase_shift + 180;
	}
	else
	{
		xg.phase_shift = xg.phase_shift - 180;
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
	break;


	case XG_DO_B: //+/- phase
	xg.phase_shift = - xg.phase_shift;
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;

	case XG_DO_C: //invertion (+/-180 degrees)
	if (xg.phase_shift < 0)
	{
		xg.phase_shift = xg.phase_shift + 180;
	}
	else
	{
		xg.phase_shift = xg.phase_shift - 180;
	}
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;
	
	case XG_DO_D: //toggle individual channels
	channels_toggle = -1 * channels_toggle;
	xg.ampl_bal = channels_toggle * 50;
	xg.phase_shift = 0;
	ampl_phase_to_c(xg.ampl_bal,xg.phase_shift);
    break;
	
	case XG_COPY:
	ampl_phase_to_c(xg_ampl_bal,xg_phase_shift);
	break;
	
    default:
// This should never happen.
    lirerr(211053);
    break;
    }
  }
finish:;
leftpressed=BUTTON_IDLE;
mouse_active_flag=0;
make_phasing_graph(TRUE);
xg_oldx=-10000;
}

void new_ampl_bal(void)
{
xg_ampl_bal=numinput_float_data;
if(xg_ampl_bal < -50)xg_ampl_bal=-50;
if(xg_ampl_bal > 50)xg_ampl_bal=50;
pause_thread(THREAD_SCREEN);
show_phasing_parms();
resume_thread(THREAD_SCREEN);
}

void new_phase_shift(void)
{
xg_phase_shift=numinput_float_data;
if(xg_phase_shift <= -180)xg_phase_shift=-179.9;
if(xg_phase_shift > 180)xg_phase_shift=180;
pause_thread(THREAD_SCREEN);
show_phasing_parms();
resume_thread(THREAD_SCREEN);
}

void mouse_on_phasing_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_XGBUTT; event_no++)
  {
  if( xgbutt[event_no].x1 <= mouse_x &&
      xgbutt[event_no].x2 >= mouse_x &&
      xgbutt[event_no].y1 <= mouse_y &&
      xgbutt[event_no].y2 >= mouse_y)
    {
    xg_old_y1=xg.ytop;
    xg_old_y2=xg.ybottom;
    xg_old_x1=xg.xleft;
    xg_old_x2=xg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_phasing_graph;
    return;
    }
  }

mouse_active_flag=1;
numinput_xpix=xg.xleft+32*text_width;
if((mouse_x >(xg.xleft+26*text_width))&&(mouse_x<(xg.xleft+38*text_width))
	&&(mouse_y<(line3+text_height))&&(mouse_y>(line2-0.5*text_height)))
  {
  if(mouse_y > (line2+text_height))
    {
    numinput_ypix=line3;
    numinput_chars=7;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
	par_from_keyboard_routine = new_phase_shift;
	}
  else
    {
	numinput_ypix=line2;
    numinput_chars=7;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
	par_from_keyboard_routine = new_ampl_bal;
	}
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


void make_phasing_graph(int clear_old)
{
int iy;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(xg_old_x1,xg_old_x2,xg_old_y1,xg_old_y2);
  lir_fillbox(xg_old_x1,xg_old_y1,xg_old_x2-xg_old_x1+1,
                                      xg_old_y2-xg_old_y1+1,0);
  }
else
  {
  clear_button(xgbutt, MAX_XGBUTT);
  }
check_xg_borders();
hide_mouse(xg.xleft,xg.xright,xg.ytop,xg.ybottom);  
scro[phasing_graph_scro].no=PHASING_GRAPH;
// These are the coordinates of the border lines.
scro[phasing_graph_scro].x1=xg.xleft;
scro[phasing_graph_scro].x2=xg.xright;
scro[phasing_graph_scro].y1=xg.ytop;
scro[phasing_graph_scro].y2=xg.ybottom;
// Each border line is treated as a button.
// That is for the mouse to get hold of them so the window can be moved.
xgbutt[XG_LEFT].x1=xg.xleft;
xgbutt[XG_LEFT].x2=xg.xleft;
xgbutt[XG_LEFT].y1=xg.ytop;
xgbutt[XG_LEFT].y2=xg.ybottom;
xgbutt[XG_RIGHT].x1=xg.xright;
xgbutt[XG_RIGHT].x2=xg.xright;
xgbutt[XG_RIGHT].y1=xg.ytop;
xgbutt[XG_RIGHT].y2=xg.ybottom;
xgbutt[XG_TOP].x1=xg.xleft;
xgbutt[XG_TOP].x2=xg.xright;
xgbutt[XG_TOP].y1=xg.ytop;
xgbutt[XG_TOP].y2=xg.ytop;
xgbutt[XG_BOTTOM].x1=xg.xleft;
xgbutt[XG_BOTTOM].x2=xg.xright;
xgbutt[XG_BOTTOM].y1=xg.ybottom;
xgbutt[XG_BOTTOM].y2=xg.ybottom;
// Draw the border lines
graph_borders((void*)&xg,7);
xg_oldx=-10000;
iy=xg.ytop+text_height/2+4;
settextcolor(12);
make_button(xg.xleft+33*text_width,iy, xgbutt,XG_COPY,'<');
settextcolor(9);
make_button(xg.xleft+9.5*text_width,iy,xgbutt,XG_DO_A,'O');
make_button(xg.xleft+10.5*text_width+6,iy, xgbutt,XG_DO_B,'-');
make_button(xg.xleft+11.5*text_width+12,iy, xgbutt,XG_DO_C,'I');
make_button(xg.xleft+12.5*text_width+18,iy, xgbutt,XG_DO_D,'T');
iy+=text_height+5;
line2=iy-text_height/2;
make_button(xg.xleft+21*text_width,iy,xgbutt,XG_INCREASE_PAR1,24);
make_button(xg.xleft+2*text_width+6,iy,xgbutt,XG_DECREASE_PAR1,25);
make_button(xg.xleft+22*text_width+6,iy,xgbutt,XG_INCREASE_PAR1_F,24);
make_button(xg.xleft+text_width,iy,xgbutt,XG_DECREASE_PAR1_F,25);
iy+=1.5*text_height;
line3=iy-text_height/2;
make_button(xg.xleft+21*text_width,iy,xgbutt,XG_INCREASE_PAR2,24);
make_button(xg.xleft+2*text_width+6,iy,xgbutt,XG_DECREASE_PAR2,25);
make_button(xg.xleft+22*text_width+6,iy,xgbutt,XG_INCREASE_PAR2_F,24);
make_button(xg.xleft+text_width,iy,xgbutt,XG_DECREASE_PAR2_F,25);

settextcolor(11);
iy=xg.ybottom-text_height/2-2;
make_button(xg.xleft+10*text_width, iy, xgbutt,XG_DO_1,'<');
make_button(xg.xleft+20*text_width, iy, xgbutt,XG_DO_2,'<');
make_button(xg.xleft+30*text_width, iy, xgbutt,XG_DO_3,'<');

make_button(xg.xleft+11*text_width+6, iy, xgbutt,XG_MEM_1,'M');
make_button(xg.xleft+21*text_width+6, iy, xgbutt,XG_MEM_2,'M');
make_button(xg.xleft+31*text_width+6, iy, xgbutt,XG_MEM_3,'M');

settextcolor(7);
show_phasing_parms();
make_modepar_file(GRAPHTYPE_XG);
resume_thread(THREAD_SCREEN);
}

void init_phasing_window(void)
{
// Set initial values in code or by reading
// a file of your own.
if(pg.enable_phasing == 0 || fft1_correlation_flag != 0)return; 
if (read_modepar_file(GRAPHTYPE_XG) == 0)
  {
  xg.ytop=pg.ytop;
  xg.ybottom=xg.ytop+XG_HEIGHT;
  xg.xleft=pg.xright+text_width;
  xg.xright=xg.xleft+XG_WIDTH;
  xg.m1ampl=0;
  xg.m1phase=0;
  xg.m2ampl=0;
  xg.m2phase=0;
  xg.m3ampl=0;
  xg.m3phase=0;
  }
c_to_ampl_phase();
xg_ampl_bal=xg.ampl_bal;
xg_phase_shift=xg.phase_shift;
xg_flag=1;
phasing_graph_scro=no_of_scro;
make_phasing_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

void phasing_init_mode(void)
{
// A switch statement can be used to do different things depending on
// the processing mode.
// Open a window to allow mouse control of your own things
if(fft1_correlation_flag != 0)return;
  init_phasing_window();
/*
switch (rx_mode)
  {
  case MODE_WCW:
  case MODE_HSMS:
  case MODE_QRSS:
  break;

  case MODE_NCW:
  case MODE_SSB:
  break;

  case MODE_FM:
  case MODE_AM:
  case MODE_TXTEST:
  case MODE_RX_ADTEST:
  case MODE_TUNE:
  break;
  }
*/
}

