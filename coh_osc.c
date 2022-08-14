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
#include "sigdef.h"
#include "screendef.h"
#include "fft1def.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define CG_A 50.

void cg_show_traces(void);
void make_cg_trace(int mode,char *trtext,float *x,int trlevel,int txtlevel);

int cg_oscx=500;

void update_cg_traces(void)
{
hide_mouse(0,screen_width>>1,0,screen_height);
clear_cg_traces();
make_cg_trace(1,"Raw",baseb_raw,0,0);
make_cg_trace(4,"Raw",baseb_raw,1,0);
make_cg_trace(1,"Coh1",baseb,2,0);
make_cg_trace(4,"Coh1",baseb,3,0);
if(bg.agc_flag==1)
  {
  make_cg_trace(2,"Thr",baseb_threshold,4,0);
  make_cg_trace(12,"AGC",baseb_agc_level,5,0);
  }
if(bg.agc_flag == 2)
  {
  make_cg_trace(5,"AGC",baseb_agc_level,6,0);
  }
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  make_cg_trace(3,"",(float*)(baseb_ramp),2,0);
  make_cg_trace(1,"Det",baseb_fit,4,-text_height);
//  make_cg_trace(4,"Det",baseb_fit,4,-text_height);
  make_cg_trace(11,"Env",baseb_envelope,5,0);
//  make_cg_trace(4,"tmp",baseb_tmp,6,0);
  make_cg_trace(1,"SH1",baseb_sho1,6,0);
  make_cg_trace(4,"SH2",baseb_sho2,7,0);
  make_cg_trace(4,"Wide",baseb_wb_raw,9,0);
  }
make_cg_trace(8,"Carr",baseb_carrier,8,0);
make_cg_trace(9,"",baseb_carrier_ampl,8,0);
if( (cg.oscill_on&1) !=0 )
  {
  if(cg_osc_shift_flag==0)
    {
    if(cg.oscill_on > 1)
      {
      cg_show_traces();
      }
    cg.oscill_on |= 4;
    }
  }
}

void make_cg_trace(int mode,char *trtext,float *x,int trlevel,int txtlevel)
{
int i,j,k,ia,yzer,txty,osc_ptr; 
int amax;
float t1,ampfac;
short int *x_short;
unsigned char *x_char;
char s[80];
x_short=(short int*)x;
x_char=(unsigned char*)x;
txty=50;
if(cg.oscill_on == 0)return;
if(cg.oscill_on == 1)
  {
  if(trlevel > cg_max_trlevel)cg_max_trlevel=trlevel;
  return;
  }
cg_osc_points=mix2.new_points;
cg_oscw=screen_width/2;
if(cg_oscw > MAX_CG_OSCW)cg_oscw=MAX_CG_OSCW;
if(cg_osc_points > cg_oscw)cg_osc_points=cg_oscw;
osc_ptr=cg_osc_ptr;
ia=(baseb_ps-cg_osc_offset+baseband_size)&baseband_mask;
yzer=cg_lsep/2+trlevel*cg_lsep;
amax=cg_lsep/2-4;
cg_oscx=screen_last_xpixel-cg_oscw;
if(cg_osc_shift_flag == 0)
  {
  txty=txtlevel+yzer+2-text_height;
  settextcolor(7);
  lir_pixwrite(cg_oscx,txty,trtext);
  i=(ia+cg_osc_points-cg_oscw+baseband_size)&baseband_mask;
  j=(ia+cg_osc_points)&baseband_mask;
  sprintf(s,"%6d to %6d  inc=%d  ",i,j,cg_osc_offset_inc);
  lir_text(cg_oscx/text_width+33,screen_last_line,s);
  }
switch (mode)
  {
  case 1:
// Amplitude and phase.  
  ampfac=0.5*daout_gain/bg_amplimit;
  if(cg_no_of_traces+2 > CG_MAXTRACE)
    {
    lirerr(8877221);
    return;
    }
  if(cg_osc_shift_flag == 0)
    {
    settextcolor(4);
    lir_pixwrite(cg_oscx,txty+text_height,"pha");
    settextcolor(7);
    lir_pixwrite(cg_oscx,txty+2*text_height,"amp");
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=CG_A*ampfac*sqrt(x[2*ia]*x[2*ia]+x[2*ia+1]*x[2*ia+1]);
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    t1=0.5*amax*atan2(x[2*ia+1],x[2*ia])/PI_L;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces+1]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=15;
  cg_color[cg_no_of_traces+1]=12;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_yzer[cg_no_of_traces+1]=yzer;
  cg_no_of_traces+=2;
  break;

  case 2:
// Power or threshold   
  ampfac=0.5*daout_gain/bg_amplimit;
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(8877222);
    return;
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=CG_A*ampfac*sqrt(x[ia]);
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=14;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;

  case 3:
// Ramp   
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(8877223);
    return;
    }
  for(i=0; i<cg_osc_points; i++)
    {
    k=x_short[ia];
    if(k>amax)k=amax;
    if(k<-amax)k=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-k;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=10;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;

  case 4:
  ampfac=0.5*daout_gain/bg_amplimit;
  goto cmplx;
  case 7:
  ampfac=0.009*amax;
  
cmplx:;
  if(cg_no_of_traces+2 > CG_MAXTRACE)
    {
    lirerr(8877224);
    return;
    }
// Complex amplitude.  
  if(cg_osc_shift_flag == 0)
    {
    settextcolor(10);
    lir_pixwrite(cg_oscx,txty+text_height,"Re");
    settextcolor(13);
    lir_pixwrite(cg_oscx,txty+2*text_height,"Im");
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=CG_A*ampfac*x[2*ia];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    t1=CG_A*ampfac*x[2*ia+1];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces+1]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=10;
  cg_color[cg_no_of_traces+1]=13;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_yzer[cg_no_of_traces+1]=yzer;
  cg_no_of_traces+=2;
  break;
    
  case 5:
// Two channel agc level   
  ampfac=0.5*daout_gain/bg_amplimit;
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(887322);
    return;
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=2*CG_A*ampfac*x[2*ia];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    t1=2*CG_A*ampfac*x[2*ia+1];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces+1]=yzer+t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces  ]=14;
  cg_color[cg_no_of_traces+1]=14;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_yzer[cg_no_of_traces+1]=yzer;
  cg_no_of_traces+=2;
  break;

  case 6:
  ampfac=0.9*amax;
  for(i=0; i<cg_oscw; i++)
    {
    t1=ampfac*x[2*i];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*i+cg_no_of_traces]=yzer-t1;
    t1=ampfac*x[2*i+1];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*i+cg_no_of_traces+1]=yzer-t1;
    }
  cg_color[cg_no_of_traces]=10;
  cg_color[cg_no_of_traces+1]=13;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_yzer[cg_no_of_traces+1]=yzer;
  cg_no_of_traces+=2;
  break;

  case 8:
  for(i=0; i<cg_osc_points; i++)
    {
    t1=0.5*amax*atan2(x[2*ia+1],x[2*ia])/PI_L;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=12;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;

  case 9:
// Amplitude
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(8877225);
    return;
    }
  ampfac=daout_gain/bg_amplimit;
  for(i=0; i<cg_osc_points; i++)
    {
    t1=CG_A*ampfac*x[ia];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=15;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;

  case 10:
// Flags  
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(8877226);
    return;
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=-amax;
    j=x_char[ia];
    if(j==0)
      {
      t1=0;
      }
    else
      {  
      t1=.25*amax;
      }
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=15;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;

  case 11:
// Amplitude and phase.  
// Compute all the points. These arrays are detect output data
// and we may have changed them...
  ampfac=daout_gain/bg_amplimit;
  if(cg_no_of_traces+2 > CG_MAXTRACE)
    {
    lirerr(8877227);
    return;
    }
  if(cg_osc_shift_flag == 0)
    {
    settextcolor(4);
    lir_pixwrite(cg_oscx,txty+text_height,"pha");
    settextcolor(7);
    lir_pixwrite(cg_oscx,txty+2*text_height,"amp");
    }
  ia=(ia-osc_ptr+baseband_size)&baseband_mask;
  for(i=0; i<cg_osc_points+osc_ptr; i++)
    {
    t1=CG_A*ampfac*sqrt(x[2*ia]*x[2*ia]+x[2*ia+1]*x[2*ia+1]);
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*i+cg_no_of_traces]=yzer-t1;
    t1=0.5*amax*atan2(x[2*ia+1],x[2*ia])/PI_L;
    cg_traces[CG_MAXTRACE*i+cg_no_of_traces+1]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces]=15;
  cg_color[cg_no_of_traces+1]=12;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_yzer[cg_no_of_traces+1]=yzer;
  cg_no_of_traces+=2;
  break;

  case 12:
// One channel agc level   
  ampfac=0.5*daout_gain/bg_amplimit;
  if(cg_no_of_traces+1 > CG_MAXTRACE)
    {
    lirerr(887322);
    return;
    }
  for(i=0; i<cg_osc_points; i++)
    {
    t1=2*CG_A*ampfac*x[ia];
    if(t1>amax)t1=amax;
    if(t1<-amax)t1=-amax;
    cg_traces[CG_MAXTRACE*(i+osc_ptr)+cg_no_of_traces]=yzer-t1;
    ia=(ia+1)&baseband_mask;
    }
  cg_color[cg_no_of_traces  ]=14;
  cg_yzer[cg_no_of_traces]=yzer;
  cg_no_of_traces++;
  break;



  }
if(cg_osc_shift_flag == 0)settextcolor(7);
}  

void clear_cg_traces(void)
{
int i,j,osc_ptr;
cg_osc_points=mix2.new_points;
cg_oscw=screen_width/2;
if(cg_oscw > MAX_CG_OSCW)cg_oscw=MAX_CG_OSCW;
if(cg_osc_points > cg_oscw)cg_osc_points=cg_oscw;
osc_ptr=cg_oscw-cg_osc_points;
if(osc_ptr < 0)osc_ptr=0;
cg_lsep=screen_height/(cg_max_trlevel+1);
if( (cg.oscill_on&2) != 0 )
  {
  if(cg_osc_shift_flag<=1)
    {
    for(j=0; j<cg_no_of_traces; j++)
      {
      for(i=1; i<cg_oscw; i++)
        {
        lir_line(cg_oscx+i-1,cg_traces[CG_MAXTRACE*(i-1)+j],
                 cg_oscx+i,cg_traces[CG_MAXTRACE*i+j],0);
        if(kill_all_flag) return;
        }
      }
    }
// Move all data points backwards by cg_osc_points points and set pointer.
  for(i=0; i<osc_ptr; i++)
    {
    for(j=0; j<cg_no_of_traces; j++)
      {
      cg_traces[CG_MAXTRACE*i+j]=cg_traces[CG_MAXTRACE*(i+cg_osc_points)+j];
      }
    }
  }
cg_osc_ptr=osc_ptr;
cg_no_of_traces=0;
}



void cg_show_traces(void)
{
int i, j, yzer;
if( (cg.oscill_on&6) == 4)
  {
  cg.oscill_on|=2;
  for(j=0; j<cg_no_of_traces; j++)
    {
    yzer=cg_yzer[j];
    for(i=0; i<cg_oscw; i++)
      {
      cg_traces[CG_MAXTRACE*i+j]=yzer;
      }
    }  
  }
else
  {
  i=cg_lsep/2;
  while(i<screen_height)
    {
    lir_hline(cg_oscx,i,cg_oscx+cg_oscw,8);  
    if(kill_all_flag) return;
    i+=cg_lsep;
    }
  for(j=0; j<cg_no_of_traces; j++)
    {
    for(i=1; i<cg_oscw; i++)
      {      
      lir_line(cg_oscx+i-1,cg_traces[CG_MAXTRACE*(i-1)+j],
               cg_oscx+i,cg_traces[CG_MAXTRACE*i+j],cg_color[j]);
      if(kill_all_flag) return;
      }
    }  
  }
baseb_ps=(baseb_ps+mix2.new_points)&baseband_mask;
}


void shift_cg_osc_track(int shift)
{
int i,j;
j=cg_oscw/cg_osc_points;
if(j*cg_osc_points < cg_oscw)j++;
cg_osc_offset+=shift;
baseb_ps=(baseb_ps-j*cg_osc_points+baseband_size)&baseband_mask;
if( (cg.oscill_on&1) !=0 )cg_osc_shift_flag=1;
for(i=1; i<j; i++)
  {
  update_cg_traces();
  if(kill_all_flag) return;
  baseb_ps=(baseb_ps+cg_osc_points)&baseband_mask;
  cg_osc_shift_flag++;
  }
cg_osc_shift_flag=0;
update_cg_traces();
baseb_ps=(baseb_ps+cg_osc_points)&baseband_mask;
}



